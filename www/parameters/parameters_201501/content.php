<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Search parameters (2015.01)</h1>
         <p><strong>Comet</strong> search parameters are defined here.  These are valid for
         Comet version 2015.01.x.</p>
         
         <p>Parameters for all versions of Comet <a href="http://comet-ms.sourceforge.net/parameters">can be found here</a>.
         <br>Entries marked with an <font color="red">*</font> are new parameters.
         <br>Entries marked with an <font color="red">**</font> are redefined compared to previous release.</p>
         
         <p>To generate a comet.params file appropriate for your Comet binary, issue the command "comet -p".
         <br>Example comet.params files (primary differences are the MS and MS/MS mass tolerance settings):
         <br>&#9679;&nbsp; <a target="new" href="comet.params.low-low">comet.params.low-low</a> for low res MS1 and low res MS2 e.g. ion trap
         <br>&#9679;&nbsp; <a target="new" href="comet.params.high-low">comet.params.high-low</a> high res MS1 and low res MS2 e.g. Velos-Orbitrap
         <br>&#9679;&nbsp; <a target="new" href="comet.params.high-high">comet.params.high-high</a> high res MS1 and high res MS2 e.g. Q Exactive or Q-Tof

         <p>Here's a link to <a href="combined.php">all parameter descriptions on a single web page</a>.</p>

         <h3>Database</h3>
         <ul>
         <li><a href="database_name.php">database_name</a></li>
         <li><a href="decoy_search.php">decoy_search</a></li>
         </ul>

         <h3>CPU threads</h3>
         <ul>
         <li><a href="num_threads.php">num_threads</a></li>
         </ul>

         <h3>Masses</h3>
         <ul>
         <li><a href="peptide_mass_tolerance.php">peptide_mass_tolerance</a></li>
         <li><a href="peptide_mass_units.php">peptide_mass_units</a></li>
         <li><a href="mass_type_parent.php">mass_type_parent</a></li>
         <li><a href="mass_type_fragment.php">mass_type_fragment</a></li>
         <li><a href="isotope_error.php">isotope_error</a></li>
         </ul>

         <h3>Search enzyme</h3>
         <ul>
         <li><a href="search_enzyme_number.php">search_enzyme_number</a></li>
         <li><a href="num_enzyme_termini.php">num_enzyme_termini</a></li>
         <li><a href="allowed_missed_cleavage.php">allowed_missed_cleavage</a></li>
         </ul>

         <h3>Fragment ions</h3>
         <ul>
         <li><a href="fragment_bin_tol.php">fragment_bin_tol</a></li>
         <li><a href="fragment_bin_offset.php">fragment_bin_offset</a></li>
         <li><a href="theoretical_fragment_ions.php">theoretical_fragment_ions</a></li>
         <li><a href="use_A_ions.php">use_A_ions</a></li>
         <li><a href="use_B_ions.php">use_B_ions</a></li>
         <li><a href="use_C_ions.php">use_C_ions</a></li>
         <li><a href="use_X_ions.php">use_X_ions</a></li>
         <li><a href="use_Y_ions.php">use_Y_ions</a></li>
         <li><a href="use_Z_ions.php">use_Z_ions</a></li>
         <li><a href="use_NL_ions.php">use_NL_ions</a></li>
         <li><a href="use_sparse_matrix.php">use_sparse_matrix</a>
         </ul>

         <h3>Output</h3>
         <ul>
         <li><a href="output_sqtstream.php">output_sqtstream</a></li>
         <li><a href="output_sqtfile.php">output_sqtfile</a></li>
         <li><a href="output_txtfile.php">output_txtfile</a></li>
         <li><a href="output_pepxmlfile.php">output_pepxmlfile</a></li>
         <li><a href="output_percolatorfile.php">output_percolatorfile</a></li>
         <li><a href="output_outfiles.php">output_outfiles</a></li>
         <li><a href="print_expect_score.php">print_expect_score</a></li>
         <li><a href="num_output_lines.php">num_output_lines</a></li>
         <li><a href="show_fragment_ions.php">show_fragment_ions</a></li>
         <li><a href="sample_enzyme_number.php">sample_enzyme_number</a></li>
         </ul>
                        
         <h3>mzXML/mzML parameters</h3>
         <ul>
         <li><a href="scan_range.php">scan_range</a></li>
         <li><a href="precursor_charge.php">precursor_charge</a></li>
         <li><a href="override_charge.php">override_charge</a></li>
         <li><a href="ms_level.php">ms_level</a></li>
         <li><a href="activation_method.php">activation_method</a></li>
         </ul>

         <h3>Misc. parameters</h3>
         <ul>
         <li><a href="digest_mass_range.php">digest_mass_range</a></li>
         <li><a href="num_results.php">num_results</a></li>
         <li><a href="skip_researching.php">skip_researching</a></li>
         <li><a href="max_fragment_charge.php">max_fragment_charge</a></li>
         <li><a href="max_precursor_charge.php">max_precursor_charge</a></li>
         <li><a href="nucleotide_reading_frame.php">nucleotide_reading_frame</a></li>
         <li><a href="clip_nterm_methionine.php">clip_nterm_methionine</a></li>
         <li><a href="spectrum_batch_size.php">spectrum_batch_size</a></li>
         <li><a href="decoy_prefix.php">decoy_prefix</a></li>
         <li><a href="output_suffix.php">output_suffix</a></li>
         </ul>

         <h3>Spectral processing</h3>
         <ul>
         <li><a href="minimum_peaks.php">minimum_peaks</a></li>
         <li><a href="minimum_intensity.php">minimum_intensity</a></li>
         <li><a href="remove_precursor_peak.php">remove_precursor_peak</a></li>
         <li><a href="remove_precursor_tolerance.php">remove_precursor_tolerance</a></li>
         <li><a href="clear_mz_range.php">clear_mz_range</a></li>
         </ul>
                        
         <h3>Variable modifications</h3>
         <ul>
         <li><a href="variable_mod01.php">variable_mod01</a> <font color="red">**</font></li>
         <li><a href="variable_mod02.php">variable_mod02</a> <font color="red">**</font></li>
         <li><a href="variable_mod03.php">variable_mod03</a> <font color="red">**</font></li>
         <li><a href="variable_mod04.php">variable_mod04</a> <font color="red">**</font></li>
         <li><a href="variable_mod05.php">variable_mod05</a> <font color="red">**</font></li>
         <li><a href="variable_mod06.php">variable_mod06</a> <font color="red">**</font></li>
         <li><a href="variable_mod07.php">variable_mod07</a> <font color="red">**</font></li>
         <li><a href="variable_mod08.php">variable_mod08</a> <font color="red">**</font></li>
         <li><a href="variable_mod09.php">variable_mod09</a> <font color="red">**</font></li>
         <li><a href="max_variable_mods_in_peptide.php">max_variable_mods_in_peptide</a></li>
         <li><a href="require_variable_mod.php">require_variable_mod</a> <font color="red">*</font></li>
         </ul>

         <h3>Static modifications</h3>
         <ul>
         <li><a href="add_Cterm_peptide.php">add_Cterm_peptide</a></li>
         <li><a href="add_Nterm_peptide.php">add_Nterm_peptide</a></li>
         <li><a href="add_Cterm_protein.php">add_Cterm_protein</a></li>
         <li><a href="add_Nterm_protein.php">add_Nterm_protein</a></li>
         <li><a href="add_G_glycine.php">add_G_glycine</a></li>
         <li><a href="add_A_alanine.php">add_A_alanine</a></li>
         <li><a href="add_S_serine.php">add_S_serine</a></li>
         <li><a href="add_P_proline.php">add_P_proline</a></li>
         <li><a href="add_V_valine.php">add_V_valine</a></li>
         <li><a href="add_T_threonine.php">add_T_threonine</a></li>
         <li><a href="add_C_cysteine.php">add_C_cysteine</a></li>
         <li><a href="add_L_leucine.php">add_L_leucine</a></li>
         <li><a href="add_I_isoleucine.php">add_I_isoleucine</a></li>
         <li><a href="add_N_asparagine.php">add_N_asparagine</a></li>
         <li><a href="add_D_aspartic_acid.php">add_D_aspartic_acid</a></li>
         <li><a href="add_Q_glutamine.php">add_Q_glutamine</a></li>
         <li><a href="add_K_lysine.php">add_K_lysine</a></li>
         <li><a href="add_E_glutamic_acid.php">add_E_glutamic_acid</a></li>
         <li><a href="add_M_methionine.php">add_M_methionine</a></li>
         <li><a href="add_O_ornithine.php">add_O_ornithine</a></li>
         <li><a href="add_H_histidine.php">add_H_histidine</a></li>
         <li><a href="add_F_phenylalanine.php">add_F_phenylalanine</a></li>
         <li><a href="add_R_arginine.php">add_R_arginine</a></li>
         <li><a href="add_Y_tyrosine.php">add_Y_tyrosine</a></li>
         <li><a href="add_W_tryptophan.php">add_W_tryptophan</a></li>
         <li><a href="add_B_user_amino_acid.php">add_B_user_amino_acid</a></li>
         <li><a href="add_J_user_amino_acid.php">add_J_user_amino_acid</a></li>
         <li><a href="add_U_user_amino_acid.php">add_U_user_amino_acid</a></li>
         <li><a href="add_X_user_amino_acid.php">add_X_user_amino_acid</a></li>
         <li><a href="add_Z_user_amino_acid.php">add_Z_user_amino_acid</a></li>
         </ul>

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
