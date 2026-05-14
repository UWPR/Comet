# Sparse Spectrum Matrix: Format Analysis and Optimization Guide

**Date:** 2026-04-23  
**Scope:** `ppfSparseSpScoreData`, `ppfSparseFastXcorrData`, `ppfSparseFastXcorrDataNL` in `struct Query`  
**Files:** `CometDataInternal.h`, `CometPreprocess.cpp`, `CometSearch.cpp`, `CometPostAnalysis.cpp`

---

## 1. Current Format Mechanics

Each of the three sparse matrices uses a **two-level chunked pointer array**:

```
float*  ppf[iFastXcorrDataSize]    outer: NULL = no data in that 100-bin band
float   block[SPARSE_MATRIX_SIZE]  leaf: allocated on first nonzero write
```

`SPARSE_MATRIX_SIZE = 100` (compile-time constant in `CometData.h`)

**Construction** (`CometPreprocess.cpp`):

```cpp
pScoring->iFastXcorrDataSize = (iArraySize / SPARSE_MATRIX_SIZE) + 1;
// ... allocate outer array ...
for (int i = 0; i < iArraySize; ++i) {
    if (value > FLOAT_ZERO || value < -FLOAT_ZERO) {
        int x = i / SPARSE_MATRIX_SIZE;
        if (ppf[x] == NULL)
            ppf[x] = new float[SPARSE_MATRIX_SIZE]();  // lazy-alloc
        ppf[x][i - x * SPARSE_MATRIX_SIZE] = value;
    }
}
```

**Scoring access** (`CometSearch.cpp`, `XcorrScore`/`XcorrScoreI`):

```cpp
int x = bin / SPARSE_MATRIX_SIZE;
if (!(bin <= 0 || x > iMax || ppf[x] == NULL))
    dXcorr += ppf[x][bin - x * SPARSE_MATRIX_SIZE];
```

This is an **O(1) random-access lookup** requiring two pointer dereferences: one into the outer pointer array, one into the leaf block.

### Key Size Formula

```
iArraySizeGlobal = (int)((dPeptideMassHigh + dCushion) / dFragmentBinSize)
  where dCushion = dInputTolerancePlus + 5.0  (≈ 8 Da for default 3 Da tolerance)
```

For `dPeptideMassHigh = 5000 Da` (default), `dCushion = 8 Da`:

---

## 2. Array Sizing by Fragment Bin Size

*Assumes `dPeptideMassHigh = 5000 Da`, `SPARSE_MATRIX_SIZE = 100`*

| `dFragmentBinSize` | `iArraySizeGlobal` | `iFastXcorrDataSize` | Outer array (64-bit) | Outer cache tier |
|---|---|---|---|---|
| 1.0005 Da | 5,005 | 51 | 408 B | L1 |
| 0.50 Da | 10,016 | 101 | 808 B | L1 |
| 0.20 Da | 25,040 | 251 | 2.0 KB | L1 |
| 0.10 Da | 50,080 | 501 | 3.9 KB | L1 |
| 0.05 Da | 100,160 | 1,002 | 7.8 KB | L2 |
| 0.02 Da | 250,400 | 2,505 | 19.6 KB | L2 |
| 0.01 Da | 500,800 | 5,009 | 39.1 KB | L3 |

The outer pointer array fits in L1 data cache (typically 32–64 KB) for bin sizes ≥ 0.10 Da.

---

## 3. Memory Per Matrix vs. Bin Size

### Fast-XCorr Preprocessing: The Nonzero Bin Multiplier

The fast-XCorr preprocessing (applied once per spectrum in `CometPreprocess`) subtracts a
`2 × iXcorrProcessingOffset + 1 = 151`-bin running average from each bin. Every spectrum peak
at position P creates nonzero values at positions `[P-75, P+75]` — a 151-bin influence zone.
These values are all above `FLOAT_ZERO = 1e-6` and are retained in the sparse matrix.

This is the dominant factor in the nonzero bin count:

```
nnz ≈ min(N_peaks × 151,  0.85 × iArraySize)
```

The upper bound (0.85 × iArraySize) applies when halos from neighboring peaks overlap.
Overlap is governed by the ratio of the 151-bin halo width to the average inter-peak spacing
(`~100 Da / dFragmentBinSize` bins):

