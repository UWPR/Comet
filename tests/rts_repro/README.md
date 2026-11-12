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

Parameter setup follows `SearchSettings.ConfigureInputSettings()`'s "index already exists" branch
in `RealtimeSearch/SearchMS1MS2.cs` (RTS never reads `comet.params`). That harness is an example
program whose settings change as RTS is tried in different configurations, so `rts_repro.cpp`'s
defaults are a snapshot of it, last aligned on 2026-11-08 (`isotope_error` 2, `max_index_runtime`
500 ms), not a contract:

- To reproduce a particular harness run, set the values that differ with `RTS_REPRO_PARAMS`.
- Test T80 compares the two parameter blocks and prints any differences as notes; it does not fail
  on drift.
- The unit tests never rely on these defaults: `run_tests.py` passes the parameters its RTS tests
  depend on (`RTS_TEST_PARAMS`: `isotope_error` 2, `max_index_runtime` 0), and a test that needs
  other values sets them itself.

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

Args: `<database.idx> <fixture_file> <num_threads> <output_file> [ascorepro:0|1] [index_search_type:0=PI_DB|1=FI_DB]`,
then optionally the chimeric-search settings (`docs/20261105_RTSchimeric.md`):
`[chimeric:0|1|2] [zmin] [zmax] [tolMZ] [maxExpect] [maxPeptides] [flankBins] [isotopes] [isotopeError] [globalMinIonsScore] [histogramFill] [useNLIons]`.
`flankBins` is ignored, with a note on stderr when it is not -1: since 2026-11-08 the removal
flank follows `theoretical_fragment_ions`. The slot stays so the later arguments keep their
positions. `isotopes` sets `chimeric_remove_isotopes` (default 2, clamped to 0..3).
`chimeric` 1 searches every `SPECTRUM` block at charges `zmin..zmax` with a +/- `tolMZ` window
(`DoSingleSpectrumSearchChimeric`'s charge-range form); 2 treats every block sharing a scan
number as one `(m/z, charge)` candidate of a single spectrum (the explicit-list form); 0 is
this reproducer's original single-search behaviour and output. In the chimeric modes the
output has one line per reported peptide (`iter`, `z`, peptide, xcorr, dCn, E-value, mass,
ions, protein), `max_index_runtime` is set to 0, and `RTS_CHIMERIC_DEBUG=1|2` in the
environment traces each iteration (1) plus, per candidate search, its real-candidate count
and, per kept PSM, its matched and removed bins (2) on stderr; `COMET_EVALUE_TRACE=1|2`
(any mode, any search path) adds each search's E-value fit and, at 2, its xcorr histogram
before and after the synthetic padding (run single-threaded to keep one call's lines
together; `docs/20261106_FI_evalue_histogram_fill.md`). `isotopeError`
sets `isotope_error` in every mode (default 2); `globalMinIonsScore`
> 0 sets `fragindex_min_ions_score` itself; `histogramFill` >= 0 sets
`fragindex_histogram_fill` (`docs/20261106_FI_evalue_histogram_fill.md`; test T68);
`useNLIons` >= 0 sets `use_NL_ions`. Run without arguments for the defaults.

Two more environment variables, so a new knob needs no new positional argument:

- `RTS_REPRO_PARAMS="<i|d|s|v>:<name>=<value>;..."` sets any further parameter, as an int,
  double, string or comma-separated double list (`v`, a `vector<double>` parameter such as
  `precursor_NL_ions`), after every built-in setting (so it can override one) and before
  `InitializeSingleSpectrumSearch()`, e.g. `RTS_REPRO_PARAMS="i:use_Z1_ions=1"` (test T71) or
  `"i:scale_fragmentNL=1;v:precursor_NL_ions=97.976896"` (test T76).
  The type must be the one Comet reads the parameter as: `GetParamValue()` casts to the
  requested type and a mismatch dereferences null instead of failing cleanly. Each applied
  entry is echoed to stderr; a malformed one stops the run.
- Fixture numbers (the SPECTRUM line's m/z and every peak's m/z and intensity) are read
  with `strtod`, so `nan`, `inf` and `-inf` load as those values; T73 feeds invalid inputs
  through the API this way.
- `RTS_REPRO_CANCEL_RACE=<us>` runs a thread that, until the workers finish, sleeps a random
  0..us microseconds, calls `CancelSearch()`, and calls `ResetSearchStatus()` 20 us later, so
  chimeric calls are cancelled at arbitrary points (test T82).
- `RTS_REPRO_TOPN=<n>` sets the `topN` mode 0 passes to `DoSingleSpectrumSearchMultiResults()`
  (default 1). A value below 1 makes the call return false, printed as `NO_MATCH` (test T83).
- Chimeric output: a call that stopped early (`bIncomplete`) keeps its rows and adds a line
  `MS2 <scan>\tINCOMPLETE budget` (`max_index_runtime` ran out, returned true) or `INCOMPLETE
  stopped` (cancelled or an index search failed, returned false). `ERROR` means false without
  `bIncomplete`, that is, invalid arguments.
- `RTS_REPRO_FRAGMENTS=1` adds, after each result line, one `FRAG` line per matched fragment
  the API returned: `FRAG <scan>`, `row`, `type` (the `ION_SERIES_*` index: 0 a, 1 b, 2 c,
  3 x, 4 y, 5 z, 6 z+1), `num`, `z` (fragment charge), `mass` (the fragment's 1+ mass), `nl`
  (neutral loss) and `inten`.

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

## Real data on Linux: mzxml_to_fixture.py

`mzxml_to_fixture.py` writes the MS2 scans of an mzXML file in the same `SPECTRUM` block
format (base64 peaks decoded, 32/64-bit, optional zlib; `--max N`, `--first-scan`,
`--last-scan`), which is how the chimeric-search measurements in
`docs/20261105_RTSchimeric.md` section 10 ran the 42,030-scan HeLa benchmark file here
without `.raw` access:

```bash
python3 tests/rts_repro/mzxml_to_fixture.py 20170103_HelaQC_01.mzXML > hela.txt
```

## Using this from tests/unit/run_tests.py

`tests/unit/run_tests.py` drives this reproducer from the T22 variants (integration tests,
`--integration`) and from the regular unit tests T67, T68, T71-T77, T79, T81-T83, T86, T87 and T89 (see
`tests/tests.md`). It builds `rts_repro` once per run (the same `g++` invocation as above)
when the binary is missing or older than `rts_repro.cpp` or any library it links; a failed
build fails those tests rather than skipping them. T22 runs two checks per index type
(FI_DB via `-i`, PI_DB via `-j`):

1. **Ground truth**: builds an FI_DB/PI_DB from `tests/unit/data/t19_ascore_fidb.fasta`
   (the same unambiguous single-phospho-peptide fixture T19/T20 use for the batch path)
   and asserts RTS finds `ACDEFGS[79.9663]K` -- confirming the RTS single-spectrum path
   actually works against both index types, not just that it runs.
2. **Determinism**: the 1-vs-8-thread check described above, against an FI_DB/PI_DB built
   from `data/human.small.fasta`.

`ms2_to_fixture.py` converts any `.ms2` file into this reproducer's `SPECTRUM` block
format (one block per `Z` line), so new ground-truth fixtures don't require extracting
real spectra from a `.raw` file:

```bash
python3 tests/rts_repro/ms2_to_fixture.py path/to/input.ms2 > my_fixture.txt
```

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
SPECTRUM <scanNumber> <charge> <precursorMz> <numPeaks> [toleranceMZ]
<mass1> <intensity1>
<mass2> <intensity2>
...
```

The optional `toleranceMZ` is that candidate's own precursor tolerance in m/z, used by the
chimeric explicit-list form (`chimeric` 2) as `PrecursorCandidate::dToleranceMZ`; absent or 0
means the per-call `tolMZ` (test T75).
