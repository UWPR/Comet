# FI_DB Output-Order Jitter: Test and Observation

> Written 2026-07-09, spun out of the Phase 0 findings in
> `docs/20260708_evalue.md`. That document's primary investigation was into
> `FASTA_DB` `sp_rank`/E-value jitter (confirmed, root-caused, fix proposed).
> This document records a *separate*, smaller nondeterminism found in `FI_DB`
> while using it as a negative control -- it is not yet root-caused. A
> follow-up session will use this writeup to plan the fix.

## Summary

While confirming that `FI_DB` (fragment ion index search) does **not**
exhibit the `sp_rank` admission-race jitter that `FASTA_DB` does (see
`docs/20260708_evalue.md`'s Phase 0 findings), 2 of 5 identical repeated
`FI_DB` searches produced output that differed from the other 3. The
difference is confined to **one scan** (17089, charge 2, peptide
`AGAILEVCG`), which is reported as **three** separate output rows -- same
scan/mass/e-value/xcorr/sp_rank in all three, differing only in the
`next_aa` (flanking residue) column: `L`, `U`, `G`. All 5 runs report the
same three flanking-residue values (never more, never fewer) -- what
differs between runs is only their **relative print order**:

- `run1`, `run2`, `run5`: printed in order `L, U, G`
- `run3`, `run4`: printed in order `G, U, L` (the reverse)

`U` stays in the middle position in every observed run (5 runs is too few
to know if that's structural or coincidence).

Cross-checking the source database: the peptide `AGAILEVCG` occurs at
exactly **one** location in the entire database (`O60613|SEP15_HUMAN`), and
the residue immediately following it there is `U` (selenocysteine -- the
protein is literally named Selenoprotein F, and this is its UGA-readthrough
selenocysteine site). **Only the `U` row corresponds to a real match.** `L`
and `G` are not the true next residue at any locus of this peptide anywhere
in the database -- they are, respectively, the residues sitting **2** and
**3** positions further into the sequence, i.e. `U`(+1) `K`(+2) `L`(+3)
`G`(+4). This looks like an off-by-N flanking-residue lookup bug that is
specifically triggered when the true next residue is the rare/non-standard
residue `U`, producing two spurious extra candidate rows with incorrect
flanking annotations, whose relative print order (but not their identities
or count) is nondeterministic across threads.

## Developer note:

Developer note:  confirm if this is a tryptic digest search (which cleaves
after residues R and K).  If so, AGAILEVCG should not be possible as cleavage
after G is not a valid tryptic cleavage site.  The first order of business is
to identify how peptide AGAILEVCG is analyzed and stored for a tryptic
search. There is no point tracking down why a false flanking residue is
reported if the underlying problem is earlier in the analysis.

## Test setup

Same repro harness used for the `docs/20260708_evalue.md` Phase 0 work
(scratch directory, not committed -- rebuild from these params if rerunning):

```
database_name           = data/human.small.fasta.idx   # FI_DB (fragment ion index)
decoy_search             = 1
num_threads              = 64
num_output_lines         = 30
allowed_missed_cleavage  = 4
scan_range               = 1 20000
output_txtfile           = 1
```

Input: `data/20250520_Hela_60min_06.mzXML` (15,395 MS2 spectra in the scan
range). Index: `data/human.small.fasta.idx`, header `"Comet fragment ion
index plain peptides. Comet version 2026.02 rev. 0"`.

Ran the identical search command 5 times (`run1` .. `run5`), diffed the
tab-delimited output bodies (header line, which contains a timestamp,
excluded from comparison).

## Observation

```
run1.1-20000.txt: 1cb66b00abd49e147ea6fa3a6531d202
run2.1-20000.txt: 1cb66b00abd49e147ea6fa3a6531d202
run3.1-20000.txt: 5cd71e09bfcadbd562de35e76245631a
run4.1-20000.txt: 5cd71e09bfcadbd562de35e76245631a
run5.1-20000.txt: 1cb66b00abd49e147ea6fa3a6531d202
```

Two distinct output bodies across 5 runs (3 runs produced one, 2 runs
produced the other). This contradicts the assumption in
`docs/20260708_evalue.md` that `FI_DB` is fully deterministic under
concurrency (it does *not* contradict that document's specific H1 claim
about the `XcorrScoreI` admission gate having no fudge factor -- see
"What this is not" below).

### The exact rows, all 5 runs

```
$ grep '^17089\t9\t2.*AGAILEVCG' run{1,2,3,4,5}.1-20000.txt

run1: L, U, G   (in that print order)
run2: L, U, G
run3: G, U, L
run4: G, U, L
run5: L, U, G
```

Full row (identical across all 3 flanking-residue variants except the
`modified_peptide`/`next_aa` columns):

```
17089  9  2  831.412109  831.416039  1.24E+02  0.5870  0.0528  40.8  4  16  AGAILEVCG  Y.AGAILEVCG.<X>  Y  <X>  sp|O60613|SEP15_HUMAN  1  -  1673.7  10
```

(Columns: `scan num charge exp_neutral_mass calc_neutral_mass e-value xcorr
delta_cn sp_score ions_matched ions_total plain_peptide modified_peptide
prev_aa next_aa protein protein_count modifications retention_time_sec
sp_rank`. `<X>` is `L`, `U`, or `G` depending on the row.)

This is the **entirety** of the diff across all 15,395 spectra x up to 30
output lines each: exactly one scan, exactly three rows, same three
flanking-residue identities in every run, only the print order changes (and
only into one of two observed orderings: forward or fully reversed). No
other field -- `scan`, `charge`, masses, `e-value`, `xcorr`, `delta_cn`,
`sp_score`, `ions_matched/total`, `sp_rank`, `prev_aa`, `protein`,
`protein_count` -- differs at all, here or anywhere else in the output,
between any of the 5 runs.

### Two of the three flanking residues are objectively wrong

Searching the source database directly for the peptide sequence:

```
$ grep -A3 "O60613" data/human.small.fasta
>sp|O60613|SEP15_HUMAN Selenoprotein F OS=Homo sapiens OX=9606 GN=SELENOF PE=1 SV=4
MVAMAAGPSGCLVPAFGLRLLLATVLQAVSAFGAEFSSEACRELGFSSNLLCSSCDLLGQ
FNLLQLDPDCRGCCQEEAQFETKKLYAGAILEVCGUKLGRFPQVQAFVRSDKPKLFRGLQ
IKYVRGSDPVLKLLDDNGNIAEELSILKWNTDSVEEFLSEKLERI
```

A programmatic scan of every protein sequence in `data/human.small.fasta`
for the literal substring `AGAILEVCG` finds **exactly one** occurrence in
the entire 2,692-protein database: `O60613|SEP15_HUMAN`, at offset 86,
preceded by `Y` (matches the reported `prev_aa=Y` for all three rows) and
followed by `UKLGRF...`. Walking that suffix residue by residue:

| offset from peptide end | +1 | +2 | +3 | +4 | +5 | +6 |
|---|---|---|---|---|---|---|
| residue | `U` | `K` | `L` | `G` | `R` | `F` |

- The `next_aa=U` row is correct -- `U` (selenocysteine) is the true
  immediate next residue, consistent with this protein's biology (it is
  literally named Selenoprotein F, and this peptide sits right before its
  UGA-readthrough selenocysteine site, encoded via
  `add_U_selenocysteine`).
- The `next_aa=L` row's `L` is the residue at **offset +3** (i.e. two
  residues past the true boundary, skipping over `U` and `K`).
