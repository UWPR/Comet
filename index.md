### Welcome to the Comet project!

<div id="sidebar">
  <h2>News</h2>
  <h3>2025/03/15: <a href="/Comet/releases/release_202501.html">Release 2025.01 rev. 1</a> is now available.</h3>
  <h3>2024/10/14: <a href="/Comet/releases/release_202402.html">Release 2024.02 rev. 0</a> is now available.</h3>
  <h3>2021/09/17: The Comet repository has migrated to GitHub.</h3>
  <h2>Keep Updated</h2>
  <h3>Subscribe to <a href="http://groups.google.com/group/comet-ms">Comet's Google group</a> for announcements, issues, questions.</h3>
</div>

Comet is an open source tandem mass spectrometry (MS/MS) sequence database
search tool released under the [Apache 2.0
license](https://www.apache.org/licenses/LICENSE-2.0).

Searching uninterpreted tandem mass spectra of peptides against sequence
databases is the most common method used to identify peptides and proteins.
Since this method was first developed in 1993, many commercial, free, and open
source tools have been created over the years that accomplish this task.
Although its history goes back two decades, the Comet search engine was first
made publicly available in August 2012 [on
SourceForge](https://sourceforge.net/projects/comet-ms/) under the Apache
License, version 2.0. The repository was [migrated to
GitHub](https://github.com/UWPR/Comet) in September 2021.

Comet is multithreaded, supports multiple input and output formats, and
binaries are available for both Windows and Linux operating systems.  Note that
Comet is just a single command line binary that performs  MS/MS database search.
It takes in spectra in various supported input formats, using the [MSToolkit
C++ library](https://github.com/mhoopmann/mstoolkit), and writes .pep.xml,
.pin, .sqt and/or .txt files. You will need some other support tool(s) to
actually make use of Comet results. Unless you specifically know that you want
to use the standalone Comet command line binary available from this repository,
you should start with [one of the many proteomics software suites](/Comet/releases/)
that Comet is integrated into.


### Publications
- [Comet: an open source tandem mass spectrometry sequence database search tool](http://onlinelibrary.wiley.com/doi/10.1002/pmic.201200439/abstract).
Eng JK, Jahan TA, Hoopmann MR. Proteomics. 2012 Nov 12.  doi: 10.1002/pmic.201200439
- [A Deeper Look into Comet - Implementation and Features](http://link.springer.com/article/10.1007%2Fs13361-015-1179-x).  Eng
JK, Hoopmann MR, Jahan TA, Egertson JD, Noble WS, MacCoss MJ. J Am Soc Mass
Spectrom. 2015 Jun 27.  doi: 10.1007/s13361-015-1179-x
- [Full-Featured, Real-Time Database Searching Platform Enables Fast and Accurate Multiplexed Quantitative Proteomics](https://pubs.acs.org/doi/abs/10.1021/acs.jproteome.9b00860).
Schweppe DK, Eng JK, Yu Q, Bailey D, Rad R, Navarrete-Perea J, Huttlin EL,
Erickson BK, Paulo JA, Gygi SP.  J Proteome Res. 2020 May 1;19(5):2026-2034.
doi: 10.1021/acs.jproteome.9b00860
- [Extending Comet for Global Amino Acid Variant and Post-Translational Modification Analysis Using the PSI Extended FASTA Format](https://analyticalsciencejournals.onlinelibrary.wiley.com/doi/10.1002/pmic.201900362).
Eng JK and Deutsch EW. Proteomics. Proteomics. 2020 Nov;20(21-22):e1900362. 
doi: 10.1002/pmic.201900362.



*I know you're smart. But everyone here is smart. Smart isn't enough. The kind
of people I want on my research team are those who will help everyone feel
happy to be here.*  [Randy Pausch, The Last Lecture](http://www.youtube.com/watch?v=ji5_MqicxSo)

