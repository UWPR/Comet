### Comet releases 2024.01

Documentation for parameters for release 2024.01 [can be found 
here](/Comet/parameters/parameters_202401/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2024.01 rev. 0 (2024.01.0), release date 2024/05/03

- Add the parameters
"[peptide_mass_tolerance_lower](https://uwpr.github.io/Comet/parameters/parameters_202401/peptide_mass_tolerance_lower.html)"
and
"[peptide_mass_tolerance_upper](https://uwpr.github.io/Comet/parameters/parameters_202401/peptide_mass_tolerance_upper.html)"
to allow the specification of non-symmetric precursor mass tolerances.
This means that "peptide_mass_tolerance" is retired and you should start
with a fresh comet.params with this release and not re-use an old
parameters file.
- Add support for up to 15 variable modifications with the addition of
"[variable_mod10](https://uwpr.github.io/Comet/parameters/parameters_202401/variable_modXX.html)"
through
"[variable_mod15](https://uwpr.github.io/Comet/parameters/parameters_202401/variable_modXX.html)".
Please do not attempt to search with 15 (or even 9) variable mods without
using some serious constraints unless you are the most patient person in the world.
- Add support for what I will term an "exclusive" modification where only one from
the set of exclusive variable modifications can appear in a peptide. You would want
to apply this option to rare modifications that are unlikely to co-exist and be
identified along with another rate modification in the same peptide.  Denoting which
variable modifications are an "exclusive" modification is accomplished by setting
field 7 in the 
"[variable_mod##](https://uwpr.github.io/Comet/parameters/parameters_202401/variable_modXX.html)"
parameters to "-1".  The exclusive modification can still
apply to multiple residues (controlled by the 4th field) and can exist in conjunction
with other variable modifications that are not denoted as being exclusive.  This
reduces the complexity and search times when analyzing many modifications by not
requiring all permutation/combinations of modifications to be analyzed.  Requested by
E. Deutsch.
- Change
"[isotope_error](https://uwpr.github.io/Comet/parameters/parameters_202401/isotope_error.html)"
options 4 thru 7.  Those options now correspond to 4 = -1/0/1/2/3,
5 = -1/0/1, 6 = -3/-2/-1/0/1/2/3, 7 = -8/-4/0/4/8. 
- Add the parameter
"[resolve_fullpaths](https://uwpr.github.io/Comet/parameters/parameters_202401/resolve_fullpaths.html)"
to allow the control of whether or not 
to resolve the full path base_names in the pepXML output.  Default behavior is 
to resolve those full paths. This parameter allows the user to control 
leaving the paths as-is.  Requested by M. Riffle.
- Fix calculating good E-value scores for extremely sparse spectra. This occurs for
sparse spectra as any match to any single peak looks like an outlier from the
majority of peptides that match no peaks.  This is handled by putting a
constraint on the linear regression step of the E-value calculation.
- Simplify the spectral processing for Sp scoring (preliminary score) by just
taking the raw binned spectra and normalizing the max intensity to 100.
- Change the convention for the dCn (delta Cn) score for single hit results 
i.e. those results where only a single peptide is scored/reported.  In the 
past, these single peptide hits received a dCn score of 1.0 but now these 
single peptide hits will receive a dCn score of 0.0.
- Update to the
[MSToolkit library](https://github.com/mhoopmann/mstoolkit)
to fix a scan numbering bug when spectra are not numbered.  Implemented by
the talented M. Hoopmann.
- Update the index search, including the CometWrapper.dll interface used for 
real time search (RTS), to use fragment ion indexing.  It is still a work in
progress and not all functionality has been implemented (so do not use it
unless you want to be a beta tester). Documentation will be added when it is
ready for general use.  The fragment ion indexing is used as a pre-filter
to the full cross-correlation scoring and is not fast compared to other search tools.
Thanks to V. Sharma for implementing the modifications permutation code and
the E. Bergstrom, C. McGann, and D. Schweppe for development/testing feedback.
- Added
"[set_X_residue](https://uwpr.github.io/Comet/parameters/parameters_202401/set_X_residue.html)"
parameters which allow user to redefine the base mass of each amino acid residue
e.g. set_A_residue to modify the base mass of alanine. Making use of static modifications
can effectively accomplish the same thing so there is a very limited use case
for this new feature.  Feature requested by m.f.abdollahnia via the Comet google group.
- Implemented returning multiple results, instead of just the top hit peptide,
for each RTS spectrum query through the CometWrapper.dll interface.  Code was
contributed by our Thermo collaborators J. Canterbury and W. Barshop and
integrated by C. McGann.
- "comet -p" now generates a slightly simplified comet.params.new file.  Some
lesser used parameters are left out of that file.  "comet -q" will generate
a comet.params.new file with a more complete list of supported search parameters.
- Fixed issues with the mzIdentML output as reported by R. Marissen in
[issue #45](https://github.com/UWPR/Comet/issues/45).
- Fixed the inconsistent Sp rank numbers between runs, reported by keesh0 in
[issue #46](https://github.com/UWPR/Comet/issues/46).
- Fixed bug with counting the number of missed cleavages for enzymes that cut 
before (N-terminal of) the residue, reported by cpaul32015 in
[issue #47](https://github.com/UWPR/Comet/issues/47).