- The `next_aa=G` row's `G` is the residue at **offset +4** (three
  residues past the true boundary, skipping over `U`, `K`, and `L`).

There is no second genomic or proteomic locus anywhere in the database
where `AGAILEVCG` is followed by `L` or by `G` -- both are fabricated
flanking-residue values for the *same* single physical match, not real
second/third occurrences. The consistent +2/+3 offset pattern (always
landing past `U`, never past an ordinary residue) is a strong hint that
whatever computes/looks up the flanking residue is mis-stepping
specifically when the true next residue is the rare/non-standard code `U`.

## What this is not

- **Not the `docs/20260708_evalue.md` H1 mechanism.** H1 is about the
  `dXcorr + 0.00005 >= dLowestXcorrScore` fudge-factor admission gate in
  `StorePeptide()` / `XcorrScore()` (`CometSearch.cpp:5099-5125`), which is
  specific to `FASTA_DB`. The `FI_DB` branch of `XcorrScoreI`'s admission
  gate (`CometSearch.cpp:8520-8527`) uses an exact comparison with no
  epsilon, and `FI_DB` is dispatched one query per thread as an indivisible
  unit (no cross-thread writes to one `Query`) -- so this is not a
  candidate-admission race in the same sense.
- **Not an E-value/rank/candidate-count issue.** `e-value`, `xcorr`, and
  `sp_rank` are identical across all three rows and all 5 runs, and the
  *set* of three candidates is identical across all 5 runs (same three
  flanking residues, same count -- no candidate appears or disappears). The
  bug is entirely in (a) the correctness of the `L`/`G` flanking-residue
  values, which is a static/index-time property that should not depend on
  threading at all, and (b) the nondeterministic print order of the three
  rows, which is presumably a genuinely thread-order-dependent property.
  These may turn out to be two different bugs (one correctness, one
  ordering) rather than one.

