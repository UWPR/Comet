### Comet releases 2012.01

Documentation for parameters for release 2012.01 [can be found
here](http://comet-ms.sourceforge.net/parameters/parameters_201201/).
Download release [here](https://sourceforge.net/projects/comet-ms/files/).

#### release 2012.01 rev 3 (2012.01.3), release date 2013/02/04
- This is a maintenance release.
- Addresses issue with .out file generation when full/relative path is
specified for input file.
- Removes full/relative paths in pep.xml output (i.e. in "summary_xml" and
"spectrum" elements).
- Fixes issue where spurrious modifications are reported if no variable
modification is specified in the search.
- Known issue with pep.xml modification reporting, see
[this post describing the problem](https://groups.google.com/forum/#!topic/comet-ms/KrbM57S050M).
- Reported issue with reading certain mzXML files. The program will segfault
when reading particular scans (not known why).
[More details here](https://groups.google.com/forum/#!topic/comet-ms/aHb_cP_5bXw).
- Known bug: -D command line parameter to set database does not work; you must
set the database in the params file.

#### release 2012.01 rev 2 (2012.01.2), release date 2013/01/10
- This is a maintenance release.
- Change is in pep.xml output only.
- This release implements the "deltacnstar" score in pep.xml output which is
important for things like phospho-searches containing homologous top-scoring
peptides when analyzed by PeptideProphet (using the "leave alone entries with
asterisked score values" option).

#### release 2012.01 rev 1 (2012.01.1), release date 2012/12/14
- This is a maintenance release.
- Covers most changes from the comet-ms SourceForge project revisions r24
through 34.
- Fixes/changes include correcting modified internal decoy peptides, corrects
modification reporting in pep.xml output, and adds more complete headers to SQT
output.
- Source and binary release files are named comet_source.2012011.zip and
comet_binaries.2012011.zip, respectively.
- Known bug: in pep.xml output, the "deltacnstar" and "deltacn" parameters
currently still does not implement the code logic of noting "similar" peptides.
This is important for the "leave alone asterisked score values" option in
PeptideProphet in conjunction with variable modification searches.

#### release 2012.01 rev 0 (2012.01.0), release date 2012/08/16
- This is the initial release of Comet.
- Known bug: modifications with internal decoy search will cause a segfault if
the enzyme cuts n-terminal to specified residues. This bug has been fixed in
the sources files in trunk as of 2012/10/09.
