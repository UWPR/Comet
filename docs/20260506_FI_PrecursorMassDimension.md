# Fragment Ion Index Precursor Mass Dimension Analysis

**Date:** 2026-05-06  

---

## 1. Evaluate adding a precursor mass dimension to the fragment ion index

Currently g_iFragmentIndex[] is a compressed sparse row flat data with data
entries being index entries in g_vFragmentPeptides. The goal is to analyze
whether or not adding a precursor mass dimension to the fragment ion Index
will help with performance. When using a larger fragment ion bin size of
1.0005 such as typical for low-resolution MS/MS spectra, each fragment ion
index bin will have many more peptide entries than if a smaller (0.02)
fragment ion bin is used, common for high-resolution MS/MS spectra.

With many more entries in every fragment ion bin

A batch search is run as:

```
cd c:\Work\CometMaster\RealTimeSaerch\bin\x64\Release ; c:\Work\Comet-master\x64\Release\Comet.exe -Dhuman.target-decoy.fasta.idx 20170103_HelaQC_01.raw
```

And the same real-time search is run as:

```
cd c:\Work\CometMaster\RealTimeSaerch\bin\x64\Release ; c:\Work\Comet-master\RealtimeSearch\bin\x64\Release\RealtimeSearch.exe 20170103_HelaQC_01.raw 20170103_HelaQC_01.raw human.target-decoy.fasta.idx 20
```

The relevant batch search timing output is:

```
 - searching "20170103_HelaQC_01" ... 10s (39970 spectra, 0.25ms/spec, 3930Hz)
```


The releveant real-time search timing output is:

```
search elapsed time: 33.21 s, avg 0.83 ms/spectrum (39970 spectra), 1204 Hz
```