| `dFragmentBinSize` | Avg inter-peak spacing (bins) | Halo overlap | nnz (N=300 peaks) |
|---|---|---|---|
| 1.0005 Da | ~100 | 1.51× — full overlap | ~4,250 (85% of array) |
| 0.50 Da | ~200 | 0.76× — heavy | ~8,500 (85% of array) |
| 0.20 Da | ~500 | 0.30× — moderate | ~21,300 (85% of array) |
| 0.10 Da | ~1,000 | 0.15× — light | ~42,600 (85% of array) |
| 0.05 Da | ~2,000 | 0.08× — minimal | ~45,300 (plateau) |
| 0.02 Da | ~5,000 | 0.03× — isolated | ~45,300 (plateau) |
| 0.01 Da | ~10,000 | 0.02× — isolated | ~45,300 (plateau) |

For bins ≤ 0.05 Da the nonzero count **plateaus at ~45,300** because peak halos no longer
overlap. The spectrum array grows, but nnz does not.

### Leaf Block Utilization

At coarse bin sizes (≥ 0.2 Da), heavy halo overlap means nearly every 100-bin chunk in the
array contains at least one nonzero value. Fill per active chunk is 85%+. At fine bin sizes
(≤ 0.05 Da), only the ~600 chunks directly containing peak halos are allocated; each is ~75%
full (151 nonzero values distributed across 200 slots of a 2-chunk span).

### Memory Per Matrix (S=100, N=300 peaks)

| `dFragmentBinSize` | nnz | Active chunks | Fill/chunk | Leaf memory | Outer array | **Total/matrix** |
|---|---|---|---|---|---|---|
| 1.0005 Da | 4,254 | 51 | 83% | 19 KB | 0.4 KB | **20 KB** |
| 0.50 Da | 8,513 | 101 | 84% | 39 KB | 0.8 KB | **40 KB** |
| 0.20 Da | 21,284 | 251 | 85% | 98 KB | 2 KB | **100 KB** |
| 0.10 Da | 42,568 | 501 | 85% | 195 KB | 3.9 KB | **199 KB** |
| 0.05 Da | 45,300 | 866 | 52% | 338 KB | 7.8 KB | **346 KB** |
| 0.02 Da | 45,300 | 886 | 51% | 346 KB | 19.6 KB | **365 KB** |
| 0.01 Da | 45,300 | 893 | 51% | 348 KB | 39.1 KB | **387 KB** |

Three matrices are allocated per `Query` (SpScore, FastXcorr, FastXcorrNL when
`bUseWaterAmmoniaLoss` is true; two otherwise). For fine-bin searches with large batch sizes
the per-spectrum overhead is **~750–1,160 KB** for all three matrices.

---

## 4. Optimal SPARSE_MATRIX_SIZE by Bin Size

Two competing constraints govern the ideal chunk size S:

**Goal A — Outer array in L1 cache (≤ 2 KB = 256 pointer slots):**
```
S  ≥  iArraySize / 256
```

**Goal B — Leaf block fill ≥ 20% (justified allocation cost):**
```
S  ≤  halo_width / 0.20  =  151 / 0.20  =  755
```

| `dFragmentBinSize` | `iArraySize` | S_min (Goal A) | S_max (Goal B) | Feasible? | Recommended S | Outer array | Fill/block |
|---|---|---|---|---|---|---|---|
| 1.0005 Da | 5,005 | 20 | 755 | Yes | **32** | 1.25 KB (L1) | >50% |
| 0.50 Da | 10,016 | 40 | 755 | Yes | **64** | 1.25 KB (L1) | >50% |
| 0.20 Da | 25,040 | 98 | 755 | Yes | **128** | 1.57 KB (L1) | >50% |
| 0.10 Da | 50,080 | 196 | 755 | Yes | **256** | 1.57 KB (L1) | 59% |
| 0.05 Da | 100,160 | 392 | 755 | Yes | **512** | 1.57 KB (L1) | 29% |
| 0.02 Da | 250,400 | 979 | 755 | **No** | 100 † | 19.6 KB (L2) | 75% |
| 0.01 Da | 500,800 | 1,957 | 755 | **No** | 100 † | 39.1 KB (L3) | 75% |

† For bins ≤ 0.02 Da the two goals conflict. Choosing S=1024 to satisfy Goal A yields
**only 14.7% fill** and causes each of ~300 active blocks to consume 4 KB each — 1.2 MB
of leaf memory per matrix, 5× worse than S=100. Choosing S=100 keeps leaf blocks well-filled
(75%) while accepting that the outer array lives in L2/L3. **S=100 is the memory-optimal
choice when the two goals conflict.**

