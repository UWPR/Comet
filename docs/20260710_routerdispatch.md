# Router/dispatch layer for fixed multi-parameter-set RTS

Date: 2026-07-10
Status: proposed
Scope: new C# components only, under `RealtimeSearch/`. No changes to `CometSearch/`,
`CometWrapper/`, or any native code.
Builds on: `docs/20260615_multiple_rts_instances.md`, Option A ("multiple processes").
That doc explains why process-level isolation is required at all -- `g_staticParams`,
the fragment index, and related globals are process-wide in `CometSearch`, so two
`CometSearchManagerWrapper` objects in one process cannot hold independent parameter
sets. This doc designs the layer that sits above N such processes.

## 1. Goal

Within a single logical RTS run, route different subsets of spectra to different,
independently-configured parameter sets (different enzyme, mods, indexed database,
tolerances, etc.), where:

- The number of parameter sets (N) is small and fixed for the duration of the run,
  decided at startup -- not added or removed dynamically.
- Each parameter set is long-lived: its fragment index is built once
  (`InitializeSingleSpectrumSearch`) and stays resident for the whole run.
- The routing decision (which spectrum goes to which parameter set) is made
  per-spectrum, by a pluggable component, not hardcoded into any one worker.

## 2. Non-goals

- Dynamic add/remove of parameter sets mid-run. If needed later this is a bigger
  change (worker pool with lazy spawn/teardown) -- deferred.
- Reducing per-process memory via a shared database layer (Option C in the
  referenced doc). Each worker pays the full fragment-index memory cost; acceptable
  because N is small and fixed.
- Mid-run worker crash recovery. A worker dying is treated as fatal to the run,
  matching today's single-process behavior on an unhandled exception.
- Prescribing the actual routing predicate (scan range vs. method segment vs.
  explicit table, etc.) -- application-specific, left pluggable (Section 4d).

## 3. Architecture overview

```text
                     +----------------------------------+
                     |   RtsDispatcher (coordinator)     |
                     |   - reads spectra from RAW file    |
                     |   - ISpectrumRouter picks a worker |
                     |   - sends request over named pipe   |
                     |   - collects + orders results       |
                     +----+----------+----------+----------+
                          |          |          |
                    named pipe   named pipe   named pipe
                    "pset_A"     "pset_B"     "pset_C"
                          |          |          |
                     +----v---+ +----v---+ +----v---+
                     | Worker | | Worker | | Worker |
                     | pset_A | | pset_B | | pset_C |
                     | (own   | | (own   | | (own   |
                     |  proc) | |  proc) | |  proc) |
                     +--------+ +--------+ +--------+
```

Each worker is today's single-parameter-set RTS process (`CometSearchManagerWrapper`
+ `InitializeSingleSpectrumSearch` + internal search threads, i.e. the existing logic
in `RealtimeSearch/SearchMS1MS2.cs`), unchanged, except that spectra arrive over a
pipe instead of being read directly from a RAW file. The dispatcher is new.

Important consequence of this split: only the **worker** processes need to link
`CometWrapper.dll` / call into native code. The **dispatcher** only needs the Thermo
RawFileReader NuGet package (to read the source RAW file) plus the wire protocol --
it never touches `CometSearchManagerWrapper`. That keeps the coordinator small and
lets it evolve independently of the native search internals.

## 4. Components

### 4a. Worker host -- `RealtimeSearch/RtsWorkerHost.cs` (new)

A new entry point that reuses the exact configuration/search calls already in
`SearchMS1MS2.cs`, but serves requests from a pipe instead of looping over RAW scans
itself:

```csharp
class RtsWorkerHost
{
   static void Main(string[] args)
   {
      // args[0] = pipe base name for this parameter set, e.g. "pset_A"
      // args[1] = path to this worker's parameter-set config (Section 6)
      string pipeName = args[0];
      string configPath = args[1];

      var searchMgr = new CometSearchManagerWrapper();
      ParamSetConfig.Apply(searchMgr, configPath);   // Section 4e

      searchMgr.InitializeSingleSpectrumSearch();
      // searchMgr.InitializeSingleSpectrumMS1Search(dMaxQueryRT); -- if MS1 needed too

      SignalReady(pipeName);   // Section 5, startup handshake

      int numThreads = ParamSetConfig.NumThreads(configPath);
      var serverTasks = new Task[numThreads];
      for (int i = 0; i < numThreads; ++i)
         serverTasks[i] = Task.Run(() => ServeRequests(pipeName, searchMgr));
      Task.WaitAll(serverTasks);

      searchMgr.FinalizeSingleSpectrumSearch();
   }

   static void ServeRequests(string pipeName, CometSearchManagerWrapper searchMgr)
   {
      while (true)
      {
         using (var pipe = new NamedPipeServerStream(pipeName, PipeDirection.InOut,
                   -1, PipeTransmissionMode.Byte, PipeOptions.Asynchronous))
         {
            pipe.WaitForConnection();
            SpectrumRequest req = SpectrumRequest.ReadFrom(pipe);
            if (req.IsShutdown)
               return;

            searchMgr.DoSingleSpectrumSearchMultiResults(req.TopN, req.PrecursorCharge,
               req.MZ, req.Masses, req.Intensities, req.NumPeaks,
               out List<string> peptides, out List<string> proteins,
               out List<List<FragmentWrapper>> fragments, out List<ScoreWrapper> scores);

            SpectrumResponse.From(req.ScanNumber, peptides, proteins, scores).WriteTo(pipe);
         }
      }
   }
}
```

Multiple `NamedPipeServerStream` instances on the same pipe name give this worker its
own internal fan-out (`numThreads`), exactly mirroring the `Task.Run` fan-out already
in `SearchMS1MS2.cs`'s `ProcessScans` loop -- that part of the threading model is
unchanged, just moved behind a pipe instead of a shared `ConcurrentQueue<int>` of scan
numbers.

`DoSingleSpectrumSearchMultiResults` is called with the same signature as today
(`CometWrapper/CometWrapper.h:45-54`); nothing about the native call changes.

### 4b. Wire protocol

Plain length-prefixed binary framing over the pipe (`BinaryWriter`/`BinaryReader`),
no external serialization dependency -- consistent with the hand-rolled marshaling
already used between `CometWrapper` and `CometSearchManager`.

`SpectrumRequest`:

| Field | Type | Notes |
|---|---|---|
| `IsShutdown` | `bool` | if true, worker exits its serve loop after reading this |
| `ScanNumber` | `int` | echoed back in the response for reordering |
| `TopN` | `int` | |
| `PrecursorCharge` | `int` | |
| `MZ` | `double` | |
| `NumPeaks` | `int` | |
| `Masses` | `double[NumPeaks]` | |
| `Intensities` | `double[NumPeaks]` | |

`SpectrumResponse`:

| Field | Type | Notes |
|---|---|---|
| `ScanNumber` | `int` | |
| `Peptides` | `string[]` | |
| `Proteins` | `string[]` | |
| `Scores` | flattened `ScoreWrapper` fields (`xCorr`, `dExpect`, `mass`, `dAScorePro`, `sAScoreProSiteScores`, ...) | plain primitives, not the native wrapper type -- the dispatcher does not reference `CometWrapper.dll` |

`FragmentWrapper` detail (matched fragment ions) is omitted from the response by
default to keep frames small; add it as an optional field if the dispatcher's output
report needs it.

### 4c. Dispatcher -- `RealtimeSearch/RtsDispatcher.cs` (new)

Replaces the body of today's `SearchMS1MS2.cs` `Main`: same RAW-file scan loop,
same `ConcurrentBag<ScanResult>` + `OrderBy(ScanNumber)` result assembly and the
same histogram/report output at the end, but instead of calling
`globalSearchMgr.DoSingleSpectrumSearchMultiResults(...)` directly, it:

1. Reads the spectrum (mass/intensity arrays) from the RAW file exactly as
   `ProcessScans` does today (`SearchMS1MS2.cs:186-330`).