## Likely area to investigate (not yet confirmed -- for the follow-up session)

- The offset pattern (+1 for the correct row, +3 and +4 for the two wrong
  rows -- i.e. always landing 2 and 3 positions past a `U`) suggests
  whatever computes `cNextAA` is advancing a cursor an extra 1 or 2
  positions in some code path specific to non-standard/rare residues.
  `U` is handled via `add_U_selenocysteine` as a distinct amino acid code
  from the 20 standard ones -- worth checking whether digestion, the
  fragment-index builder, or flanking-residue lookup has a residue-class
  branch (e.g. "standard AA vs other") that mishandles `U` specifically,
  as opposed to a purely off-by-one bug that would show up universally.
- `cPrevAA` / `cNextAA` are read from `g_vRawPeptides` keyed by
  `iWhichPeptide` during `FI_DB` candidate resolution/sorting, e.g.
  `CometSearch.cpp:1756-1757`. If the fragment index build enumerates
  *multiple* candidate windows around the same physical peptide occurrence
  when a rare residue like `U` is involved (rather than exactly one), each
  with its own -- and in two of three cases incorrect -- stored
  `cNextAA`, that would explain three coexisting candidates for one true
  match, and their nondeterministic relative order would then just be a
  side effect of whatever order they get collected into a container before
  the deterministic sort at `CometSearch.cpp:1502-1506` (or a downstream
  step, if one exists, that doesn't preserve that sort's ordering for
  values it considers equal).
- `cPrevAA`/`cNextAA` are written into the index at
  `CometFragmentIndex.cpp:697-698` / `:769-770` and read back at
  `CometFragmentIndex.cpp:1410-1411`. Worth checking whether index *build*
  time is where the three candidates are created (in which case rebuilding
  the index wouldn't help unless the build logic is fixed), or whether
  it's purely a read/lookup-time artifact -- e.g. dump/inspect the raw
  `.idx` file's entries for this peptide directly to see if there are
  really three stored entries or just one correct one plus lookup-time
  corruption.
- Existing project memory flags a related, previously-known issue: the
  `v2026.01.1` baseline had an "I/L long-path dedup bug (uses byte-exact
  `memcmp` instead of canonical L==I comparison)" for `FI_DB` peptide dedup
  (see `tests/unit/run_tests.py` T17 design notes). Given one of the two
  spurious rows here literally has flanking residue `L`, it's worth
  checking for a connection, though the mechanism described in that older
  bug (I/L canonicalization) doesn't obviously explain a `U`-adjacent
  offset error -- may be coincidental letter overlap rather than the same
  root cause.
- `equal_I_and_L = 1` was set in the repro params (inherited from the
  default params template used for the `docs/20260708_evalue.md` work).
  Not yet tested with `equal_I_and_L = 0` -- worth checking whether the
  spurious rows disappear with that off.

## Open questions for the follow-up session

1. Is this specific to `U` (selenocysteine), or does it reproduce for
   peptides adjacent to other non-standard/ambiguous residue codes (`B`,
   `J`, `O`, `X`, `Z`)? `data/human.small.fasta` likely has very few
   selenoprotein peptides to sample from (only a couple dozen known human
   selenoproteins) -- the larger database used in the companion large-DB
   E-value rerun, `/mnt/c/Work/data/HUMAN.fasta.20260127`, would have more
   to test against if it retains the same selenoproteins.
2. Does this reproduce with `equal_I_and_L = 0`?
3. Dump the raw `.idx` file entries for peptide `AGAILEVCG` (or the raw
   protein/offset it maps to) directly -- are there really 3 stored
   fragment-index entries with 3 different `cNextAA` values, or 1 correct
   stored entry plus 2 that get fabricated/corrupted at read/lookup time?
   This determines whether the fix belongs in `CometFragmentIndex.cpp`
   (build) or `CometSearch.cpp` (lookup/search).