### Practical Impact of Outer Array Cache Tier

The outer array is **read-only during scoring** and is accessed once per theoretical fragment
ion per peptide candidate. With thousands of peptide candidates scored per spectrum, the outer
array warms up quickly and stays resident:

- **L1 (≤ 4 KB):** negligible access latency; no measurable impact.
- **L2 (4–32 KB, 0.05–0.10 Da range):** ~5 cycle penalty per miss; amortized over thousands
  of peptides, impact is < 1% of total scoring time.
- **L3 (32–64 KB, 0.01 Da):** ~40 cycle penalty per cold miss; again amortized and minor.

The outer pointer array is never the performance bottleneck. The leaf block working set
(240–350 KB for fine bins) fits comfortably in L2/L3 and stays warm across all peptide
candidates for a given spectrum.

### Implication for the S=16 Suggestion

A prior suggestion to reduce `SPARSE_MATRIX_SIZE` from 100 to 16 (one cache line per block)
was evaluated for 1 Da bins. **It must not be applied universally:**

| Metric | S=16, 1 Da bins | S=16, 0.01 Da bins |
|---|---|---|
| Outer array | 2.5 KB (L1 ✓) | 250 KB (L3 ✗) |
| Active chunks | ~266 | ~2,830 |
| Leaf memory | 17 KB | 181 KB |

S=16 would increase the outer array at 0.01 Da from 39 KB to 250 KB — well into L3 territory
and directly in the per-peptide scoring hot path. For a codebase that supports both 1 Da and
0.02 Da bin modes, S=16 is a regression. S=100 is the better universal default.

---

## 5. COO with Binary Search: Crossover Analysis

### Format Definition

COO (Coordinate format) stores only the nonzero entries as parallel sorted arrays:

```cpp
struct SparseXcorrCOO {
    vector<uint32_t> indices;  // sorted bin indices
    vector<float>    values;   // parallel intensities
};
```

Lookup: binary search over `indices` to find `bin`, return `values[pos]`.

### Memory Comparison

At fine bin sizes (≤ 0.05 Da), nnz ≈ 45,300 (plateau):

```
COO memory = 45,300 × (4 + 4) = 362,400 bytes ≈ 353 KB  (constant)
```

| `dFragmentBinSize` | Current S=100 (per matrix) | COO (per matrix) | Memory winner |
|---|---|---|---|
| 1.0005 Da | 20 KB | 33 KB | **Current** |
| 0.20 Da | 100 KB | 353 KB | **Current** |
| 0.10 Da | 199 KB | 353 KB | **Current** |
| 0.05 Da | 346 KB | 353 KB | **Current** (tie) |
| 0.02 Da | 365 KB | 353 KB | **COO** (+12 KB) |
| 0.01 Da | 387 KB | 353 KB | **COO** (+34 KB) |

**COO becomes smaller in memory at approximately `dFragmentBinSize ≈ 0.04 Da`**, where the
growing outer array tips the current format above the fixed COO footprint.

The advantage is narrow: at 0.01 Da, COO saves ~34 KB per matrix = ~100 KB for all three
matrices combined. For a batch of 10,000 spectra, that is ~1 GB savings, which is meaningful
but must be weighed against the performance cost below.

### Performance Comparison

For a peptide with ~300 theoretical fragment ions, the per-peptide scoring cost is:

| Operation | Current format | COO binary search |
|---|---|---|
| Per-ion lookup | O(1): 2 pointer dereferences | O(log nnz): binary search |
| Comparisons/ion | ~2 | log₂(45,300) ≈ **15.5** |
| Total for 300 ions | ~300 dereferences | **~4,640 comparisons** |
| Relative cost | 1× | ~**15×** slower |

Even after the working set (COO: 353 KB; current: 260–387 KB) warms into L2/L3 cache, the
binary search executes ~15 comparisons per ion lookup with unpredictable branch outcomes.
The current format performs exactly two loads, both with predictable access patterns.

**COO is never a better choice for XcorrScore performance, at any bin size.** The fast-XCorr
preprocessing is the root cause: by design it populates ~45,300 nonzero bins, which makes the
nnz too large for binary search to be competitive.

### Summary of Crossover Points

| Crossover criterion | `dFragmentBinSize` threshold | Practical? |
|---|---|---|
| COO smaller in memory per matrix | ~0.04 Da | Yes — but margin is small |
| COO smaller for 3 matrices combined | ~0.04 Da | ~100 KB savings at 0.01 Da |
| COO better for scoring performance | **Never** | nnz always too large |

