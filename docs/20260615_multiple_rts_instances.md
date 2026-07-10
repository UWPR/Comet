# Multiple Concurrent RTS Instances: Design Options

**Goal:** Allow N concurrent RTS instances in the same host process (or across processes), each running an independent set of search parameters, so that different subsets of spectra can be searched with different parameter sets simultaneously.

---

## The Core Challenge

All state that makes one search parameterization distinct from another is currently a process-wide singleton. There are two categories:

**Must be per-instance (encode the parameter set):**
- `g_staticParams` -- the parameter root
- `g_iFragmentIndex` / `g_iFragmentIndexOffset` / `g_vFragmentPeptides` -- the index encodes enzyme cleavage, variable mods, and peptide length range; different params -> different index
- `MOD_NUMBERS` / `MOD_SEQS` / `PEPTIDE_MOD_SEQ_IDXS` -- mod permutation tables built from `variableModParameters`
- `CometFragmentIndex::_pbSearchMemoryPool` / `CometSearch::_ppbDuplFragmentArr` -- pool sized to param-set's thread count (2026-07-10: corrected class attribution -- _pbSearchMemoryPool is a CometFragmentIndex static, not CometSearch)
- `g_AScoreOptions` / `g_AScoreInterface` -- if AScore settings differ
- `g_cometStatus` -- each instance needs independent error/cancel state
- All init flags and `singleSearchInitializationComplete`

**Potentially shared (encode the database, not the params):**
- `g_vRawPeptides` (~300 MB) -- plain peptide sequences from the `.idx` file
- `g_pvProteinsList` (~200 MB CSR) -- protein file offsets per peptide
- `g_pvProteinNameCache` (~7 MB) -- protein name strings
- `g_pvProteinNames` -- indexed protein accessions
- `g_vSpecLib` / `g_vulSpecLibPrecursorIndex` -- if all instances use the same MS1 reference

---

## Option A: Multiple processes

Run N separate instances of the host application (or N `RealtimeSearch.exe` processes). Each process has its own address space and therefore its own independent copy of all globals. A C# coordinator routes spectra to the right process and aggregates results.

**Zero C++ changes required.** Works today.

**Pros:** Complete isolation, no lock contention between instances, simplest reasoning about state.

**Cons:** N x full memory footprint per process. For a human target-decoy `.idx`, the fragment index alone is 3-8 GB; three instances = 9-24 GB just for the index. IPC cost for routing spectra and collecting results across process boundaries.

**When to choose:** If memory is not constrained, or if the parameter sets are infrequently changed and process startup latency is acceptable.

---

## Option B: Per-instance context struct (recommended long-term path)

Move all process-global state into a `SearchContext` struct owned by each `CometSearchManager` instance. Multiple `CometSearchManager` objects can then coexist in the same process with fully independent state.

```cpp
// New: CometSearch/RtsContext.h
struct RtsContext {
    StaticParams                                  params;
    unsigned int*                                 iFragmentIndex       = nullptr;
    uint64_t*                                     iFragmentIndexOffset = nullptr;
    vector<FragmentPeptidesStruct>                vFragmentPeptides;
    vector<PlainPeptideIndexStruct>               vRawPeptides;
    ProteinsListCSR                               pvProteinsList;
    unordered_map<comet_fileoffset_t, string>     pvProteinNameCache;
    map<long long, IndexProteinStruct>            pvProteinNames;
    bool*                                         bIndexPrecursors     = nullptr;
    vector<SpecLibStruct>                         vSpecLib;
    vector<vector<unsigned int>>                  vulSpecLibPrecursorIndex;
    AScoreProCpp::AScoreOptions                   AScoreOptions;
    AScoreProCpp::AScoreDllInterface*             pAScoreInterface     = nullptr;
    vector<ModificationNumber>                    MOD_NUMBERS;
    vector<string>                                MOD_SEQS;
    // ... mod index arrays ...
    bool*                                         pbSearchMemoryPool   = nullptr;
    bool**                                        ppbDuplFragmentArr   = nullptr;
    CometStatus                                   status;
    // init flags are already members of CometSearchManager
};
```