4. Does the same anomaly appear in `PI_DB` (peptide index) search against
   the same database/peptide, or is it `FI_DB`-index-specific?
5. Single-thread control (`num_threads = 1`) was not run for this specific
   `FI_DB` repro -- worth confirming whether the *print order* becomes
   deterministic at `num_threads=1` (would confirm a threading component to
   the ordering, independent of the separate correctness bug in the `L`/`G`
   values, which should persist regardless of thread count since it looks
   like a static data problem).
6. Is `U` in the middle position in every run coincidental (5 runs, 2
   distinct orderings observed, both of which happen to keep `U` in the
   middle) or structural (e.g. `U`'s candidate is generated/inserted via a
   different code path than `L`/`G`'s, and only the L/G pair's relative
   order is actually nondeterministic)? More repeated runs would help
   distinguish these.

## Scope note

This does not affect `dExpect`, `xcorr`, or `sp_rank` -- unlike the
`FASTA_DB` jitter in `docs/20260708_evalue.md`, this is not an E-value
reproducibility bug. It is a correctness bug (a database match reported
with a flanking residue that does not correspond to any real occurrence)
combined with a separate print-order nondeterminism. Treat as a separate
workstream from the `docs/20260708_evalue.md` `StorePeptide` fix.

## Follow-up session findings (2026-07-09, same day): Developer note resolved

Investigated the Developer note directly against the code and the live
`data/human.small.fasta.idx` on disk. Two findings, in order of discovery.

### 1. Not a tryptic search -- the params' enzyme setting never applied

`database_name = data/human.small.fasta.idx` in the repro params points at a
*prebuilt* index. Comet does not rebuild it from FASTA + the run's
`comet.params`; it reads the `.idx` as-is. `CometFragmentIndex::
ReadPlainPeptideIndex()` (`CometFragmentIndex.cpp:1204-1217`)
unconditionally overwrites `g_staticParams.enzymeInformation.
{iSearchEnzymeOffSet, szSearchEnzymeBreakAA, szSearchEnzymeNoBreakAA}` from
the index file's own `Enzyme:` header line -- silently, with no
mismatch warning against whatever `search_enzyme_number` the run's params
requested.

The header of the actual file on disk reads:

```
Enzyme: Cut_everywhere [0 - -]
LengthRange: 8 13
```

This is a **no-enzyme** index (matches the T17 no-enzyme len-8-13 build
convention already noted in project memory/`tests/unit/run_tests.py`
design notes). So `AGAILEVCG` ending in `G` and preceded by `Y` is
completely unremarkable -- cleavage rules never applied to this index
build at all. The Developer note's hypothesis was correct in spirit
("if tryptic, this shouldn't be possible") but the premise was false: this
was never effectively a tryptic search, regardless of what
`search_enzyme_number` the repro's `comet.params` specified.

### 2. The real bug: a residue-packing table collision on `U` (and `B`/`J`/`O`/`X`/`Z`)

`CometSearch/core/Types.h:291-364` packs peptides <=12 residues into a
5-bit-per-residue `uint64_t` (`kAA5bit`/`PackPeptide`) for fast per-protein
dedup, then reconstructs the display string later (`k5bitAA`/
`UnpackPeptide`):