2. Asks `ISpectrumRouter` which parameter set this scan belongs to.
3. Gets a pooled pipe client connection for that parameter set (Section 4c-i).
4. Sends a `SpectrumRequest`, blocks for the `SpectrumResponse`.
5. Feeds the response into the same `ScanResult`/`results` bag used today.

```csharp
class WorkerConnection   // one live NamedPipeClientStream + its assigned parameter set
{
   public string ParamSetName;
   public NamedPipeClientStream Pipe;
   public SpectrumResponse Send(SpectrumRequest req) { ... }
}

class WorkerConnectionPool   // one pool per parameter set, sized to that worker's numThreads
{
   BlockingCollection<WorkerConnection> _free;
   public WorkerConnection Rent() => _free.Take();
   public void Return(WorkerConnection c) => _free.Add(c);
}
```

Sizing each pool to the target worker's own configured `numThreads` bounds
in-flight requests per worker to that worker's real internal parallelism, so the
dispatcher never queues more work at a worker than it can run concurrently.

### 4d. Router -- pluggable, `RealtimeSearch/ISpectrumRouter.cs` (new)

```csharp
public interface ISpectrumRouter
{
   // Returns a parameter-set name matching one of the configured worker pool keys.
   string Route(int scanNumber, MSOrderType scanType, double precursorMz, double rt);
}
```

The routing criterion itself (scan-number range, method segment, an explicit
scan-to-parameter-set table, precursor m/z window, etc.) is not prescribed here --
it is application-specific and swappable behind this one-method interface. Ship a
couple of trivial reference implementations (`ExplicitScanTableRouter`,
`ScanRangeRouter`) so the plumbing can be exercised without committing to one
scheme.

### 4e. Parameter-set configuration -- `RealtimeSearch/ParamSetConfig.cs` (new)

Gap found while grounding this plan: `ICometSearchManager` has no method to load a
`comet.params` file directly (only `CometSearchManagerWrapper.SetParam(name, str,
value)` per-field calls -- confirmed by grep across `CometSearchManager.h` /
`CometInterfaces.h`). Today's `SearchSettings.ConfigureInputSettings` in
`SearchMS1MS2.cs:511-781` is a hardcoded sequence of ~30 `SetParam` calls.

For N parameter sets, hand-writing a C# method per set does not scale. Recommend
extracting `ConfigureInputSettings` into a generic key/value-driven translator:

```csharp
static class ParamSetConfig
{
   // Reads a simple "key value" text file (comet.params-like syntax already
   // used elsewhere in this repo) and issues the matching SetParam call per key,
   // using the same key names ConfigureInputSettings already hardcodes today.
   public static void Apply(CometSearchManagerWrapper mgr, string configPath) { ... }
   public static int NumThreads(string configPath) { ... }
}
```

This is the one piece of net-new *logic* (as opposed to plumbing) in this plan --
everything else is IPC and routing around calls that already exist. `SearchMS1MS2.cs`
itself can be refactored to call the same `ParamSetConfig.Apply` for its single
parameter set, removing the duplication rather than adding a second copy.

## 5. Lifecycle

**Startup.** Dispatcher spawns all N worker processes in parallel
(`Process.Start`, one `RtsWorkerHost` per parameter set) so index builds overlap --
`InitializeSingleSpectrumSearch` is the expensive step and there is no reason to
serialize it across workers. Each worker signals readiness on a `pipeName + "_ready"`
pipe once its index build completes; the dispatcher waits for all N ready signals
before reading the first spectrum. This bounds startup latency to the slowest single
worker's index build, not the sum.

**Steady state.** Dispatcher's scan loop is unchanged in shape from today's
`ProcessScans` (`SearchMS1MS2.cs:186-330`); only the search call is replaced with
router-pick + pooled-pipe round trip. Result ordering is unaffected -- `sortedResults
= results.OrderBy(r => r.ScanNumber)` (`SearchMS1MS2.cs:361`) already tolerates
out-of-order completion, which is exactly what multiple workers with independent
queues produce.

