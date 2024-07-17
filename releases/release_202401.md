### Comet releases 2024.01

Documentation for parameters for release 2024.01 [can be found 
here](/Comet/parameters/parameters_202401/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2024.01 rev. 1 (2024.01.1), release date 2024/07/17

- Report the previous and next amino acid residues in the fragment ion index search output.  Previously the index search did not track this information and simply returned '-' for the preceding and trailing residues. Thanks to E. Bergstrom for tracking this and the other issues in the fragment ion index project.
- Remove the lower limit to allow smaller than 0.01 [fragment_bin_tol](https://uwpr.github.io/Comet/parameters/parameters_202401/fragment_bin_tol.html) values.  Thanks to I. Smith for reporting the presence of this lower limit in the code.
- Adds support for the new parameter "[pinfile_protein_delimiter](https://uwpr.github.io/Comet/parameters/parameters_202401/pinfile_protein_delimiter.html)" which will replace the Percolator pin file protein field delimiter from a tab to the specified character or string.  If this parameter entry is left blank the protein field delimiter remains a tab.  This is a hidden parameter in that it doesn't appear in the example comet.params files that can be downloaded from the Comet website.  Nor is it present in the abbreviated "comet.params.new" file generated with "comet -p".  It will be present in the full "comet.param.new" file generated with "comet -q". Thanks to S. Paez for requesting this feature in issue #66. (I should not that I also deprecated a previously undocumented "pin_mod_proteindelim" parameter that, when set, changed the pin file protein delimiter to a comma from the tab.)
- Fix backwards compatibility with the old/retired [peptide_mass_tolerance](https://uwpr.github.io/Comet/parameters/parameters_202301/peptide_mass_tolerance.html) parameter.  Although this parameter has been replaced by [peptide_mass_tolerance_lower](https://uwpr.github.io/Comet/parameters/parameters_202401/peptide_mass_tolerance_lower.html) and [peptide_mass_tolerance_upper](https://uwpr.github.io/Comet/parameters/parameters_202401/peptide_mass_tolerance_upper.html), Comet code was intended to continue supporting the old [peptide_mass_tolerance](https://uwpr.github.io/Comet/parameters/parameters_202301/peptide_mass_tolerance.html) parameter.  During a late change to support the new parameters, I broke support for the old parameter.  This is addressed by commit 23a3901.  Thanks to C. Bielow for posting this issue #60.
- Searches will stall when a fasta sequence loading threshold has been hit; addressed by commit e5cf236.  Thanks to C. Bielow for posting the issue #62.
- A logic error in the StorePeptide() kills the search as I did not properly account for the strcmp() string comparison returning true when one of the strings is empty.  This was also addressed by commit e5cf236.  Thanks to C. Bielow for posting the issue #63.
- Fix the mzIdentML output regular expression for the digestion enzyme, removing an extra space in the regular expression.  Also added the more complete EnzymeName element which references the PSI-MS ontology for common enzymes.  These were addressed by commit 754514f.  Thanks to J. Uszkoreit for posting the issue #64.
- Asp_N, Asp-N_ambic, and PepsinA enzyme definitions in comet.params.new are now updated as part of commit 754514f.
- Change the ppm tolerances to be applied to m/z instead of the deconvoluted mass.  There are slight differences between the two that become apparent in extremely large ppm tolerances; in practice this change doesn't really do anything.  This change was prompted by [issue #689](https://github.com/crux-toolkit/crux-toolkit/issues/689) on the crux-toolkit repository.
- macOS builds now use the macos-13 runner for compiling binaries as the macos-11 runner has been deprecated by GitHub.

#### release 2024.01 rev. 0 (2024.01.0), release date 2024/05/06

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
