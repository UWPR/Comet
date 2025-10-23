### Search parameters (2025.02)

Comet search parameters are defined here. These are valid for Comet version 2025.02.X.

Parameters for all versions of Comet [can be found here](/Comet/parameters/).
Entries marked with an <font color="red">*</font> are new parameters.
Entries marked with an <font color="red">**</font> are modified parameters.

To generate a comet.params file appropriate for your Comet binary, issue the command "comet -p".

Example comet.params files (primary differences are the MS and MS/MS mass tolerance settings):
- [comet.params.low-low](comet.params.low-low) for low res MS1 and low res MS2 e.g. ion trap
- [comet.params.high-low](comet.params.high-low) high res MS1 and low res MS2 e.g. Velos-Orbitrap
- [comet.params.high-high](comet.params.high-high) high res MS1 and high res MS2 e.g. Q Exactive or Q-Tof

#### Database

- [database_name](database_name.html)
- [decoy_search](decoy_search.html)
- [peff_format](peff_format.html)
- [peff_obo](peff_obo.html)

#### CPU threads

- [num_threads](num_threads.html)

#### Masses

- [peptide_mass_tolerance_lower](peptide_mass_tolerance_lower.html)
- [peptide_mass_tolerance_upper](peptide_mass_tolerance_upper.html)
- [peptide_mass_units](peptide_mass_units.html)
- [mass_type_parent](mass_type_parent.html)
- [mass_type_fragment](mass_type_fragment.html)
- [precursor_tolerance_type](precursor_tolerance_type.html)
- [isotope_error](isotope_error.html)

#### Search enzyme

- [search_enzyme_number](search_enzyme_number.html)
- [search_enzyme2_number](search_enzyme2_number.html)
- [num_enzyme_termini](num_enzyme_termini.html)
- [allowed_missed_cleavage](allowed_missed_cleavage.html)

#### Fragment ions

- [fragment_bin_tol](fragment_bin_tol.html)
- [fragment_bin_offset](fragment_bin_offset.html)
- [theoretical_fragment_ions](theoretical_fragment_ions.html)
- [use_A_ions](use_A_ions.html)
- [use_B_ions](use_B_ions.html)
- [use_C_ions](use_C_ions.html)
- [use_X_ions](use_X_ions.html)
- [use_Y_ions](use_Y_ions.html)
- [use_Z_ions](use_Z_ions.html)
- [use_Z1_ions](use_Z1_ions.html)
- [use_NL_ions](use_NL_ions.html)

#### Fragment-ion index

- [fragindex_max_fragmentmass](fragindex_max_fragmentmass.html)
- [fragindex_min_fragmentmass](fragindex_min_fragmentmass.html)
- [fragindex_min_ions_report](fragindex_min_ions_report.html)
- [fragindex_min_ions_score](fragindex_min_ions_score.html)
- [fragindex_num_spectrumpeaks](fragindex_num_spectrumpeaks.html)
- [fragindex_skipreadprecursors](fragindex_skipreadprecursors.html)


#### Output

- [output_mzidentmlfile](output_mzidentmlfile.html)
- [output_pepxmlfile](output_pepxmlfile.html)
- [output_percolatorfile](output_percolatorfile.html)
- [output_sqtfile](output_sqtfile.html)
- [output_sqtstream](output_sqtstream.html)
- [output_txtfile](output_txtfile.html)
- [output_outfiles](output_outfiles.html)
- [print_expect_score](print_expect_score.html)
- [num_output_lines](num_output_lines.html)
- [show_fragment_ions](show_fragment_ions.html)
- [sample_enzyme_number](sample_enzyme_number.html)

#### mzXML/mzML parameters

- [scan_range](scan_range.html)
- [precursor_charge](precursor_charge.html)
- [override_charge](override_charge.html)
- [ms_level](ms_level.html)
- [activation_method](activation_method.html)

#### Misc. parameters