**Shutdown.** Dispatcher sends one `SpectrumRequest{IsShutdown=true}` per open pipe
per worker (matching that worker's `numThreads`, since each `ServeRequests` loop
exits independently) after the RAW file scan loop completes, then waits for each
worker process to exit before writing the final report -- mirrors today's `finally`
block that calls `FinalizeSingleSpectrumSearch()` (`SearchMS1MS2.cs:351-358`), just
triggered remotely instead of in-process.

**Failure handling.** If a worker process exits unexpectedly (pipe broken /
`Process.HasExited`), the dispatcher treats the whole run as failed: log which
parameter set died and on which scan, then abort rather than attempt mid-run
recovery (see Non-goals). This matches current single-process behavior, where an
unhandled exception in `ProcessScans` already terminates the run.

## 6. Config format sketch

One manifest file for the dispatcher listing the N parameter sets and where to find
each one's settings:

```json
{
  "parameterSets": [
    { "name": "pset_A", "pipeName": "comet_pset_A", "configFile": "pset_A.params", "numThreads": 8 },
    { "name": "pset_B", "pipeName": "comet_pset_B", "configFile": "pset_B.params", "numThreads": 8 }
  ],
  "router": { "type": "explicit-scan-table", "table": "routing.tsv" }
}
```

`configFile` is the input to `ParamSetConfig.Apply` (Section 4e); `router.type`
selects which `ISpectrumRouter` implementation to instantiate.

## 7. Threading / throughput model

Total system parallelism is the sum of each worker's own `numThreads`, same as
today's per-process model just partitioned across N processes instead of one. The
dispatcher's own thread usage is bounded by the sum of the `WorkerConnectionPool`
sizes, so it never over-subscribes a worker beyond what that worker's `numThreads`
was configured to handle. No change to the actual scoring throughput per parameter
set -- the added cost per spectrum is one pipe round trip (local-machine named pipe,
sub-millisecond) on top of existing search time, which is >=1 ms per
`docs/20260709_sprankjitter.md`-era RTS latencies.

## 8. Validation plan

- **Plumbing correctness with N=1**: run the dispatcher against a single parameter
  set and diff its `rts.out` byte-for-byte against today's single-process
  `SearchMS1MS2.cs` run on the same RAW file. Any difference indicates a bug in the
  new IPC path, not in scoring (scoring code is untouched).
- **Routing correctness with N=2**: two tiny worker processes over
  `tests/unit/data/` fixtures, an `ExplicitScanTableRouter` with a known scan ->
  parameter-set mapping, assert each scan's result came from the expected parameter
  set (e.g. by giving each worker deliberately different, distinguishable params such
  as different `equal_I_and_L` settings and checking which peptide interpretation
  came back).
- **Failure path**: kill a worker process mid-run, assert the dispatcher aborts with
  a clear error identifying the parameter set and scan, rather than hanging on a
  broken pipe or silently dropping results.
- **Existing suite unaffected**: `python tests/unit/run_tests.py --comet
  /mnt/c/Work/Comet-master/comet.exe` should be unaffected since no native code
  changes -- run as a regression gate anyway.

## 9. Migration path

This is Option A from `docs/20260615_multiple_rts_instances.md`, so it inherits that
doc's stated tradeoff: N times the fragment-index memory footprint. If memory
pressure or per-spectrum pipe latency later become the binding constraint, the
recommended next step is that doc's Option B (per-instance `RtsContext`, single
process, no IPC) -- the dispatcher/router split designed here would still apply
almost unchanged, since `ISpectrumRouter` and the request/response shapes are
IPC-transport-agnostic; only `WorkerConnection` would change from a named pipe to a
direct in-process call into a second `CometSearchManager` instance.

## 10. Open questions

- Exact routing criterion (left pluggable here; needs a decision before
  `ISpectrumRouter`'s first concrete implementation is written).
- Named pipe naming/security scope if the dispatcher and workers ever run under
  different OS user accounts (not expected for a single-machine RTS deployment, but
  worth confirming before implementation).
- Whether `FragmentWrapper` (matched fragment ions) needs to cross the wire for this
  use case, or whether only `ScoreWrapper` + peptide/protein strings are needed --
  affects `SpectrumResponse` size and worth pinning down before Section 4b is
  finalized.
