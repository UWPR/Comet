# RTS E-value jitter reproducer

Phase 2 of `docs/20260714_EvalueJitter.md`. A small, Thermo-independent, Linux-buildable driver
that reproduces the RTS E-value/near-tie jitter documented there, without needing the full
`RealtimeSearch.exe` / C++/CLI / Thermo RawFileReader stack.

`rts_repro.cpp` links directly against `libcometsearch` and calls the same
`ICometSearchManager` API `RealtimeSearch/SearchMS1MS2.cs` calls via `CometWrapper.dll`
(`InitializeSingleSpectrumSearch()` + `DoSingleSpectrumSearchMultiResults()`), against a fixed set
of 197 real spectra (`fixture_spectra.txt`, extracted once from `20250520_Hela_60min_06.raw` --
see "Regenerating the fixture" below), from N worker threads pulling off a shared work queue --
the same concurrency pattern the C# harness uses.

Parameter setup mirrors `SearchSettings.ConfigureInputSettings()`'s "index already exists" branch
exactly (RTS never reads `comet.params` -- see that C# method for the authoritative list of what it
actually sets). If that method changes, update `rts_repro.cpp`'s param block to match.

## Build

```bash
cd /mnt/c/Work/Comet-master
g++ -O2 -std=c++20 -fpermissive -Wno-write-strings -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -D_NOSQLITE -D__int64=off64_t \
  -ICometSearch -IMSToolkit/include -IMSToolkit/extern/expat-2.2.9/lib -IMSToolkit/extern/zlib-1.2.11 -IAScorePro/include \
  tests/rts_repro/rts_repro.cpp -o tests/rts_repro/rts_repro \
  -LMSToolkit -LCometSearch -LAScorePro -lcometsearch -lmstoolkit -lmstoolkitextern -lascorepro -lm -lpthread
```

Requires `libcometsearch.a`/`libmstoolkit.a`/`libmstoolkitextern.a`/`libascorepro.a` already built
(`make` from the repo root builds all of these).

## Run

Needs a PI_DB `.idx` built from `data/human.small.fasta` (the fixture spectra's precursor
masses/charges were extracted against this exact database, so real candidate matches occur):

```bash
mkdir -p /tmp/rts_repro_work && cd /tmp/rts_repro_work
cp /mnt/c/Work/Comet-master/data/human.small.fasta .
cp /mnt/c/Work/Comet-master/data/comet_phospho.params comet.params
/mnt/c/Work/Comet-master/comet.exe -Dhuman.small.fasta -j        # builds human.small.fasta.idx (PI_DB)

/mnt/c/Work/Comet-master/tests/rts_repro/rts_repro human.small.fasta.idx \
   /mnt/c/Work/Comet-master/tests/rts_repro/fixture_spectra.txt 8 run1.out 1
```

Args: `<database.idx> <fixture_file> <num_threads> <output_file> [ascorepro:0|1]`.

Output lines are unordered (worker threads finish in whatever order they finish) -- sort by scan
number before diffing two runs:

```bash
sort -k2,2n run1.out > run1.sorted.out
sort -k2,2n run2.out > run2.sorted.out
diff run1.sorted.out run2.sorted.out
```

## Expected result

**As of the Phase 5 fix (see `docs/20260714_EvalueJitter.md`), both `num_threads=1` and
`num_threads=8` are byte-identical across reruns** -- 0 differing lines in every pairwise
comparison, matching RTS's now-fixed determinism. This reproducer was built and used *during* the
investigation, when `num_threads=8` still showed a small number of the 197 spectra (observed 3-6
per pairwise comparison, ~1.5-3%) differing between runs -- either the same peptide with a different
E-value, or (rarer) a different winning peptide on a near-tied XCorr, matching the class and rough
rate of jitter documented in `docs/20260714_EvalueJitter.md`'s full-harness bisection table (there:
~0.5-0.7% of ~44,700 spectra; higher here because `fixture_spectra.txt` intentionally over-samples
scans already known to have shown jitter). One specific case reproduced exactly during that phase:
scan 25030 flipping between `R.AYAETSKMK.V` and `R.SVSSSSYRR.M`, the identical flip (same two
peptides, same scan) documented in `docs/20260714_rtspostprocessing.md`'s original investigation.
Kept as a regression test for this bug class -- if this reproducer ever shows non-zero jitter again,
start with `docs/20260714_EvalueJitter.md`'s Phase 3 methodology (full XCorr histogram dump,
compare content not just count) rather than re-deriving it from scratch.

## Regenerating the fixture

`fixture_spectra.txt` was generated once via a temporary, since-reverted dump added to
`RealtimeSearch/SearchMS1MS2.cs` (env-var gated, `RTS_DUMP_FIXTURES=1`), run against
`data/20250520_Hela_60min_06.raw` on Windows. It captures 197 spectra: a fixed list of scan numbers
already known (from earlier full-harness bisection) to show jitter, plus a modulo-300 sample across
the full scan range for general coverage. See git history for `RealtimeSearch/SearchMS1MS2.cs`
around the Phase 2 commit if this needs to be regenerated (e.g. against a different raw file or
with a larger sample) -- the dump logic itself was not kept in the shipped harness, only the
resulting fixture file.

Format: repeated blocks of

```
SPECTRUM <scanNumber> <charge> <precursorMz> <numPeaks>
<mass1> <intensity1>
<mass2> <intensity2>
...
```