- `kAA5bit['U'] == 0` -- the same code used for the table's unused/pad
  slots. (`B`, `J`, `O`, `X`, `Z` all map to 0 too -- `U` just happens to be
  the one this database's selenoprotein triggers.)
- `k5bitAA[0] == '\0'` -- so `UnpackPeptide()` writes a premature NUL byte
  the instant it decodes a slot that was originally `U`.

Dumped every window at this exact locus directly from the live
`data/human.small.fasta.idx` (via `tests/unit/compare_idx.py`'s
`_PepReader`):

| length | stored text | mass (MH+) | next_aa | status |
|---|---|---|---|---|
| 9  | `AGAILEVCG`     | 832.42  | U | correct |
| 10 | *(absent)*      | --      | -- | **silently dropped** |
| 11 | `AGAILEVCG`     | 1111.47 | L | **truncated display**, real mass |
| 12 | `AGAILEVCG`     | 1224.56 | G | **truncated display**, real mass |
| 13 | `AGAILEVCGUKLG` | 1281.58 | R | correct (long-path storage, len > 12, untouched by pack/unpack) |

Mass deltas confirm these are three genuinely different real peptides, not
one match reported three times: 1111.47 - 832.42 = 279.05 ~= U(150.95) +
K(128.09); 1224.56 - 1111.47 = 113.08 ~= L(113.08). "AGAILEVCG",
"AGAILEVCGUK", and "AGAILEVCGUKL" are all independently valid no-enzyme
windows starting at the same protein position.

Two distinct failure modes stack here:

- **The 10-mer is silently lost, not just mis-displayed.** `_seenShort`
  (`CometSearch.h:383`) is *one* dedup set shared across all lengths <=12
  for a protein (cleared once per protein at `CometSearch.cpp:1242`, not
  partitioned per length). The 9-mer's packed key has zero-padding in slot
  9; the 10-mer's key has the same zero in slot 9 because `U` also packs to
  0 -- the two keys are bit-identical. `_seenShort.insert(key).second`
  (`CometSearch.cpp:3442`) returns false for the 10-mer (already "seen" as
  the 9-mer), so it is silently rejected and never written to the index.
  `AGAILEVCGU` cannot be found by *any* FI_DB search, tryptic or not --
  this is unconditional peptide loss, independent of the enzyme-override
  issue in finding 1.
- **The 11-mer/12-mer survive but display wrong.** Their packed keys
  diverge from the 9-mer's (K/L fill slots 10/11 with nonzero codes further
  along), so they get their own `DBIndex`/`.idx` entries -- but
  `UnpackPeptide` -> `strcpy(dbi.sPeptide, szSeq)`
  (`CometFragmentIndex.cpp:761,767`) truncates their *stored text* at the
  same NUL-at-`U` point, so both display as `AGAILEVCG`.

That display truncation then cascades into fragment-index construction.
`AddFragments()` (`CometFragmentIndex.cpp:394`) reconstructs `sPeptide` as
a `std::string` from the already-truncated `szPeptide` char array and
**recomputes both the fragment-ion ladder and the precursor mass from that
9-character string** (`iEndPos = sPeptide.length() - 1`,
`CometFragmentIndex.cpp:416-422`) -- it does not use the correctly-stored
`dPepMass`/length metadata from the original digestion at all for these
entries. This is why the original repro observed identical
`calc_neutral_mass`/`xcorr`/`e-value`/`sp_rank` across all three printed
rows: the 11-mer and 12-mer entries were reduced to the exact same
9-residue fragment-index contribution as the true 9-mer, even though their
real masses (1111/1224 Da) are nowhere near the query's ~831 Da precursor.

### Net assessment

- Finding 1 (enzyme override) is a repro-methodology gotcha worth fixing
  separately (a `.idx`-vs-`comet.params` enzyme mismatch should at minimum
  warn, since a user could unknowingly run what they believe is a tryptic
  search against a stale no-enzyme index) but is not itself a correctness
  bug in search results.
- Finding 2 (packing-table collision) is a real correctness bug, affecting
  any peptide -- in any search, any enzyme, any database -- containing `U`,
  `B`, `J`, `O`, `X`, or `Z` within a <=12-residue index window. One flavor
  (dedup-key collision) silently deletes a real peptide from the index;
  the other (Unpack/`strcpy` truncation) corrupts the displayed sequence
  and the fragment-index mass/ion-ladder for surviving entries that share
  its truncated prefix.
- The print-order nondeterminism from the original writeup is likely a
  fourth, separate/unrelated issue, as already suspected there (container
  iteration order downstream of the deterministic sort at
  `CometSearch.cpp:1502-1506`).
- **Fix location:** `CometSearch/core/Types.h`'s `kAA5bit`/`k5bitAA` tables
  need distinct, non-colliding codes for the 6 non-standard residues (5
  bits covers 32 values; only 21 are currently used, so there is room),
  plus `AddFragments()` should trust the stored peptide length/mass rather
  than re-deriving both from a NUL-terminated C string.

**Resolved (2026-07-09, same day as this writeup):** both fixes landed --
commit `9f171942` ("Fix 5-bit peptide packing collision for non-standard
residue codes") gives `B`, `J`, `O`, `U`, `X`, `Z` their own distinct codes
21-26 in `kAA5bit`/`k5bitAA` (`core/Types.h:290-299`, whose own comment now
cites this doc), and commit `c920b7bc` ("Harden AddFragments against silent
peptide-string truncation") addressed the second half. This section is no
longer open work; treat it as the record of what was found and fixed, not a
pending TODO.
