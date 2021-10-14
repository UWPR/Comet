#### Comet

Comet is an open source tandem mass spectrometry (MS/MS) sequence database search tool.

Searching uninterpreted tandem mass spectra of peptides against sequence databases is the most common method used to identify peptides and proteins. Since this method was first developed in 1993, many commercial, free, and open source tools have been created over the years that accomplish this task.

Although its history goes back two decades, the Comet search engine was first made publicly available in August 2012 [on SourceForge](https://sourceforge.net/projects/comet-ms/) under the Apache License, version 2.0. The repository was [moved to GitHub](https://github.com/UWPR/Comet) in September 2021.  Comet is multithreaded, supports multiple input and output formats, and binaries are available for both Windows and Linux operating systems.

Note that Comet is just a single command line binary that perfoms  MS/MS database search. It takes in spectra in various supported input formats, using the [MSToolkit C++ library](https://github.com/mhoopmann/mstoolkit), and writes .pep.xml, .pin, .sqt and/or .txt files. You will need some other support tool(s) to actually make use of Comet results. Or use [one of the many proteomics software suites](./releases/) that Comet is integrated into.

![cometlogo](/images/cometlogo_small.png)

#### Publications
- Comet: an open source tandem mass spectrometry sequence database search tool. Eng JK, Jahan TA, Hoopmann MR. Proteomics. 2012 Nov 12. doi: [10.1002/pmic.201200439](http://onlinelibrary.wiley.com/doi/10.1002/pmic.201200439/abstract)
- A Deeper Look into Comet - Implementation and Features. Eng JK, Hoopmann MR, Jahan TA, Egertson JD, Noble WS, MacCoss MJ. J Am Soc Mass Spectrom. 2015 Jun 27. doi: [10.1007/s13361-015-1179-x](http://link.springer.com/article/10.1007%2Fs13361-015-1179-x)
