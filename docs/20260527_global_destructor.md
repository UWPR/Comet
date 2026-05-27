# The Delay: C++ Global Destructor Chains at Program Exit

When `g_pvInputFiles.size() == 0` the function returns and `main()` exits.
Before the process terminates, the C++ runtime calls the destructors of all
file-scope global objects — and for an MHC (no-enzyme) search those globals
are enormous.

---

## The Four Culprits

### 1. `g_vRawPeptides` — `vector<PlainPeptideIndexStruct>` (biggest offender)

`WriteFIPlainPeptideIndex` populates this during the k-way merge write loop
for every peptide, so it can have tens of millions of entries when it returns.
It is never cleared before program exit.

Each `PlainPeptideIndexStruct` originally contained a `std::string sPeptide`.
For non-SSO strings (peptides >= 16 chars), each destructor calls `free()` on
the heap allocation.  For 20 million peptides, that is 20 million individual
`free()` calls through the system allocator, which must walk its free-list,
handle coalescing, and may cause cache/TLB thrashing.

### 2. `g_pvProteinsList` — `vector<vector<comet_fileoffset_t>>`

One inner vector per unique peptide (maps peptide to list of protein offsets).
Never cleared inside `WriteFIPlainPeptideIndex`.  At program exit, each inner
vector's destructor calls `free()` on its heap buffer.  Again: millions of
individual `free()` calls.

### 3. `g_pvProteinNames` — `map<long long, IndexProteinStruct>`

A red-black tree.  Its destructor traverses and deallocates each tree node
individually — slower than freeing a contiguous vector because each node is a
separate heap allocation with poor locality.

### 4. `MOD_SEQS` — `vector<string>`

Populated by `PermuteIndexPeptideMods` -> `ModificationsPermuter::getModifiableSequences`.
Each entry is a heap-allocated `std::string`.  Never cleared.

---

## What IS Cleaned Up (Not the Problem)

| Structure | Freed where |
|-----------|-------------|
| `g_pvDBIndex` | `g_pvDBIndex.clear()` inside `WriteFIPlainPeptideIndex` |
| `g_vvvPepGenShort/Long` | `.clear()` in `GeneratePlainPeptideIndex` |
| `_pbSearchMemoryPool` | `CometSearch::DeallocateMemory` |
| `PEPTIDE_MOD_SEQ_IDXS`, `MOD_SEQ_MOD_NUM_START/CNT` | Leaked (no `delete[]`) — but raw `int*` arrays are reclaimed by the OS instantly at exit, not via destructors, so they add no delay |

---

## The Fix

The time cost is real (millions of `free()` calls) but it is completely silent —
it happens after the `" - done. (...)"` message.  Two options were identified.

### Option A — Explicit clear before return

Makes the cost visible in timing, but does not eliminate it.
At end of `WriteFIPlainPeptideIndex`, before `return bSucceeded`:

```cpp
{ vector<PlainPeptideIndexStruct>().swap(g_vRawPeptides); }
{ vector<vector<comet_fileoffset_t>>().swap(g_pvProteinsList); }
g_pvProteinNames.clear();
{ vector<string>().swap(MOD_SEQS); }
{ vector<FragmentPeptidesStruct>().swap(g_vFragmentPeptides); }
```

### Option B — `_exit(0)` for index-only runs

Eliminates the cost entirely.  Safe since `fclose(fp)` has already run.
In `CometSearchManager.cpp`, after `WriteFIPlainPeptideIndex` + `DeallocateMemory`:

```cpp
if (g_pvInputFiles.size() == 0 && !m_bRTSIndexBuild)
{
    fflush(stdout);
    fflush(stderr);
    _exit(0);   // skip all destructors; OS reclaims memory instantly
}
```

`_exit()` bypasses C++ static-duration destructors while still allowing stdio to
be flushed first.  For a write-and-exit workflow this is the right tool.

---

## What Was Actually Done

Both options were combined with two additional struct refactors.  The work was
done in stages; timings below are from the MHC no-enzyme search (189.7M unique
peptides).

---

### Baseline (before any changes)

| Milestone | Time |
|-----------|------|
| `done:` message | 3m 41s |
| `g_pvProteinsList` free at exit | 374 s (189M inner-vector `free()` calls) |
| **Total wall time** | ~10 min |

---

### Stage 1 — `g_pvProteinsList` CSR flat layout (commit `fad3990b`)

Replaced `vector<vector<comet_fileoffset_t>> g_pvProteinsList` with a CSR
(Compressed Sparse Row) layout class `ProteinsListCSR` in `CometDataInternal.h`,
storing all protein offsets in two flat arrays (`m_flat` + `m_off`).  This
eliminated the 189M individual inner-vector heap allocations and their `free()` calls.

Simultaneously, `LenResult::prots` (per-length sort/dedup working buffer) was
flattened from `vector<vector<comet_fileoffset_t>>` to `prots_flat + prots_cnt`
flat arrays, eliminating a further 189M temporary heap allocations during the
parallel sort phase.

`g_pvDBIndex.clear()` was moved to run **after** `g_vRawPeptides.swap()` (not
before) to keep the allocator's size-class bins warm for the string frees that
follow.

