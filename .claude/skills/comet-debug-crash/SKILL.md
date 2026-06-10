---
name: comet-debug-crash
description: Debug crashes, silent exits, or wrong results in Comet. Use when a search dies unexpectedly, produces no output, crashes during index build or search, or gives suspicious results.
---

# Comet Debug Crash

## Identifying the crash phase

The progress output tells you exactly where it died. Match against these milestones:

| Last output seen | Likely location |
|-----------------|----------------|
| `* read .idx ...` | `ReadFragmentIndex()` in `CometFragmentIndex.cpp` |
| `* generate fragment ion index` | `CreateFragmentIndex()` start |
| `* store peptide list and reserve memory ...` | `AddFragmentsThreadProc(bCountOnly=1)` or CSR prefix-sum |
| `* sort peptides by mass ...` | `std::sort` on `g_vFragmentPeptides` |
| `* populate index ...` | `AddFragments()` write pass — **most common crash site** |
| `searching "..."` starts but hangs | `RunSearch` / `AcquirePoolSlot` deadlock, or OOM during search |

## Known crash: uint64 overflow in CSR prefix-sum (FIXED 2026-05-04)

**Symptom:** Dies silently immediately after `populate index ...` is printed.

**Cause:** Non-enzymatic or very broad searches generate >4.29 billion total fragment index entries, overflowing the old `unsigned int` accumulator in `CreateFragmentIndex()`. The underallocated `g_iFragmentIndex` array causes an out-of-bounds write → SIGSEGV.

**Fix applied:** `g_iFragmentIndexOffset` and `s_iWritePos` changed from `unsigned int*` to `uint64_t*`; `uiBinBase` in `CometSearch.cpp` changed to `uint64_t`. Verify the fix is present before chasing this as a new bug.

**Trigger conditions:** non-enzymatic search (`enzyme_number = 0`), peptide length range wider than ~10-30, large database (human canonical or larger).

## Known crash: g_vFragmentPeptides too large for unsigned int (guarded)

**Symptom:** Exception with message `Error: g_vFragmentPeptides.size() too large for unsigned int`.

**Location:** `CometFragmentIndex.cpp` line ~203, just before the populate loop.

**Meaning:** The number of distinct (peptide × mod combination) entries exceeds UINT_MAX. This is a hard limit because fragment index entries store peptide indices as `unsigned int`. Reduce search space.

## Instrumentation: adding a temporary printf

To narrow down exactly which line in a function is crashing:

```cpp
printf("checkpoint A\n"); fflush(stdout);
// ... suspected code ...
printf("checkpoint B\n"); fflush(stdout);
```

`fflush(stdout)` is critical — without it, buffered output may not appear before the crash.

After finding the line, remove the printfs before committing.

## RTS_TIMING for per-phase C++ timing

Enable in `CometSearch\CometSearch.vcxproj` Release preprocessor: `RTS_TIMING_OFF` → `RTS_TIMING`.

Adds a tab-separated line per spectrum to stdout:
```
TIMING  <mz>  <charge>  <preprocess_us>  <runsearch_us>  <sort_us>  <calcSP_us>  <calcEvalue_us>  <calcDeltaCn_us>  <calcAScore_us>  <results_us>  <total_us>
```

Capture to file and average columns with PowerShell:
```powershell
.\RealtimeSearch.exe ... 2>&1 | Select-String "^TIMING" | ForEach-Object {
    $cols = $_.Line.TrimEnd().Split("`t")
    # cols[2]=preprocess, [3]=runsearch, [9]=results(protein lookup), [10]=total
} | ...
```

**Warning:** stdout contention from 20 threads each printing per-spectrum adds ~8s to search time. Disable before any throughput comparison.

## Memory issues

If the process dies with no output at all (not even a crash message), suspect OOM:
- Check available RAM: `Get-WmiObject Win32_OperatingSystem | Select FreePhysicalMemory`
- For non-enzymatic human search, `g_iFragmentIndex` can require 25–40 GB
- WSL has a memory cap (check `/proc/meminfo` or `.wslconfig`)

## Checking for silent integer overflow

Before assuming a logic bug, verify all size-related variables near the crash:
- Anything accumulating counts in a loop: should be `uint64_t` or `size_t`, not `int` or `unsigned int`
- Array indices into large flat arrays: must be 64-bit if the array can exceed 4 billion elements
- `new T[n]`: if `n` came from an `unsigned int` accumulator that could overflow, the allocation size is wrong

## Thread sanitizer / address sanitizer (Linux)

For Linux builds, add to `Makefile` CFLAGS: `-fsanitize=address` to catch out-of-bounds writes and use-after-free at the exact source line. ASAN will report the crash with a full stack trace instead of a silent SIGSEGV.
