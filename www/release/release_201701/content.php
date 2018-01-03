<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2017.01</h1>
            <ul>
               <b>release 2017.01 rev. 2 (2017.01.2), release date 2017/11/08</b>
               <li>Buf fix: in SQT, text and Percolator outputs, static n-term and c-term modification were
                   being reported in the peptide string.  These are no longer being reported and
                   the peptide string now correctly only includes variable modification mass
                   differences if present.
               <li>Bug fix: in SQT output, a third entry in the L protein lines is an integer number
                   representing the start position of the peptide in the protein sequence.  The first
                   L entry reported the correct start position but the position was off by one in
                   subsequent duplicate proteins.  This is corrected.  Thanks P. Wilmarth for
                   reporting both of these issues.
            </ul>
            <ul>
               <b>release 2017.01 rev. 1 (2017.01.1), release date 2017/10/17</b>
               <li>Report all duplicate proteins as a comma separated list under the "proteinId1" column
                   in Percolator output 
                   (<a href="/parameters/parameters_201701/output_percolatorfile.php">output_percolatorfile</a>);
                   rev. 0 release reported just a single protein identifier.
               <li>Bug fix: a decoy peptide that appears multiple times within a decoy protein
                   would previously cause that decoy protein to be reported multiple times.
               <li>Bug fix: add a missing comma separating protein list in text output
                   (<a href="/parameters/parameters_201701/output_txtfile.php">output_txtfile</a>)
                   when performing a combined target-decoy search
                   ("<a href="/parameters/parameters_201701/decoy_search.php">decoy_search = 2</a>").
                   The missing comma was between the target protein list and the decoy protein list.
            </ul>
            <ul>
               <b>release 2017.01 rev. 0 (2017.01.0), release date 2017/10/02</b>
               <li>Comet now tracks and reports all duplicate protein entries.  The residues I and L
                   treated as being the same/equivalent.  You can turn off I/L equivalence by adding
                   an undocumented/hidden parameter "equal_I_and_L = 0".
               <li>Major update to code to add <a href="http://www.psidev.info/node/363" target="new">PSI's enhanced fasta format (PEFF)</a> support.
                   Comet currently supports the ModResPsi and VariantSimple keywords only.  This
                   enables one to search annotated variable modifications and amino acid substitutions
                   (aka variants).  The ModResPsi modifications can be analyzed in conjunction with the
                   standard Comet variable modifications.
                   <br>&bullet; Note that the controls for Comet's standard
                   variable modifications (such as binary mods, force modification, etc.) only apply
                   to the standard variable modifications and not the PEFF mods.
                   <br>&bullet; For the VariantSimple amino acid substitutions, Comet will currently
                   only allow a single amino acid substitution in a peptide at a time.
                   <br>&bullet; PSI-Mod OBO entries must have "xref: DiffMono:".  UniMod OBO  entries must
                   have "xref: delta_mono_mass:".  Entries without these values are ignored.
                   <br>&bullet; Addition of <tt>source="peff"</tt> or <tt>source="param"</tt> attributes
                   to the <tt>mod_aminoacid_mass</tt> element in the pepXML output.
                   <br>&bullet; Addition of <tt>id</tt> attribute, referencing the modification ID from
                   the OBO file, to the <tt>mod_aminoacid_mass</tt> element in the pepXML output.
                   <br>&bullet; PEFF modifications will not be specified in the "search_summary" element at the
                   head of each pepXML file.
                   <br>&bullet; Note that any static mods are *always* applied and variable or PEFF
                   modification masses are added on top of static mods.
                   <br>&bullet; When a PEFF modification occurs at the same position as a PEFF variant,
                   the modification will be considered at that position on both the original residue
                   as well the variant residue.
                   <br>&bullet; The .out, .sqt, and .pin output formats currently expect a modification
                   character (e.g. * or #) to appear in the sequence for each variable modification.
                   This does not extend well to PEFF modifications so for these output formats, the
                   peptide string will replace these modification characters with the bracketed
                   modification mass difference e.g. "K.DLS[79.9663]MK.L" instead of "K.DLS*MK.L".
                   <b><font color="red">This will break many downstream tools that expect modification characters</font></b>
                   but unfortunately there's no way to maintain backwards compatibility as there
                   are not enough modification characters to encode all possible PEFF modifications.
                   <b><font color="red">Stick with <a href="http://comet-ms.sourceforge.net/release/release_201601/">version 2016.01.3</a>
                   if you are using downstream processing tools that cannot handle the new output.</font></b>
               <li>In pepXML modification output, 'static="<i>mass</i>"' and 'variable="<i>mass</i>'
                   are added to each "mod_aminoacid_mass" element.
               <li>Add a "<a href="/parameters/parameters_201701/peff_format.php">peff_format</a>"
                   to control whether or not a PEFF search is performed.  For a PEFF search, his parameter
                   additionally controls whether to search both PEFF modifications and variants, modifications
                   alone, or variants alone.
               <li>Add a "<a href="/parameters/parameters_201701/peff_verbose_output.php">peff_verbose_output</a>"
                   hidden parameter.  This parameter controls whether or not (default) extra
                   diagnostic information is reported.  If set to "1", it currently warns if
                   a PEFF modification does not have a mass difference entry or if a PEFF variant
                   is the same residue as in the original sequence.  This parameter is "hidden" in
                   that it is not written to the comet.params file generated by "comet -p" nor is it
                   present in the example comet.params files available for download.  To invoke this
                   parameter, a user must manually add "peff_verbose_output = 1" to their comet.params file.
               <li>The parameter <a href="/parameters/parameters_201701/remove_precursor_peak.php">remove_precursor_peak</a>" 
                   has been extended to handle phosphate neutral loss peaks.  When this parameter is
                   set to "3", the HPO3 and H2PO4 neutral loss peaks are removed from the spectrum
                   prior to analysis.
               <li>Modified the text output, controlled by the
                   <a href="/parameters/parameters_201701/output_txtfile.php">output_txtfile</a>" parameter.
                   The "protein" column prints out a comma separated list of all protein accessions.
                   The "duplicate_protein_count" column is now the "protein_count" column which reports
                   the protein count.  The "peptide" column is renamed "modified_peptide";
                   this column contains the peptide string with modification mass differences in brackets.
                   Added "peff_modified_peptide" column which contains the peptide string with
                   OBO identifier in brackets for PEFF modifications in addition to bracketed mass
                   differences for normal static and variable modifications.  This "peff_modified_peptide"
                   column is present only for PEFF searches.  Lastly, added a "modifications" column; this
                   column was previously present in the Crux-compiled text output.  The "modifications"
                   column indicates position, type of modification (static, variable, PEFF, PEFF substitution),
                   and mass difference or original residue.  See the parameter page for more information.
               <li>Allow the specification that a variable modification cannot occur on the C-terminal residue.
                   This is accomplished by setting the 5th parameter entry in the
                   <a href="/parameters/parameters_201701/variable_mod01.php">variable_mods</a>" parameter
                   to "-2".
               <li>Change Crux compiled text output to also represent mass differences (e.g. M[15.9949])
                   instead of actual modified masses (e.g. M[147.0]) in the modified peptide string.
                   This brings it in line with the standard text output.  Additionally, both standard
                   and Crux versions will now report the mass differences to four decimal points.
               <li>Modified the options for the
                   "<a href="/parameters/parameters_201701/isotope_error.php">isotope_error</a>" parameter
                   to add more options.  Please see the parameters page for the changes. <b>**Note that if
                   you have been using "isotope_error = 1" in previous releases, the closest setting
                   is now "isotope_error = 3"</b>.
               <li>Bug fix:  In releases 2015.01.0 through 2016.01.2, any peptide
                   with a variable modification had its precursor mass calculated with the mass types
                   assigned to the fragment ions.  So if one specified average masses for the precursor
                   and monoisotopic masses for the fragments, all modified peptides would have incorrect
                   peptide masses (calculated with monoisotopic mass values).  This bug would not be
                   relevant if both mass types were assigned to the same type i.e. monoisotopic masses
                   used for both precursor and fragment ions.  Thanks to D. Zhao for identifying the bug.
               <li>Bug fix:  In previous releases when the "force modification be present" parameter option
                   is set (7th parameter entry of <a href="/parameters/parameters_201701/variable_mod01.php">variable_mods</a>"
                   parameter set to 1), Comet could stop permuting modifications for a peptide prematurely.
                   This has been fixed.
               <li>Change how the "<a href="/parameters/parameters_201701/scan_range.php">scan_range</a>"
                   parameter is applied. One can set either the start scan
                   or end scan independently now.  For example, "scan_range = 5000 0" will search all scans starting
                   at 5000.  Similarly "scan_range = 0 250" will search all scans from the first scan
                   to scan 250.  Previously, a start scan of 0 would ignore this parameter and a scan range
                   setting of "5000 0" would not search as the end scan is less than the start scan.  This
                   also fixes the issue where no spectra would be searched if the specified first scan
                   was not the appropriate scan type (i.e. MS/MS scan for "scan_level = 2").  Reported by
                   A. Sharma.
               <li>The parameter entry "output_outfiles" is no longer documented online nor written in
                   the example params files.  However, it is still functional if you add
                   "output_outfiles = 1" manually into your comet.params files.  This is a start of an
                   eventual slow phase out of .out file output support.
            </ul>

            <p>Documentation for parameters for release 2017.01
            <a href="/parameters/parameters_201701/">can be found here</a>.

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> page.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
