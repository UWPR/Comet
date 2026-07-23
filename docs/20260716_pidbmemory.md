# PI_DB vs FI_DB RTS memory comparison; VarModSites compaction

Status: **IMPLEMENTED.** Investigated why a PI_DB RTS search (11.6GB) used nearly as much memory
as an FI_DB RTS search (12.8GB, after the FragmentPeptidesStruct repacking) against the same
heavily-modified (M-ox + STY-phospho) database, despite FI_DB additionally storing a 2.316-billion-
entry fragment-ion posting list that PI_DB has none of. Found that PI_DB's per-entry variable-mod
storage (`DBIndex::pcVarModSites`, a `vector<char>`) was actively expanding an already-compact
on-disk encoding into a dense, mostly-zero, separately-heap-allocated array for every one of tens
of millions of modified peptide entries. Replaced it with `VarModSites`, a compact, inline
(no heap allocation) struct mirroring the on-disk `(position, residue)` pair format directly, and
reordered `DBIndex`'s fields to eliminate resulting struct padding.

**Note on figures:** the commit message for this change quotes "13.1GB->9.2GB (-30%)" at the
80.7M-peptide production scale. That does not match this doc's own numbers: the production-scale
baseline measured here is 11.6GB, not 13.1GB, and the only real A/B measurement in the Validation
section below is at the 52.3M-peptide scale (6.79GB->5.03GB search RSS); the 80.7M-peptide figure
(~8.7-9.1GB) is an extrapolation from that measurement, not a direct measurement. Treat this doc's
numbers, not the commit message's, as authoritative.

## Background

Follow-up to `docs/20260715_fusedflush.md`'s PI_DB batch-search memory work. A real RTS PI_DB
search (`human.canonical.target-decoy.fasta.idx`, 80,720,511 peptides after M-ox + STY-phospho
enumeration, `max_variable_mods_in_peptide=2`) reported 11.6GB peak memory. The equivalent FI_DB
RTS search against the same database (after the `FragmentPeptidesStruct` repacking, see below)
reported 12.8GB -- despite FI_DB additionally holding `g_iFragmentIndex`, a 2.316-billion-entry
fragment-ion posting list (8.63GB on its own) that PI_DB doesn't need at all, since PI_DB doesn't
precompute fragment ions. The question: if FI_DB is paying for something PI_DB doesn't need, why
isn't PI_DB dramatically smaller?

### FragmentPeptidesStruct repacking (prerequisite, done first)

Before this investigation, `FragmentPeptidesStruct` (per-modified-variant metadata in FI_DB's
`g_vFragmentPeptides`) was repacked from 32 to 24 bytes: `iWhichPeptide` narrowed from `size_t` to
`unsigned int` (safe -- `g_vRawPeptides` never exceeds ~5-10M entries in practice, bounded to
UINT_MAX by a new check in `CometFragmentIndex::CreateFragmentIndex()`), and fields reordered
(`double` first) to avoid the resulting alignment gap. At 80M+ entries this saved ~645MB and was
validated against the real 12.8GB FI_DB figure quoted above.

## Finding: PI_DB expands already-compact data into a dense, per-entry-allocated array

Computed the actual memory breakdown for both paths using real `sizeof()` measurements:

| | PI_DB (`g_pvDBIndex`) | FI_DB (`g_vRawPeptides` + `g_vFragmentPeptides` + `g_iFragmentIndex`) |
|---|---|---|
| Base struct | 80.72M x 96B = 7.22GB | 5.01M x 72B + 80.72M x 24B = 2.14GB |
| Mod-state storage | ~75.7M modified entries x ~30-55B (heap, `vector<char>`) = 2.1-3.9GB | 0 (4B index into `g_vFragmentPeptides`, already counted above) |
| Fragment postings | 0 (not stored) | 2.316e9 x 4B = 8.63GB |
| **Total** | **~9.3-11.1GB** | **~10.77GB** |

Both bracket their respective observed totals well. The insight: PI_DB and FI_DB are bulky for
*different* reasons that happen to land in a similar range for this dataset -- FI_DB pays for the
fragment-ion postings it needs and PI_DB doesn't; PI_DB pays for redundant, un-deduplicated,
over-expanded modification-state storage that FI_DB avoids entirely via its compact `modNumIdx`.

### The specific inefficiency

