# Fragment Ion Index Analysis Time Comparison

**Date:** 2026-04-28  

---

## 1. Batch search and real time search

Using the same input raw file and hopefully the same search parameters
against the same .idx database, the batch search is running the search much
faster than the real-time search through the CometWrapper.dll interface.
Analyze the code paths, run analysis, and determine why the real-times
search through CometWrapper.dll is slower.  Implement a fix if possible.

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