---

## 6. Feasibility of Dual Data Structure Support

To support both formats with a runtime branch on `dFragmentBinSize`, the following changes
would be required:

### Scope of Changes

| Component | Changes required |
|---|---|
| `CometDataInternal.h` (`struct Query`) | Add `SparseXcorrCOO` members alongside existing `float**` pointers; add `bUseCOO` flag; restructure destructor |
| `CometPreprocess.cpp` | Branch on `bUseCOO` in all three sparse matrix construction loops; COO path uses `push_back` + final `sort` |
| `CometSearch.cpp` (`XcorrScore`, `XcorrScoreI`) | Every bin lookup becomes a branch: `ppf[x][y]` vs `lower_bound(indices, bin)` |
| `CometPostAnalysis.cpp` | `FindSpScore` and the E-value scoring loop both read the sparse matrices |
| `CometSpecLib.cpp` | Spectral library scoring reads `ppfSparseFastXcorrData` |
| `CometPreprocess.cpp` (`RtsScratch`) | Pool allocator is sized for `SPARSE_MATRIX_SIZE` leaf blocks; COO path would not use it |

At minimum, every read site (scoring inner loop) would add a branch on `bUseCOO`. The
scoring inner loop runs once per theoretical ion per peptide candidate — it is the single
hottest loop in the search engine. Adding a branch there, even a predictable one, risks
instruction cache pressure and complicates future SIMD vectorization.

### Cost/Benefit Assessment

| Factor | Assessment |
|---|---|
| Memory saving at 0.01 Da (3 matrices) | ~100 KB per Query |
| Memory saving across 10,000-spectrum batch | ~1 GB |
| Scoring performance penalty (COO path) | ~15× slower XcorrScore; effective only at bin sizes where it never wins |
| Code complexity increase | ~6 files, ~4 new read paths, new data structure, new pool design |
| Test surface expansion | All bin sizes × all search modes × RTS vs batch |

The memory savings at 0.01 Da are real but achievable through simpler means:
- **Reduce `spectrum_batch_size`** to process fewer spectra concurrently.
- **Tune `SPARSE_MATRIX_SIZE`** dynamically at startup (single compile-time constant becomes
  a global set once in `CometSearchManager` before any preprocessing runs); this shrinks the
  outer array without touching any read path.

**Conclusion: dual data structure support is not recommended.** The performance disadvantage
of COO is fundamental (15× per-lookup cost), the memory advantage is narrow (~4–9% at 0.01
Da), and the implementation scope is large. The current chunked format remains the correct
choice across all practical fragment bin sizes.

---

## 7. Recommendations

### Immediate (no code change required)

The current `SPARSE_MATRIX_SIZE = 100` is a sound default for all bin sizes from 1.0005 Da
down to 0.01 Da. No change is needed.

### Optional: Runtime-adaptive SPARSE_MATRIX_SIZE

For deployments focused exclusively on high-resolution (fine bin) or low-resolution (coarse
bin) searches, a startup-time value computed from `dFragmentBinSize` would improve outer array
cache behavior without touching any read path:

```cpp
// In CometSearchManager, after dFragmentBinSize is finalized:
// Pick the smallest power of 2 in [20, 512] that keeps the outer array in L1.
// Fall back to 100 if no power-of-2 satisfies the constraint.
static const int TARGET_OUTER_ENTRIES = 256;  // 256 × 8 = 2 KB
int iOptS = 1;
while (iOptS < g_staticParams.iArraySizeGlobal / TARGET_OUTER_ENTRIES)
    iOptS <<= 1;
g_iSparseMatrixSize = std::clamp(iOptS, 32, 512);
```

This keeps the outer array at ~2 KB (L1-resident) for all bin sizes ≥ 0.05 Da with no leaf
memory penalty. For 0.02 Da and 0.01 Da, the constraint cannot be satisfied without
sacrificing fill, so the value clamps to 512 (partial benefit) or defaults to 100.

This optimization is **low risk** (no read path changes, single constant becomes a global) and
**low priority** (outer array latency is amortized across thousands of peptides per spectrum).

### Do Not Implement

- COO / binary search as an alternative sparse format: performance regression of ~15× in the
  XcorrScore inner loop; memory savings insufficient to justify the implementation cost.
- `SPARSE_MATRIX_SIZE = 16` universally: correct for 1 Da bins in isolation but increases the
  outer array to 250 KB at 0.01 Da, placing it firmly in L3.
