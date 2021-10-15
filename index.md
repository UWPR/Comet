<div id="sidebar">
  ## Recent Updates
  <li>2021/10/13: The Comet repository has migrated to GitHub
  <li>2021/06/23: Release 2021.01 rev. 0 is now available.
</div>

### Comet

Comet is an open source tandem mass spectrometry (MS/MS) sequence database search tool released under the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

Searching uninterpreted tandem mass spectra of peptides against sequence databases is the most common method used to identify peptides and proteins. Since this method was first developed in 1993, many commercial, free, and open source tools have been created over the years that accomplish this task.

Although its history goes back two decades, the Comet search engine was first made publicly available in August 2012 [on SourceForge](https://sourceforge.net/projects/comet-ms/) under the Apache License, version 2.0. The repository was [moved to GitHub](https://github.com/UWPR/Comet) in September 2021.  Comet is multithreaded, supports multiple input and output formats, and binaries are available for both Windows and Linux operating systems.

Note that Comet is just a single command line binary that perfoms  MS/MS database search. It takes in spectra in various supported input formats, using the [MSToolkit C++ library](https://github.com/mhoopmann/mstoolkit), and writes .pep.xml, .pin, .sqt and/or .txt files. You will need some other support tool(s) to actually make use of Comet results. Consider using [one of the many proteomics software suites](./releases/) that Comet is integrated into.




### Publications
- Comet: an open source tandem mass spectrometry sequence database search tool.
Eng JK, Jahan TA, Hoopmann MR. Proteomics. 2012 Nov 12.
doi: [10.1002/pmic.201200439](http://onlinelibrary.wiley.com/doi/10.1002/pmic.201200439/abstract)
- A Deeper Look into Comet - Implementation and Features.
Eng JK, Hoopmann MR, Jahan TA, Egertson JD, Noble WS, MacCoss MJ. J Am Soc Mass Spectrom. 2015 Jun 27.
doi: [10.1007/s13361-015-1179-x](http://link.springer.com/article/10.1007%2Fs13361-015-1179-x)
- Full-Featured, Real-Time Database Searching Platform Enables Fast and Accurate Multiplexed Quantitative Proteomics.
Schweppe DK, Eng JK, Yu Q, Bailey D, Rad R, Navarrete-Perea J, Huttlin EL, Erickson BK, Paulo JA, Gygi SP.
J Proteome Res. 2020 May 1;19(5):2026-2034.
doi: [10.1021/acs.jproteome.9b00860.](https://pubs.acs.org/doi/abs/10.1021/acs.jproteome.9b00860)

