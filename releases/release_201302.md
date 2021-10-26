### Comet releases 2013.02

Documentation for parameters for release 2013.02 [can be found
here](http://comet-ms.sourceforge.net/parameters/parameters_201302/).
Download release [here](https://sourceforge.net/projects/comet-ms/files/).

#### release 2013.02 rev 2 (2013.02.2), release date 2014/01/23
- The major change with this maintenance release addresses Windows search
performance. When searching using lots of memory (such as with small
"fragment_bin_tol" values), the search performance was greatly reduced. We
believe allocated memory was being swapped to disk even when there is more than
enough memory/RAM on the host machine. This issue is addressed by allocating
memory differently (using the VirtualAlloc function call) and locking memory
from being paged out to disk (using the VirtualLock function call).
- Extend maximum protein accession string length from 40 to 512.
- Change default "remove_precursor_tolerance" value from 2.0 to 1.5.
- Skip writing out blank search hit lines for Crux-compiled text output.
- Corrects E-value calculation for decoy entries when using separate
target/decoy searches (i.e. "decoy_search = 2").

#### release 2013.02 rev 1 (2013.02.1), release date 2013/11/25
- Fix bug for both pep.xml and pin.xml output where terminal variable
modifications were not being written out to these two output formats.
- Fix bug where .sqt and .txt headers were not being written out to subsequent
output files after the first output file when multiple input files are
specified on the command line (i.e. "comet.exe *.mzXML").
- Allow whitespace in "database = " parameter value i.e. the database name
and/or path can contain a whitespace. Note that having whitespaces in the
database name will break some downstream tools (like TPP).
- Change Percolator pin.xml features. "dM" and "absdM" are now calculated as
(experimental_mass - calculated_mass)/calculated_mass. Previously this feature
was reporting (calculated_mz - experimental_mz). Sadly this also means that the
"calculatedMassToCharge" and "experimentalMassToCharge" attributes are actually
report MH+ masses now. Although this is completely wrong, this is to be
consistent with a different tool (sqt2pin) that is also generating these files.
- Change behavior in pep.xml output where "summary_xml" and "base_name"
elements now include resolved full paths. Also get rid of unused xml-stylesheet
directive.
- Increase number of significant digits to 6 for reported masses in all of the
output formats.
- Known issue: The nucleotide database search options, invoked using the
"nucleotide_reading_frame" parameter entry, is not functional.
- Known issue: Windows performance when using large memory (i.e. when the
"fragment_bin_tol" parameter is set to a small value) is poor. We believe this
is due to disk paging even for systems with sufficient memory. A fix, using
MSFT's VirtualAlloc and VirtualLock functions, is in the works.

#### release 2013.02 rev 0 (2013.02.0), release date 2013/10/18
- There are quite a few behind-the-scenes changes where the core search code
has been moved into a library in preparation for interfacing with other tools
(thanks to T. Jahan).
- Underlying changes to the code were made to facilitate integration with the
[Crux project](http://crux.ms) (thanks to S. McIlwain).
- This release incorporates an update to the
[MSToolkit](https://github.com/mhoopmann/mstoolkit)
file parsing library (thanks to M. Hoopmann).
- Support for the mz5 file format has been dropped. mzXML, mzML, and ms2
formats are the supported input formats.
- Comet will now report a warning for any unknown parameter due to manually
introduced typos. In the past, these were ignored. The recommended method to
generate a valid/current parameters file is still to use the command "comet.exe
-p" to avoid silly errors like this.
- A new parameter "decoy_prefix" allows one to define the string pre-pended to
the protein name for decoy matches when the decoy_search parameter is used.
- A new parameter "output_pinxmlfile" controls support for outputting
Percolator pin.xml files.
- The text output format has changed to add new columns and column headers.
Please note that neutral masses are now reported (vs. singly protonated masses
as in the past).
- During testing of this release, it was discovered that a subset of N-terminal
fragment ions for internally generated decoy peptides were incorrectly being
calculated; this has been corrected in this release.
- The tab-delimited text output was reported the second best hit and not the
top ranked hit. This has also been corrected in this release.
