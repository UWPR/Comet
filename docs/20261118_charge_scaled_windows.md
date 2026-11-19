# Charge-scaled precursor windows in the FASTA_DB search

2026-11-18

## The bug

With `peptide_mass_units` amu or mmu and `precursor_tolerance_type = 1` (precursor m/z), a
query's mass window is the tolerance times its charge, so a 4+ query is four times as wide as a
1+ query of nearly the same mass. The FASTA_DB candidate search finds the queries a peptide could
match by bisecting the mass-sorted query list on each query's window edge (`BinarySearchMass`),
walking back to the first possible query, and stopping at the first query whose low edge is
above the peptide mass. All of that assumes the edges increase along the list. With
charge-scaled windows they do not, so:

- candidates were missed, deterministically (a wide window just below the peptide, followed in
  mass order by narrow windows that end below it, was lost by the bisection; a narrow window
  that starts above the peptide, followed by wide ones that contain it, ended the walk early);
- `g_massRange` (`SearchUtils.cpp`) took `queries.front().Minus` and `queries.back().Plus`,
  which are not the extremes either;
- with several threads the output also varied from run to run. Queries enter the list in
  thread-completion order and the sort is not stable for equal masses, so the list order, and
  with it which candidates were lost, changed between runs.

ppm windows and MH+-type amu/mmu windows are constant offsets from the mass and were always
correct. FI_DB and PI_DB bracket each query on its own and are not affected: `t68_window_fi` /
`t68_window_pi` check a 72-query batch against each query searched alone on an index (plain,
variable-mod and required-mod searches) and pass on the previous build too.

Found while checking Mango output reproducibility: the Mango `comet.params` use ppm, so Mango
runs as configured were never affected, but any amu/mmu m/z-type search is.

## The fix

`PepMassInfo` gains two monotone envelopes, built by `BuildMassWindowEnvelope()` right after the
sort: `dSearchPlus` (prefix-max of `dPeptideMassTolerancePlus`) and `dSearchMinus` (suffix-min of
`dPeptideMassToleranceMinus`). The bisection, the walk-back, the early breaks (plain,
variable-mod and compound-mod loops) and `g_massRange` use them. They only bound where to look;
`CheckMassMatch()` still applies each query's own exact window, so the set of matches is exactly
"every query whose own window holds the mass". `BinarySearchMass` keeps its contract (first
query whose own window contains the mass, else -1) by scanning forward from the bisection point.
A lone query (RTS) and a not-yet-enveloped query default to a window that cannot reject
anything.

## Measurements

Real Mango file (6374 queries with hits), FASTA_DB, amu 0.02 / type 1, 1 thread, fixed vs
previous build: 71 rows in 21 queries (0.33%) change; in 34 rows the reported peptide changed and
3 queries gain a higher-scoring top hit (none lose one). Before the fix: 4 distinct outputs in 6
runs at 8 threads; after: 1 distinct output in 8 runs, equal to the 1-thread output. With the
shipped ppm params the `.txt`, pepXML and `.pin` of FASTA_DB, FI_DB and PI_DB are
byte-identical to the previous build and run times are unchanged.

## Tests

`t68_window_plain` (amu and mmu bug cases, amu MH+-type and ppm controls), `t68_window_varmod`,
`t68_window_reqvarmod`, `t68_window_compound`, `t68_window_peff`: a batch of generated queries
must equal each query searched alone. All fail on the previous build (the same scans are missed
on every run) and pass now; the controls pass on both.
