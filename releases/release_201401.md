### Comet releases 2014.01

Documentation for parameters for release 2014.01 [can be found
here](http://comet-ms.sourceforge.net/parameters/parameters_201401/).
Download release [here](https://sourceforge.net/projects/comet-ms/files/).

#### release 2014.01 rev. 1 (2014.01.1), release date 2014/06/03
- Known issue: when using "precursor_tolerance_type = 1" to specify the
tolerance is applied on the precursor m/z peak, Comet has been errantly
calculating the mass tolerance on the deconvoluted peptide mass and then
additionally scaling this tolerance by the charge state. This inflates the
precursor tolerance by N-fold where N is the precursor charge state (so a 10
ppm tolerance ends up being 20 ppm for 2+ precursors and 30 ppm for 3+
precursors). The fix is to always specify "precursor_tolerance_type = 0". This
parameter will be deprecated in the next release.
- Known issue: using custom amino acids (B, J, U, X, Z) fails.
- Known issue: high-res ms/ms searches (using small "fragment_bin_tol" values)
are ~2x slower than 2014.01.0 due to an unnecessary array initialization that
was added in this maintenance release.
- Known issue: Using -N<name> command line option adds full path to "spectrum"
attribute in Windows.
- Known issue: Using -N<name> command line option, the pep.xml "base_name"
attribute has full path to the output file instead of to the input file.
- Re-use temporary arrays during spectral preprocessing for better memory
management running under Windows, implemented by M. Hoopmann.
- New parameter "override_charge" instructs Comet to override the listed
precursor charge state in the input file with that specified by the
"charge_range" parameter, implemented by D. Shteynberg.
- Update MSToolkit to version r68 to support non-consecutive mzXML scans.
- Bug fix: synchronize normal and sparse matrix xcorr scores when decoys are
generated to fill in the xcorr distribution for the expectation score
calculation, reported by D. Tabb.
- Bug fix: buffer overflow issue on some linux distributions when retrieving
host name, reported by K. Tamura.
- Bug fix: close spawned threads, reported by M. Riffle.
- Bug fix: corrected "sample_enzyme" reporting in pep.xml output, reported by
L. Mendoza.
- Bug fix: remove extra tab in txt output, reported by A. Kertesz-Farkas.
- Bug fix: output file name gets mangled when .out files are used and the input
is specified using a full path, reported by C. Hoogland.

#### release 2014.01 rev. 0 (2014.01.0), release date 2014/03/25
- Known issue: the sample_enzyme information in pep.xml output is completely
broken.
- Known issue: there is a memory leak in the program. Avoid invoking Comet with
a ton of input files on the command line.
- Known issue: report of inconsistent results using "sparse_matrix = 0" and
"sparse_matrix = 1" searches. I believe this is due to decoy xcorr score
generation, affecting E-values, in cases where there is a low number of
candidate peptide matches.
- Known issue: it's been reported that results are inconsistent depending on
the number of threads used in a search (Windows). Some numbers deviate in the
3rd/4th decimal point.
- Add "output_suffix" parameter option which appends a suffix to the base name
of pep.xml, pin.xml, txt, and sqt output files.
- Add Native ID attribute to pep.xml output when searching mzML files. This
entails an update to the MSToolkit file parsing library which was made by M.
Hoopman.
- Report the name of the file that's being searched in the runtime output.
- Avoid creating empty/stub output files in the case where an input file has
been moved away during a search. These first four features were based on
requests by D. Tabb.
- Change how E-values are calculated when "decoy_search = 2" is applied. This
setting performs a separate target and decoy search as if two different
databases were searched separately resulting in two sets of search results. In
the past, the E-value calculations for the target hits were calculated on the
underlying target xcorr score distribution only. And the decoy E-value
calculations were based only on the decoy xcorr score distribution. Now, the
target and decoy score hits contribute to one xcorr score distribution that is
used to calculate the target and decoy E-values per suggestion by the Noble
lab.
- Sort results by scan number; previously results were reported in order of
increasing peptide mass. With results sorted by scan number, this allows more
efficient analysis using quantification tools that cache spectral data in
memory. Requested by D. Shteynberg.
- Bug fix: Change in spectral processing to handle edge case where the last
peak in some spectra are excluded from the analysis.
- Bug fix: Correct a bug that required order of two loops to be swapped. Before
this fix, a small number of fragment ions possibly do not get considered for
particular peptide/spectrum/charge combinations. Thanks to A. Kertesz-Farkas
for identifying these last two issues.
- For Windows binary only: revert previous memory changes (using VirtualAlloc
and VirtualLock functions) as there were negative consequences associated with
unexpected out of memory errors.
- For Crux compiled version only: modify text output, primarily changing column
order using changes submitted by S. McIlWain.
- For Crux compiled version only: report all peptide hits, including those with
negative xcorr scores.
- As the "output_suffix" is the only new parameter entry which most researchers
likely won't use, this version of Comet will also run with comet.params files
from version 2013.02.
