# Fragment-index lookup of multiply-charged fragment peaks (2026-11-08)

## 1. Problem

The fragment index holds 1+ b/y masses (each includes one proton). `SearchFragmentIndex()`
probes every observed peak at each fragment charge up to the spectrum's maximum and converts
it to that 1+ mass. A peak at charge z is (M + z * proton) / z, so the 1+ mass is
mz * z - (z - 1) * proton. The lookup subtracted (z - 1) * 1.0 instead:

```
uiFragmentMass = BIN((*it2) * iChg - (iChg - 1.0));                 // before
uiFragmentMass = BIN((*it2) * iChg - (iChg - 1) * PROTON_MASS);     // after
```

Every 2+ probe landed 0.0073 Da high and every 3+ probe 0.0146 Da high: a third and two
thirds of a 0.02 Da bin. At `fragment_bin_tol 0.02` / offset 0 a 2+ fragment is looked up in
the wrong bin whenever its 1+ mass sits in the top third of its bin (about 36% of 2+ and 73%
of 3+ lookups in a Monte Carlo over peptides); at 1.0005 Da bins about 1%. Scoring
(`CometMassSpecUtils::GetFragmentIonMass()`, `XcorrScoreI()`) has always used `PROTON_MASS`,
so only the candidate prefilter was affected: which peptides reach
`fragindex_min_ions_score`, the top-`FRAGINDEX_MAX_NUMSCORED` cut and the histogram-fill
ranking.

Found by the branch code review. The expression dates from the first fragment-index release
(an integer `(iChg - 1)` until the 2023 cleanup `f27ce821` made it `1.0`), so released Comet
versions behave the same way.

## 2. Who is affected

- Fragment-index searches only (batch FI_DB and RTS, including the chimeric search). PI_DB and
  FASTA searches score every candidate in the precursor window and never use this lookup.
- Only precursors of 3+ and above: the maximum fragment charge is the precursor charge - 1
  (capped by `max_fragment_charge`), so a 2+ precursor probes 1+ only. Verified: every 2+ row
  of the HeLa search below is byte-identical before and after.
- Mostly high-resolution bin widths.

The index file does not change; no rebuild is needed.

## 3. Measurement

`20170103_HelaQC_01.mzXML` (42,030 HCD scans), `human.canonical.target-decoy.fasta`
(`DECOY_`), `data/comet_phospho.params` (20 ppm, `isotope_error` 0, 0.02 Da bins,
`max_fragment_charge` 3, M oxidation and STY phospho), indexes built with `-i` / `-j`,
batch `comet.exe` at 20 threads, rank-1 only, FDR = decoys / targets (`tools/qvalue.py`).
Before = `dd2490e2`, after = the fix.

Targets at 1% FDR, q-values computed within each precursor charge:

| | z2 | z3 | z4 | z5+ | sum |
|---|---|---|---|---|---|
| E-value, FI before | 9,135 | 3,748 | 193 | 4 | 13,080 |
| E-value, FI after | 9,135 | **3,771** | **207** | 2 | **13,115** |
| E-value, PI | 9,034 | 3,658 | 207 | 3 | 12,902 |
| xcorr, FI before | 8,617 | 3,537 | 168 | 1 | 12,323 |
| xcorr, FI after | 8,617 | 3,540 | 168 | 1 | 12,326 |
| xcorr, PI | 8,600 | 3,482 | 160 | 2 | 12,244 |

With q-values over all charges together (what `tools/qvalue.py` prints): by E-value 13,051 ->
13,091, by xcorr 11,450 -> 11,401. The xcorr drop is a charge-mixing artifact: the 2+ rows are
identical, but 3+ and 4+ spectra gain results (rank-1 rows 27,655 -> 27,813; decoys among
3+ and higher rows 3,844 -> 3,902) whose xcorr is not comparable to 2+ xcorr, so the global
threshold moves; within each charge xcorr counts are flat.

Agreement with the peptide index, which scores every candidate in the window: FI rank-1 equal
to PI rank-1 on 3+ precursors **6,220 -> 6,391** (+2.7%); 4+ 543 -> 539; 5+ and above 21 -> 26.
1,936 of 10,562 3+ rank-1 peptides changed, almost all of them low-scoring.

Cost: 26.1 s -> 24.3 s wall (one run each, index load included): no measurable change. The
number of lookups is the same; they now land in the right bins.

The gain on this HCD file is modest because most 3+ spectra already clear the score floor on
their 1+ fragments; the fix matters most where 2+ fragments carry the evidence (high-charge
precursors, ETD-like or sparse spectra). T70 is the extreme case: a 3+ spectrum of 12 2+
fragment peaks that the old lookup counted as 0 index hits (no match) and the new one as 12.

## 4. Tests

- **T70** (`t70_fi_fragment_charge`, `tests/unit/data/t70_make_highcharge.py`): the fixture
  keeps only 2+ b/y ions the old conversion looked up in the wrong bin. FI_DB finds the target
  after the fix and returned no match before; PI_DB finds it either way.
- T23/T24 compare PSM counts at 1% FDR by xcorr against `BASELINE_TAG` within 10%; the HeLa
  numbers above move FI by -0.4% on that measure.

## 5. Release note

*"Fragment-ion-index searches now look up multiply-charged fragment peaks at the correct mass:
the conversion to the indexed 1+ mass subtracted 1.0 instead of the proton mass per extra
charge, so at high-resolution bin widths many 2+ and 3+ fragments of 3+ and higher precursors
were looked up in the wrong bin. Results for 2+ precursors are unchanged; 3+ and higher
precursors can return different, more often correct, peptides (on a HeLa benchmark +2.7% rank-1
agreement with the peptide index on 3+ precursors). No index rebuild is needed."*
