### Comet releases 2016.01

Documentation for parameters for release 2016.01 [can be found
here](http://comet-ms.sourceforge.net/parameters/parameters_201601/).
Download release [here](https://sourceforge.net/projects/comet-ms/files/).

#### release 2016.01 rev. 3 (2016.01.3), release date 2017/05/02
- Bug fix: peptides with variable modifications would have their precursor mass
calculated using fragment ion masses. This is an issue when the precursor mass
type and fragment mass types are different e.g. average masses for the
precursor calculation and monoisotopic masses for the fragment ion
calculations. Thanks to D. Zhao for identifying the bug.
- Update MSToolkit to latest version. This encapsulates the bug fix mentioned
below as well as better support for ETD+SA scans. Thanks to P. Pedrioli for
originally implementing the ETD+SA fixes.
- Incorporate r981 which was missing edge cases in internal binary mass search.
- pepXML output fix: declare aminoacid_modification before any instance of
terminal_modification per schema. Thank to L. Mendoza for reporting this issue.

#### release 2016.01 rev. 2 (2016.01.2), release date 2016/04/06
- Reverts the modification encoding in the "output_txtfile" output back to
reporting variable modification mass differences (e.g. DLSTM[16.0]HK) instead
of the actual modified residue mass values (e.g. DLSTM[147]HK).
- Known bug: there's an issue with raw file parsing where the charge state and
precursor mass might not be correctly read. Mike Hoopmann made an update to the
MSToolkit file parsing library to fix this. I've decided not to make a new
release (branching and tagging the code and assigning a new version string).
Please download the comet_source_2016012.zip or comet_binaries_2016012.zip
files again to obtain the patched program. These zip files were updated on
10/13/2016. Thanks to A. Sharma for reporting the bug.

#### release 2016.01 rev. 1 (2016.01.1), release date 2016/03/29
- Fixes a bug where variable terminal modifications on decoy peptides are not
properly generated leading to possible program crash due to calculating
fragment ions that are too large for the decoy peptide. Reported by Villen lab.
- Fixes a bug where peptide mass comparisons are failing, leading to duplicate
peptides being stored and reported as separate entries. Reported by S.
Michalakopoulos.
- For text and pepXML output, the peptide rank for ties (two peptides with
exact same xcorr) now have the same rank instead of the rankings always being
sequential.
- The 2016.01 release also changed how modifications are reported in the text
output. Instead of mass differences (e.g. DLSTM[16.0]HK), the modification
annotation now shows the modified residue mass (e.g. DLSTM[147]HK).

#### release 2016.01 rev. 0 (2016.01.0), release date 2016/03/15
- Known bug: when using a decoy search and variable n- or c-terminal
modifications, Comet could segfault/crash due to accessing an array out of
bounds. This is due to the modfications not being properly translated to the
decoy peptide which could result in calculated fragment ion masses that are too
large. A fix is implemented and will be part of the next release. Until the
next release is available, email me if you would like a patched binary.
- Add direct selenocysteine support by retiring parameter entry
"add_U_user_amino_acid" and adding "add_U_selenocysteine" per
[doi:10.1021/acs.jproteome.5b01028](https://pubs.acs.org/doi/10.1021/acs.jproteome.5b01028).
- Allow negative numbers for the "num_threads" parameter; will subtract that
many threads from # CPU cores. For example, to use 3 threads for a 4-core CPU
or 7 threads for an 8-core CPU, set "num_threads = -1". This allows your
computer to have a CPU core free when running searches. Requested by D.
Shteynberg.
- Fix bug in the expectation score calculation when "print_expect_score = 0"
and pep.xml, pin, and txt files are specified (but not .out files). Under this
scenario, the cross correlation histogram was not being fully accumulated which
is what is used to calculate the E-value. This fix should result in more
accurate E-values. Reported by A. Cheng. (Note "print_expect_score" was
intended to only affect ".out" output.)
- For Comet compiled for [Crux](http://crux.ms/), return the missing modified
peptide column in the text output.
- Update deltaCn calculation for non top-ranked peptides or .pep.xml, .pin and
.txt outputs. The deltaCn values for the top ranked hits do not change. The
lower hit entries had incorrect values associated with them (normalized xcorr
difference between consecutive entries instead of the normalized xcorr
difference from the top hit, the latter which is correct). Reported by M.
Hoopmann.
- This version of Comet will run with comet.params files generated from version
2015.02. If you use these parameter files, you will receive a warning message
about an unknown parameter "add_U_user_amino_acid" that you can safely ignore.
Set "add_U_user_amino_acid = 150.95363" for selenocysteine support with the old
parameters file.
