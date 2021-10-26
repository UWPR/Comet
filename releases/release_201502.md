### Comet releases 2015.02

Documentation for parameters for release 2015.02 [can be found
here](http://comet-ms.sourceforge.net/parameters/parameters_201502/).
Download release [here](https://sourceforge.net/projects/comet-ms/files/).

#### release 2015.02 rev. 5 (2015.02.5), release date 2016/01/22
- pepXML output: correctly report modified_peptide string; previously missing
static modifications and at times terminal modifications.
- MGF file parsing: fix how fragment masses are adjusted when their fragment
ion charge states are present.
- RAW file parsing: update [MSToolkit](https://github.com/mhoopmann/mstoolkit)
to not report warning of unknown tokens 't' and 'E'.

#### release 2015.02 rev. 4 (2015.02.4), release date 2016/01/07
- Additional parsing changes for better MGF support.
- pepXML output: correct "mod_cterm_mass" value and escape special characters
in "spectrumNativeID" values.

#### release 2015.02 rev. 3 (2015.02.3), release date 2015/11/24
- Fix incorrect MGF parsing where blank lines in the MGF file would cause an
error.
- Fix n-term distance constraint variable mod searches. Thanks to U. Eckhard
for reporting the above two issues.
- Change output file extension to ".pin" from ".tsv" for Percolator output
files.
- Fix negative deltaCn values in text file output when no second hit is
present.
- Fix case where double decoy string is appended to the protein accession for
decoy matches.

#### release 2015.02 rev. 2 (2015.02.2), release date 2015/10/15
- Minor update to fix bug in binary modifications. The bug manifested itself in
the simplest case where say two lysine residues are labeled with different
modification masses and set to different binary modification groups. In this
case, the second modification is not analyzed. Bug reported by Villen lab.
- Known bug: when decoy searches are applied, the decoy prefix could be added
multiple times for certain cases where the same decoy peptide is present in
multiple decoy proteins. A fix has been implemented and will be included in the
next release; let me know if you need a patched binary sooner than that.

#### release 2015.02 rev. 1 (2015.02.1), release date 2015/09/30
- Modify behavior the binary modifications which is controlled by the third
parameter entry in the variable modifications (e.g. "variable_mod01"). Instead
of a binary 0 or 1 value to turn off or on each binary modification, one can
now set the third parameter entry to the same value across multiple variable
modifications effectively allowing an all-modified binary behavior across
multiple variable modifications. See the examples at the bottom of the variable
modification help pages for further explanation. NOTE: a bug was just
discovered with binary modifications; a fix is imminent.
- Wide mass tolerance searches, such as those performed by the
[Gygi lab's mass-tolerant searches](https://pubmed.ncbi.nlm.nih.gov/26076430/),
are now supported by Comet. Previous versions of Comet
would crash when given large tolerances.
- Update MSToolkit to support "possible charge state" cvParam in mzML files as
implemented by M. Hoopmann.

#### release 2015.02 rev. 0 (2015.02.0), release date 2015/07/31
- Associated with this release, a Windows GUI program to run Comet searches and
visualize results is available. The Comet GUI supports 64-bit Windows only.
- Updated to [MSToolkit](https://github.com/mhoopmann/mstoolkit)
revision 81 which includes .mgf input file support.
Thanks to M. Hoopmann for updating MSToolkit for this.
- Add a fourth option to ("override_charge") which will either use the
specified charge in the input file or apply the charge states in the
charge_range parameter but include the 1+ charge rule. Requested by D.
Shteynberg.
- Add "mass_offsets" parameter. Using this parameter, one can search spectra
for peptides that have a mass offset from the experimental mass. Requested by
ISB.
- The "precursor_tolerance_type" parameter makes its return. It was not needed
for precursor tolerances specified as ppm, which is the reason it was removed.
But it is still relevant when amu and mmu are the units specified for the
precursor tolerance and is now only applied in these cases.
- The "use_sparse_matrix" parameter has been retired. All searches now use this
internal data representation by default.
- Corrected specification of terminal modifications in pep.xml output in cases
when both static peptide and protein terminal modifications are specified.
Reported by D. Hernandez.
- Fix small bug that inadvertantly removed .cms2 input file support in previous
release. Reported by MacCoss lab.