Option A (explicit timed free block) was implemented to make per-structure costs
visible in output:

```
- freed g_vRawPeptides:
- freed g_pvDBIndex:
- freed g_pvProteinsList:
- freed g_pvProteinNames:
- freed MOD_SEQS:
- freed g_vFragmentPeptides:
- freed PEPTIDE_MOD_SEQ_IDXS:
```

Option B (`_exit(0)`) was also added in the same commit for standalone index-only
runs (guarded against the RTS path — see Stage 4).

| Milestone | Time |
|-----------|------|
| `done:` message | 3m 06s |
| `g_vRawPeptides` free | 4873 ms (still slow — `DBIndex::sPeptide` strings) |
| `g_pvDBIndex` free | 126 s (104M non-SSO string `free()` calls) |
| `g_pvProteinsList` free | 81 ms (CSR: single flat-array free) |
| **Total wall time** | ~5m 19s |

---

### Stage 2 — `DBIndex::sPeptide` → `char sPeptide[MAX_PEPTIDE_LEN]` (commit `01d19c2c`)

Replaced `std::string sPeptide` in `DBIndex` with `char sPeptide[MAX_PEPTIDE_LEN]`
(51 bytes).  Struct fields were reordered for a packed 96-byte layout (was
80 bytes struct + up to 32 bytes heap per non-SSO string).

For 189.7M MHC entries:

| Layout | Memory |
|--------|--------|
| Before: 80 B struct × 189.7M + ~3.3 GB heap (104M non-SSO strings) | ~18.5 GB |
| After: 96 B struct × 189.7M, no heap | ~18.2 GB |
| **Net** | **−300 MB** |

The 126-second `g_pvDBIndex.clear()` cost collapsed to < 1 s because there are
no longer any heap string buffers to free — the vector destructor is a single
contiguous-block `free()`.

| Milestone | Time |
|-----------|------|
| `done:` message | 2m 44s |
| `g_vRawPeptides` free | 4834 ms (still slow — `PlainPeptideIndexStruct::sPeptide`) |
| `g_pvDBIndex` free | 873 ms (96 B trivial structs + empty `pcVarModSites` vectors) |
| `g_pvProteinsList` free | 81 ms |
| **Total wall time** | ~2m 51s (standalone, `_exit(0)` fires after explicit frees) |

---

### Stage 3 — `PlainPeptideIndexStruct::sPeptide` → `char szPeptide[MAX_PEPTIDE_LEN]` (commit `3bc7bb1f`)

Replaced `std::string sPeptide` with `char szPeptide[MAX_PEPTIDE_LEN]` (51 bytes)
in `PlainPeptideIndexStruct`, renamed to `szPeptide` (Hungarian notation for
null-terminated char array).  Fields reordered for a packed 72-byte layout
(was 64 bytes struct + heap).

For 189.7M MHC entries:

| Layout | Memory |
|--------|--------|
| Before: 64 B struct × 189.7M + ~3.2 GB heap (104M non-SSO strings) | ~14.5 GB |
| After: 72 B struct × 189.7M, no heap | ~12.7 GB |
| **Net** | **−1.8 GB** |

`g_vRawPeptides.clear()` now costs ~0 ms (trivially copyable struct, single `free()`).

All call sites updated: `.assign()` → `memcpy` + NUL, `.size()` → `strlen()`,
`.c_str()` → direct `char*`, `==` → `strcmp`, `.find('*')` → `strchr()`.

---

### Stage 4 — `_exit(0)` RTS guard (commit `be49239f`)

The `_exit(0)` added in Stage 1 was incorrectly firing on the RTS search path
(`InitializeSingleSpectrumSearch` → `CreateFragmentIndex` → `DoSearch`) because
`g_pvInputFiles.size() == 0` is also true for RTS searches.

Fix: added `bool m_bRTSIndexBuild` member to `CometSearchManager`, set to `true`
in `CreateFragmentIndex()` and `CreatePeptideIndex()` before calling `DoSearch()`,
reset to `false` after `DoSearch()` returns.  The `_exit(0)` guard became:

```cpp
if (g_pvInputFiles.size() == 0 && !m_bRTSIndexBuild)
```

`CreateFragmentIndex` / `CreatePeptideIndex` are the only callers of `DoSearch()`
from the RTS path, so this flag is the minimal correct discriminator.

---

### Final State (MHC no-enzyme, 189.7M peptides)

| Milestone | Time |
|-----------|------|
| `done:` message | 2m 44s |
| `g_vRawPeptides` free | ~0 ms (trivially copyable struct array) |
| `g_pvDBIndex` free | 873 ms (empty `pcVarModSites` vector destructors, memory-bandwidth bound at ~18 GB) |
| `g_pvProteinsList` free | 81 ms (single flat-array free) |
| `g_pvProteinNames` free | 6 ms |
| `MOD_SEQS` free | 0 ms |
| **Total wall time** | ~2m 51s (standalone; `_exit(0)` fires after explicit frees) |

For the RTS path (`InitializeSingleSpectrumSearch`), `_exit(0)` does not fire.
The explicit free block still runs, costing ~1 s total after the `.idx` is written.