`CometSearchManager` holds a `unique_ptr<RtsContext>`. Every internal function that currently reads `g_staticParams` receives a `const RtsContext&` (or `const StaticParams&`) instead. The `CometFragmentIndex`/`CometSearch` class static members `_pbSearchMemoryPool` / `_ppbDuplFragmentArr` become per-instance (either stored in `RtsContext` and passed in, or `CometSearch` becomes a non-static class).

The C# side creates N `CometSearchManagerWrapper` objects -- a natural extension of what is already there. Each wrapper wraps one `CometSearchManager` which owns one `RtsContext`. Spectra are routed to the appropriate wrapper by the C# coordinator.

**Pros:** Single process, low IPC overhead, easy result aggregation, no process-startup latency per instance. Full clean encapsulation -- no global state at all after the refactor.

**Cons:** Memory cost is the same as multi-process (N x index size). The refactor touches ~15 `.cpp`/`.h` files everywhere `g_staticParams`, `g_iFragmentIndex`, etc. are referenced. It is mechanical but not small -- `g_staticParams` alone appears in roughly 30 call sites in `CometSearch.cpp`.

**Scope estimate:** The most invasive change is threading `const RtsContext&` (or just `const StaticParams&` for the scoring-only functions) through the call chains in `CometSearch.cpp`, `CometPreprocess.cpp`, `CometPostAnalysis.cpp`, `CometFragmentIndex.cpp`. A staged approach works: start with `g_staticParams` (referenced everywhere), get that building cleanly, then migrate the index arrays.

---

## Option C: Shared-database layer + per-param search layer

Split `RtsContext` into two levels:

```cpp
struct DatabaseContext {            // shared via shared_ptr
    vector<PlainPeptideIndexStruct>            vRawPeptides;
    ProteinsListCSR                            pvProteinsList;
    unordered_map<comet_fileoffset_t, string>  pvProteinNameCache;
    map<long long, IndexProteinStruct>         pvProteinNames;
};

struct SearchContext {              // per-instance
    StaticParams                              params;
    shared_ptr<const DatabaseContext>         db;   // shared
    unsigned int*                             iFragmentIndex;
    uint64_t*                                 iFragmentIndexOffset;
    vector<FragmentPeptidesStruct>            vFragmentPeptides;
    // ... pools, mod tables, AScore, status ...
};
```

The `DatabaseContext` is loaded once from the `.idx` file (which encodes protein names and peptide sequences regardless of search params) and shared among all `SearchContext` instances that reference the same file.

**Memory savings:** ~500 MB per extra instance on the sharable data. The fragment index still cannot be shared when mods or enzyme differ -- and at 3-8 GB that is the dominant cost. Savings are typically 10-20% for a human proteome use case with three instances.

**Pros:** Meaningful memory reduction if N is large or the base database is very large.

**Cons:** Added complexity (two-level ownership, `shared_ptr` threading, database-identity matching). Benefit is modest when the index itself is not shared.

**When to choose:** If all N instances use the same `.idx` file (guaranteed same database) AND memory is tight enough that 500 MB x N matters.

---

## Special case: same database, same mods, different scoring params only

If the only differences between instances are tolerance, ion series (a/b/c/x/y/z), minimum score, or similar scoring-time parameters -- things that do not affect which peptides are in the index -- then the entire fragment index (`g_iFragmentIndex`, `g_vFragmentPeptides`, `g_vRawPeptides`) is the same for all instances and can be shared. Only `g_staticParams` truly differs.

In this case Option C degenerates to: share everything except `StaticParams` and the memory pool. The C++ scoring functions would receive a `const StaticParams&` argument instead of reading `g_staticParams` directly, which is a much smaller change than the full Option B refactor.

---

## Recommendation

| Timeframe | Choice | Reason |
|-----------|--------|--------|
| Immediately | **Option A** (multiple processes) | Zero C++ changes, works today, C# coordinator routes spectra |
| Long term | **Option B** (per-instance context) | Clean encapsulation, single process, natural extension of C# API, enables future optimizations including Option C |

The key question that should drive which option is prioritized: **do the N param sets use the same `.idx` database file?** If yes and memory is a concern, the staging would be: Option B first, then selectively apply Option C's `shared_ptr<DatabaseContext>` for the large read-only arrays as an optimization on top.
