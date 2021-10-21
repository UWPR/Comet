### Comet releases 2017.01

Documentation for parameters for release 2017.01 [can be found
here](/Comet/parameters/parameters_201701/).
Download release [here](https://sourceforge.net/projects/comet-ms/files/).

#### release 2017.01 rev. 4 (2017.01.4), release date 2018/02/14
- Bug fix: In the Percolator .pin output format, the deltCn and deltLCn values
for the top hit is repeated for all lower hits; this is now fixed. Thanks to F.
Long for reporting the bug.
- Modified deltaCn calculation for lower hits for pin, pep.xml, and txt
outputs. Instead of reporting the deltaCn values as they would have been
historically shown in the sqt/out formats, the deltaCn for lower hits is
calculated as the normalized xcorr difference for that hit and the next hit.
- Extend the maximum possible number of spawned threads to 128.

#### release 2017.01 rev. 3 (2017.01.3), release date 2018/01/05
- Bug fix: Corrected issue where Percolator .pin output "label" column was
always set to "1" when "decoy_search = 0". Thanks to F. Long and J. Smith for
reporting this issue.
- Implementation change: Percolator .pin output format now respects the
"num_output_lines" parameter; it previously only reported the top hit. Thanks
to S. Ting for the feature request.

#### release 2017.01 rev. 2 (2017.01.2), release date 2017/11/08
- Bug fix: in SQT, text and Percolator outputs, static n-term and c-term
modification were being reported in the peptide string. These are no longer
being reported and the peptide string now correctly only includes variable
modification mass differences if present.
- Bug fix: in SQT output, a third entry in the L protein lines is an integer
number representing the start position of the peptide in the protein sequence.
The first L entry reported the correct start position but the position was off
by one in subsequent duplicate proteins. This is corrected. Thanks P. Wilmarth
for reporting both of these issues.
- Known bug: in the Percolator .pin output file, the entry "label" for a decoy
protein match is not set to -1 when "decoy_search = 0". This is an artifact of
the new protein handling (tracking/reporting all protein hits). The next
release will address this bug but if you would like a fix sooner, send me an
email and I'll get you a patched binary.
- Known implementation issue: the Percolator .pin output format only exports
the top hit for each spectrum query. Per request, it has been updated to
respect the "num_output_lines" parameter which will be available in the next
release.

#### release 2017.01 rev. 1 (2017.01.1), release date 2017/10/17
- Report all duplicate proteins as a comma separated list under the
"proteinId1" column in Percolator output ("output_percolatorfile"); rev. 0
release reported just a single protein identifier.
- Bug fix: a decoy peptide that appears multiple times within a decoy protein
would previously cause that decoy protein to be reported multiple times.
- Bug fix: add a missing comma separating protein list in text output
("output_txtfile") when performing a combined target-decoy search ("decoy_search
= 2"). The missing comma was between the target protein list and the decoy
protein list.

#### release 2017.01 rev. 0 (2017.01.0), release date 2017/10/02
- Comet now tracks and reports all duplicate protein entries. The residues I
and L treated as being the same/equivalent. You can turn off I/L equivalence by
adding an undocumented/hidden parameter "equal_I_and_L = 0".
- Major update to code to add
[PSI's Extended Fasta Format (PEFF)](https://www.psidev.info/peff) support. Comet
currently supports the ModResPsi and VariantSimple keywords only. This enables
one to search annotated variable modifications and amino acid substitutions
(aka variants). The ModResPsi modifications can be analyzed in conjunction with
the standard Comet variable modifications.
  - Note that the controls for Comet's standard variable modifications (such as
binary mods, force modification, etc.) only apply to the standard variable
modifications and not the PEFF mods.
  - For the VariantSimple amino acid substitutions, Comet will currently only
allow a single amino acid substitution in a peptide at a time.
  - PSI-Mod OBO entries must have "xref: DiffMono:". UniMod OBO entries must
have "xref: delta_mono_mass:". Entries without these values are ignored.
  - Addition of **source="peff"** or **source="param"** attributes to the
**mod_aminoacid_mass** element in the pepXML output.
  - Addition of **id** attribute, referencing the modification ID from the OBO
file, to the **mod_aminoacid_mass** element in the pepXML output.
  - PEFF modifications will not be specified in the "search_summary" element at
the head of each pepXML file.
  - Note that any static mods are *always* applied and variable or PEFF
modification masses are added on top of static mods.
  - When a PEFF modification occurs at the same position as a PEFF variant, the
modification will be considered at that position on both the original residue
as well the variant residue.
  - The .out, .sqt, and .pin output formats currently expect a modification
character (e.g. * or #) to appear in the sequence for each variable
modification. This does not extend well to PEFF modifications so for these
output formats, the peptide string will replace these modification characters
with the bracketed modification mass difference e.g. "K.DLS[79.9663]MK.L"
instead of "K.DLS*MK.L". **This will break many downstream tools that expect
modification characters** but unfortunately there's no way to maintain backwards
compatibility as there are not enough modification characters to encode all
possible PEFF modifications. **Stick with version
[2016.01.3](/Comet/releases/release_201601.html) if you are using
downstream processing tools that cannot handle the new output**.
- In pepXML modification output, **static="mass"** and **variable="mass"** are added
to each "mod_aminoacid_mass" element.
- Add a "peff_format" to control whether or not a PEFF search is performed. For
a PEFF search, his parameter additionally controls whether to search both PEFF
modifications and variants, modifications alone, or variants alone.
- Add a "peff_verbose_output" hidden parameter. This parameter controls whether
or not (default) extra diagnostic information is reported. If set to "1", it
currently warns if a PEFF modification does not have a mass difference entry or
if a PEFF variant is the same residue as in the original sequence. This
parameter is "hidden" in that it is not written to the comet.params file
generated by "comet -p" nor is it present in the example comet.params files
available for download. To invoke this parameter, a user must manually add
"peff_verbose_output = 1" to their comet.params file.
- The parameter remove_precursor_peak" has been extended to handle phosphate
neutral loss peaks. When this parameter is set to "3", the HPO3 and H2PO4
neutral loss peaks are removed from the spectrum prior to analysis.
- Modified the text output, controlled by the "output_txtfile" parameter. The
"protein" column prints out a comma separated list of all protein accessions.
The "duplicate_protein_count" column is now the "protein_count" column which
reports the protein count. The "peptide" column is renamed "modified_peptide";
this column contains the peptide string with modification mass differences in
brackets. Added "peff_modified_peptide" column which contains the peptide
string with OBO identifier in brackets for PEFF modifications in addition to
bracketed mass differences for normal static and variable modifications. This
"peff_modified_peptide" column is present only for PEFF searches. Lastly, added
a "modifications" column; this column was previously present in the
Crux-compiled text output. The "modifications" column indicates position, type
of modification (static, variable, PEFF, PEFF substitution), and mass
difference or original residue. See the parameter page for more information.
- Allow the specification that a variable modification cannot occur on the
C-terminal residue. This is accomplished by setting the 5th parameter entry in
the "variable_mods" parameter to "-2".
- Change Crux compiled text output to also represent mass differences (e.g.
M[15.9949]) instead of actual modified masses (e.g. M[147.0]) in the modified
peptide string. This brings it in line with the standard text output.
Additionally, both standard and Crux versions will now report the mass
differences to four decimal points.
- Modified the options for the "isotope_error" parameter to add more options.
Please see the parameters page for the changes. **Note that if you have been
using "isotope_error = 1" in previous releases, the closest setting is now
"isotope_error = 3"**.
- Bug fix: In releases 2015.01.0 through 2016.01.2, any peptide with a variable
modification had its precursor mass calculated with the mass types assigned to
the fragment ions. So if one specified average masses for the precursor and
monoisotopic masses for the fragments, all modified peptides would have
incorrect peptide masses (calculated with monoisotopic mass values). This bug
would not be relevant if both mass types were assigned to the same type i.e.
monoisotopic masses used for both precursor and fragment ions. Thanks to D.
Zhao for identifying the bug.
- Bug fix: In previous releases when the "force modification be present"
parameter option is set (7th parameter entry of "variable_mods" parameter set to
1), Comet could stop permuting modifications for a peptide prematurely. This
has been fixed.
- Change how the "scan_range" parameter is applied. One can set either the
start scan or end scan independently now. For example, "scan_range = 5000 0"
will search all scans starting at 5000. Similarly "scan_range = 0 250" will
search all scans from the first scan to scan 250. Previously, a start scan of 0
would ignore this parameter and a scan range setting of "5000 0" would not
search as the end scan is less than the start scan. This also fixes the issue
where no spectra would be searched if the specified first scan was not the
appropriate scan type (i.e. MS/MS scan for "scan_level = 2"). Reported by A.
Sharma.
- The parameter entry "output_outfiles" is no longer documented online nor
written in the example params files. However, it is still functional if you add
"output_outfiles = 1" manually into your comet.params files. This is a start of
an eventual slow phase out of .out file output support.