`DBIndex::pcVarModSites` is populated in two places (`CometPeptideIndex.cpp`:
`ReadPeptideIndexEntry()`, `MaterializeIndexPeptideMods()`'s `buildEntry` lambda) and written to
disk in `WritePeptideIndex()`. The **on-disk format was already compact**: `cNumMods` (1 byte) +
`cNumMods x (position: 1 byte, residue: 1 byte)` pairs -- 3-5 bytes for a typical 1-2-mod peptide
(confirmed at `CometPeptideIndex.cpp:707-718` write side, `800-808` read side, before this change).

The **in-memory representation expanded this** into a dense `vector<char>` of length
`peptide_length + 2` (e.g. 22 bytes for a 20-residue peptide, with only 1-2 positions ever
nonzero), plus a separate heap allocation (~16-24 bytes allocator overhead) for every one of the
~75.7M modified entries -- a 5-10x expansion of data that was already stored efficiently, purely
for O(1) positional lookup convenience (`pcVarModSites[i]`).

## Fix: VarModSites (core/Types.h)

Compact, inline (no heap allocation) struct mirroring the on-disk pair format directly:

```cpp
struct VarModSites
{
   static const int MAX_SITES = 8;
   unsigned char cNumSites = 0;
   unsigned char position[MAX_SITES] = {};
   char          residue[MAX_SITES] = {};

   bool empty() const;
   void clear();
   char operator[](int pos) const;      // O(cNumSites) positional lookup, drop-in for the old vector<char>::operator[]
   bool set(int pos, char val);         // false if MAX_SITES exceeded
};
```

`sizeof(VarModSites) == 17` (measured). `MAX_SITES=8` comfortably covers the default
`max_variable_mods_in_peptide` (5) plus both termini (7 total) with margin; there is no compile-time
cap on this user-configurable parameter, so every construction site checks `set()`'s return value
and fails the build/read cleanly (rather than silently truncating) if a more permissive
configuration ever exceeds it -- `CometPeptideIndex.cpp` (`MaterializeIndexPeptideMods`,
`ReadPeptideIndexEntry`) and `CometSearch.cpp` (`MergeVarMods`, the legacy build path).

`DBIndex`'s fields were also reordered (8-byte-aligned fields first, `VarModSites` and `sPeptide`
-- both 1-byte aligned -- last) to avoid the padding a 1-byte-aligned field placed before an 8-byte
field would otherwise force. `sizeof(DBIndex)`: 96 -> 88 bytes (measured), on top of eliminating the
separate per-modified-entry heap allocation entirely.

### Migration approach

`operator[]` was added specifically so the majority of read call sites (~40 across
`CometPeptideIndex.cpp`, `CometSearch.cpp`, `CometFragmentIndex.cpp`, `CometMassSpecUtils.cpp`,
`CometSearchManager.cpp`) needed **no logic changes** -- only the field's type declaration changed.
This was a deliberate risk-reduction choice: several read sites (e.g. `CometSearch.cpp`'s
neutral-loss position scan, which walks positions ascending then descending and breaks on the first
match) depend on exact iteration order/bounds that would have been easy to get subtly wrong if
rewritten to iterate `cNumSites` directly. Only the four *construction* sites, and
`WritePeptideIndex()`'s write-out loop (which now iterates `sites.cNumSites` directly instead of
scanning all `iLen+2` positions -- a natural fit given the on-disk and in-memory formats are now
the same shape), were rewritten.

## Validation

- Full unit + integration suite: 21/21 pass, including T18 (byte-identical two-build determinism,
  exercises the rewritten `WritePeptideIndex` path) and T20 (real PI_DB build->disk->read->scoring->
  AScorePro round trip with an actual variable mod).
- Real A/B test (`git stash`, built before/after binaries): PI_DB index build from
  `human.small.fasta` with real M-oxidation + STY-phospho mods (`max_variable_mods_in_peptide=3`,
  8,937,433 plain peptides -> 52,296,965 total after enumeration):

| | Before | After | Change |
|---|---|---|---|
| Build peak RSS | 11.5GB | 9.5GB | -18% |
| Search peak RSS (20,097 real spectra) | 6.79GB | 5.03GB | -26% (1.76GB) |
| Search throughput | 482 Hz | 483 Hz | unchanged |
| `.idx` file | -- | -- | byte-identical (MD5 match) -- on-disk format unchanged by design |
| Search output | -- | -- | byte-identical (MD5 match) |

Scaling the 1.76GB search-time reduction from this 52.3M-peptide test up to the original
80.7M-peptide/11.6GB report (52.3/80.7 ratio) predicts roughly 2.5-2.9GB saved at that scale, taking
the original 11.6GB report down to somewhere in the 8.7-9.1GB range.
