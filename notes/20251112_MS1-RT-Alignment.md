### Notes 2024/11/12: MS1 retention time alignment in Comet

Comet's real-time search interface has been extended to incorporate functionality to
align an acquisition run against a reference run using MS1 scans.

Alignment is performed by scoring each query MS1 scan against the reference MS1 scans
using a unit vector dot product (that's also referred to as cosine similarity score).
The range of reference MS1 scans is limited by those within the retentime time tolerance
specified.  Alignment of runs of difference linear gradient lengths is supported (although
now that I'm writing this documentation, I think I may need to add another parameter to
allow the user to specify the RTS query's gradient length to support this).

The best match reference MS1 scan, based on having the highest unit vector dot product score,
is returned.

MS1 spectra are represented as spectral arrays in the same manner as Comet represents MS/MS
spectra internally.  These arrays are defined by the ms1_bin_tol and ms1_bin_offset
parameters where 1.0005 and 0.4 is recommended for these two parameters, respectively.
Smaller ms1_bin_tol values can be used for more sensitive matching (which I've found to
be unnecessary) but at the expense of longer query times.

To mitigate local noise in the retention time alignment, Comet keeps a history of the 250
most recent retention time matches.  A linear regresssion of the retention time matches is
performed with outlier detection.  See CometAlignment.cpp for details.

Real-time MS1 alignment queries can be performed alongside real-time MS/MS searches.  The
example C# program RealTimeSearch/SearchMS1MS2.cs gives an example of how these searches
can be run by calling DoMS1SearchMultiResult() and DoSingleSpectrumSearchMultiResult() in
the CometWrapper interface.


### MS1 alignment specific paramters
- [retentiontime_tol](https://uwpr.github.io/Comet/parameters/parameters_202503/retentiontime_tol.html)
- [ms1_bin_tol](https://uwpr.github.io/Comet/parameters/parameters_202503/ms1_bin_tol.html)
- [ms1_bin_offset](https://uwpr.github.io/Comet/parameters/parameters_202503/ms1_bin_offset.html)
- [ms1_mass_range](https://uwpr.github.io/Comet/parameters/parameters_202503/ms1_mass_range.html)

The graphs below show an example alignment between a 60 minute reference run and 60
minute query run.
![MS1 alignment graphs](https://uwpr.github.io/Comet/notes/20251112-MS1-alignment.png)


The graph below shows an extreme example where a 180 minute run is aligned against a 60
minute run.
![MS1 alignment graph B](https://uwpr.github.io/Comet/notes/20251112-MS1-alignment_b.png)