- [clip_nterm_aa](clip_nterm_aa.html)
- [clip_nterm_methionine](clip_nterm_methionine.html)
- [decoy_prefix](decoy_prefix.html)
- [digest_mass_range](digest_mass_range.html)
- [equal_I_and_L](equal_I_and_L.html)
- [mass_offsets](mass_offsets.html)
- [max_duplicate_proteins](max_duplicate_proteins.html)
- [max_fragment_charge](max_fragment_charge.html)
- [max_index_runtime](max_index_runtime.html)
- [min_precursor_charge](min_precursor_charge.html) <font color="red">*</font>
- [max_precursor_charge](max_precursor_charge.html)
- [minimum_xcorr](minimum_xcorr.html)
- [num_results](num_results.html)
- [nucleotide_reading_frame](nucleotide_reading_frame.html)
- [output_suffix](output_suffix.html)
- [peff_verbose_output](peff_verbose_output.html)
- [peptide_length_range](peptide_length_range.html)
- [precursor_NL_ions](precursor_NL_ions.html)
- [protein_modslist_file](protein_modslist_file.html)
- [skip_researching](skip_researching.html)
- [spectrum_batch_size](spectrum_batch_size.html)
- [text_file_extension](text_file_extension.html)
- [explicit_deltacn](explicit_deltacn.html)
- [old_mods_encoding](old_mods_encoding.html)
- [export_additional_pepxml_scores](export_additional_pepxml_scores.html)
- [resolve_fullpaths](resolve_fullpaths.html)
- [pinfile_protein_delimiter](pinfile_protein_delimiter.html)
- [set_X_residue](set_X_residue.html) aka set_A_residue through set_Z_residue

#### Spectral processing

- [minimum_peaks](minimum_peaks.html)
- [minimum_intensity](minimum_intensity.html)
- [remove_precursor_peak](remove_precursor_peak.html)
- [remove_precursor_tolerance](remove_precursor_tolerance.html)
- [clear_mz_range](clear_mz_range.html)

#### Variable modifications

- [variable_mod01 thru variable_mod15](variable_modXX.html)
- [max_variable_mods_in_peptide](max_variable_mods_in_peptide.html)
- [require_variable_mod](require_variable_mod.html)
- [scale_fragmentNL](scale_fragmentNL.html)
- [protein_modlist_file](protein_modlist_file.html)

#### Static modifications

- [add_Cterm_peptide](add_Cterm_peptide.html)
- [add_Nterm_peptide](add_Nterm_peptide.html)
- [add_Cterm_protein](add_Cterm_protein.html)
- [add_Nterm_protein](add_Nterm_protein.html)
- [add_G_glycine](add_G_glycine.html)
- [add_A_alanine](add_A_alanine.html)
- [add_S_serine](add_S_serine.html)
- [add_P_proline](add_P_proline.html)
- [add_V_valine](add_V_valine.html)
- [add_T_threonine](add_T_threonine.html)
- [add_C_cysteine](add_C_cysteine.html)
- [add_L_leucine](add_L_leucine.html)
- [add_I_isoleucine](add_I_isoleucine.html)
- [add_N_asparagine](add_N_asparagine.html)
- [add_D_aspartic_acid](add_D_aspartic_acid.html)
- [add_Q_glutamine](add_Q_glutamine.html)
- [add_K_lysine](add_K_lysine.html)
- [add_E_glutamic_acid](add_E_glutamic_acid.html)
- [add_M_methionine](add_M_methionine.html)
- [add_H_histidine](add_H_histidine.html)
- [add_F_phenylalanine](add_F_phenylalanine.html)
- [add_U_selenocysteine](add_U_selenocysteine.html)
- [add_R_arginine](add_R_arginine.html)
- [add_Y_tyrosine](add_Y_tyrosine.html)
- [add_W_tryptophan](add_W_tryptophan.html)
- [add_O_pyrrolysine](add_O_pyrrolysine.html)
- [add_B_user_amino_acid](add_B_user_amino_acid.html)
- [add_J_user_amino_acid](add_J_user_amino_acid.html)
- [add_X_user_amino_acid](add_X_user_amino_acid.html)
- [add_Z_user_amino_acid](add_Z_user_amino_acid.html)
