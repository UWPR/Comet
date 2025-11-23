// Copyright 2023 Jimmy Eng
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "Common.h"
#include "CometData.h"
#include "CometDataInternal.h"
#include "CometInterfaces.h"
#include "githubsha.h"
#include <unordered_map>

using namespace CometInterfaces;

void Usage(char *pszCmd);
void ProcessCmdLine(int argc,
                    char *argv[],
                    char *szParamsFile,
                    vector<InputFileInfo*> &pvInputFiles,
                    ICometSearchManager *pSearchMgr);
void SetOptions(char *arg,
                char *szParamsFile,
                int *iPrintParams,
                ICometSearchManager *pSearchMgr);
void LoadParameters(char *pszParamsFile,
                    ICometSearchManager *pSearchMgr);
void PrintParams(int iPrintParams);
bool ValidateInputFile(char *pszInputFileName);


int main(int argc, char *argv[])
{
   // add git hash to version string if present
   if (strlen(GITHUBSHA) > 0)
   {
      string sTmp = std::string(GITHUBSHA);
      if (sTmp.size() > 7)
         sTmp.resize(7);
      g_sCometVersion = std::string(comet_version) + " (" + sTmp + ")";
   }
   else
      g_sCometVersion = std::string(comet_version);

   if (argc < 2)
      Usage(argv[0]);

   vector<InputFileInfo*> pvInputFiles;
   ICometSearchManager* pCometSearchMgr = GetCometSearchManager();
   char szParamsFile[SIZE_FILE];

   ProcessCmdLine(argc, argv, szParamsFile, pvInputFiles, pCometSearchMgr);
   pCometSearchMgr->AddInputFiles(pvInputFiles);

   bool bSearchSucceeded;

   bSearchSucceeded = pCometSearchMgr->DoSearch();

   CometInterfaces::ReleaseCometSearchManager();

   if (!bSearchSucceeded)
   {
      // We already log errors when search fails, so no need to log the
      // error message again via g_cometStatus
      exit(1);
   }

   return (0);
} // main


void Usage(char *pszCmd)
{
   char szTmp[1024];
   int iSize = sizeof(szTmp);

   logout("\n");
   snprintf(szTmp, iSize, " Comet version \"%s\"\n %s\n", g_sCometVersion.c_str(), copyright);
   logout(szTmp);
   logout("\n");
   snprintf(szTmp, iSize, " Comet usage:  %s [options] <input_files>\n", pszCmd);
   logout(szTmp);
   logout("\n");
   logout(" Supported input formats include mzXML, mzML, Thermo raw, mgf, and ms2 variants (cms2, bms2, ms2)\n");

   logout("\n");
   logout("       options:  -p         to print out a comet.params.new file\n");
   logout("                 -q         to print out a comet.params.new file with more parameter entries\n");
   logout("                 -P<params> to specify an alternate parameters file (default comet.params)\n");
   logout("                 -N<name>   to specify an alternate output base name; valid only with one input file\n");
   logout("                 -D<dbase>  to specify a sequence database, overriding entry in parameters file\n");
   logout("                 -F<num>    to specify the first/start scan to search, overriding entry in parameters file\n");
   logout("                 -L<num>    to specify the last/end scan to search, overriding entry in parameters file\n");
   logout("                            (-L option is required if -F option is used)\n");
   logout("                 -i         create .idx file for fragment ion indexing\n");
   logout("                 -j         create .idx file for peptide indexing\n");
   logout("\n");
   snprintf(szTmp, iSize, "       example:  %s file1.mzXML file2.mzXML\n", pszCmd);
   logout(szTmp);
   snprintf(szTmp, iSize, "            or   %s -F1000 -L1500 file1.mzXML    <- to search scans 1000 through 1500\n", pszCmd);
   logout(szTmp);
   snprintf(szTmp, iSize, "            or   %s -PParams.txt *.mzXML         <- use parameters in the file 'Params.txt'\n", pszCmd);
   logout(szTmp);

   logout("\n");

   exit(1);
}


void SetOptions(char *arg,
      char *szParamsFile,
      int *iPrintParams,
      ICometSearchManager *pSearchMgr)
{
   char szTmp[512];
   char szParamStringVal[512];
   int iSize = sizeof(szParamStringVal);

   switch (arg[1])
   {
      case 'D':   // Alternate sequence database.
         strncpy(szTmp, arg+2, 511);
         szTmp[511]='\0';

         if (strlen(szTmp) == 0 )
            logout("Missing text for parameter option -D<database>.  Ignored.\n");
         else
            pSearchMgr->SetParam("database_name", szTmp, szTmp);
         break;
      case 'P':   // Alternate parameters file.
         strncpy(szTmp, arg+2, 511);
         szTmp[511]='\0';

         if (strlen(szTmp) == 0 )
            logout("Missing text for parameter option -P<params>.  Ignored.\n");
         else
            strcpy(szParamsFile, szTmp);
         break;
      case 'N':   // Set basename of output file (for .out, SQT, and pepXML)
         strncpy(szTmp, arg+2, 511);
         szTmp[511]='\0';

         if (strlen(szTmp) == 0 )
            logout("Missing text for parameter option -N<basename>.  Ignored.\n");
         else
            pSearchMgr->SetOutputFileBaseName(szTmp);
         break;
      case 'F':   // first scan
         if (sscanf(arg+2, "%511s", szTmp) == 0 )
            logout("Missing text for parameter option -F<num>.  Ignored.\n");
         else
         {
            IntRange iScanRange;
            pSearchMgr->GetParamValue("scan_range", iScanRange);
            iScanRange.iStart = atoi(szTmp);
            szParamStringVal[0] = '\0';
            snprintf(szParamStringVal, iSize, "%d %d", iScanRange.iStart, iScanRange.iEnd);
            pSearchMgr->SetParam("scan_range", szParamStringVal, iScanRange);
         }
         break;
      case 'L':  // last scan
         if (sscanf(arg+2, "%511s", szTmp) == 0 )
            logout("Missing text for parameter option -L<num>.  Ignored.\n");
         else
         {
            IntRange iScanRange;
            pSearchMgr->GetParamValue("scan_range", iScanRange);
            iScanRange.iEnd = atoi(szTmp);
            szParamStringVal[0] = '\0';
            snprintf(szParamStringVal, iSize, "%d %d", iScanRange.iStart, iScanRange.iEnd);
            pSearchMgr->SetParam("scan_range", szParamStringVal, iScanRange);
         }
         break;
      case 'B':  // batch size
         if (sscanf(arg+2, "%511s", szTmp) == 0 )
            logout("Missing text for parameter option -B<num>.  Ignored.\n");
         else
         {
            int iSpectrumBatchSize = atoi(szTmp);
            szParamStringVal[0] = '\0';
            snprintf(szParamStringVal, iSize, "%d", iSpectrumBatchSize);
            pSearchMgr->SetParam("spectrum_batch_size", szParamStringVal, iSpectrumBatchSize);
         }
         break;
      case 'p':
         *iPrintParams = 1;  // default set of parameters
         break;
      case 'q':
         *iPrintParams = 2;  // include additional parameters such as PEFF, fragment ion index
         break;
      case 'i':
         snprintf(szParamStringVal, iSize, "1");
         pSearchMgr->SetParam("create_fragment_index", szParamStringVal, 1);
         snprintf(szParamStringVal, iSize, "0");
         pSearchMgr->SetParam("create_peptide_index", szParamStringVal, 0);
         break;
      case 'j':
         snprintf(szParamStringVal, iSize, "0");
         pSearchMgr->SetParam("create_fragment_index", szParamStringVal, 0);
         snprintf(szParamStringVal, iSize, "1");
         pSearchMgr->SetParam("create_peptide_index", szParamStringVal, 1);
         break;
      default:
         break;
   }
}


// Reads comet.params parameter file.
void LoadParameters(char* pszParamsFile,
                    ICometSearchManager* pSearchMgr)
{
   double dTempMass;
   int iSearchEnzymeNumber = 1,
       iSearchEnzyme2Number = 0,
       iSampleEnzymeNumber = 1,
       iAllowedMissedCleavages = 2;
   char szParamBuf[SIZE_BUF],
        szParamName[128],
        szParamVal[512],
        szParamStringVal[512],
        szVersion[128],
        szErrorMsg[512];
   FILE* fp;
   bool bCurrentParamsFile = 0, bValidParamsFile;
   char* pStr;
   VarMods varModsParam;
   IntRange intRangeParam;
   DoubleRange doubleRangeParam;

   int iSize = sizeof(szParamStringVal);

   if ((fp = fopen(pszParamsFile, "r")) == NULL)
   {
      string strErrorMsg = "\n Comet version " +  g_sCometVersion + "\n\n"
         + " Error - cannot open parameter file \"" + std::string(pszParamsFile) + "\".\n";
      logerr(strErrorMsg);
      exit(1);
   }

   // Validate params file version
   strcpy(szVersion, "unknown");
   bValidParamsFile = false;
   while (!feof(fp))
   {
      if (fgets(szParamBuf, SIZE_BUF, fp) != NULL)
      {
         if (!strncmp(szParamBuf, "# comet_version ", 16))
         {
            char szRev1[12], szRev2[12];
            std::sscanf(szParamBuf, "%*s %*s %127s %11s %11s", szVersion, szRev1, szRev2);

            if (pSearchMgr->IsValidCometVersion(std::string(szVersion)))
            {
               bValidParamsFile = true;
               char szVersion2[128];
               std::sprintf(szVersion2, "%.100s %.11s %.11s", szVersion, szRev1, szRev2);
               std::strcpy(szVersion, szVersion2);
               pSearchMgr->SetParam("# comet_version", szVersion, szVersion);
               break;
            }
         }
      }
   }

   if (!bValidParamsFile)
   {
      string strErrorMsg = "\n Comet version " + g_sCometVersion + "\n\n"
         + " The comet.params file is from version " + std::string(szVersion) + "\n"
         + " Please update your comet.params file.  You can generate\n"
         + " a new parameters file using \"comet -p\"\n\n";
      logerr(strErrorMsg);
      exit(1);
   }

   rewind(fp);

   // Helper lambdas for common parameter types
   auto parse_int = [&](const char* paramName) {
      int val = 0;
      sscanf(szParamVal, "%d", &val);
      snprintf(szParamStringVal, iSize, "%d", val);
      pSearchMgr->SetParam(paramName, szParamStringVal, val);
   };
   auto parse_double = [&](const char* paramName) {
      double val = 0;
      sscanf(szParamVal, "%lf", &val);
      snprintf(szParamStringVal, iSize, "%lf", val);
      pSearchMgr->SetParam(paramName, szParamStringVal, val);
   };
   auto parse_long = [&](const char* paramName) {
      long val = 0;
      sscanf(szParamVal, "%ld", &val);
      snprintf(szParamStringVal, iSize, "%ld", val);
      pSearchMgr->SetParam(paramName, szParamStringVal, val);
   };
   auto parse_string = [&](const char* paramName, int maxLen = 255) {
      char buf[512] = { 0 };
      char fmt[16];
      snprintf(fmt, sizeof(fmt), "%%%ds", maxLen);
      sscanf(szParamVal, fmt, buf);
      pSearchMgr->SetParam(paramName, buf, buf);
   };
   auto parse_int_range = [&](const char* paramName) {
      IntRange val = { 0,0 };
      sscanf(szParamVal, "%d %d", &val.iStart, &val.iEnd);
      snprintf(szParamStringVal, iSize, "%d %d", val.iStart, val.iEnd);
      pSearchMgr->SetParam(paramName, szParamStringVal, val);
   };
   auto parse_double_range = [&](const char* paramName) {
      DoubleRange val = { 0.0,0.0 };
      sscanf(szParamVal, "%lf %lf", &val.dStart, &val.dEnd);
      snprintf(szParamStringVal, iSize, "%lf %lf", val.dStart, val.dEnd);
      pSearchMgr->SetParam(paramName, szParamStringVal, val);
   };

   // Remove whitespace lambda
   auto trim_whitespace = [](char* buf) {
      int iLen = (int)strlen(buf);
      char* szTrimmed = buf;
      while (iLen > 0 && isspace(szTrimmed[iLen - 1]))
         szTrimmed[--iLen] = 0;
      while (*szTrimmed && isspace(*szTrimmed))
      {
         ++szTrimmed;
         --iLen;
      }
      memmove(buf, szTrimmed, iLen + 1);
   };

   struct ParamEntry {
      std::function<void()> handler;
   };

   std::unordered_map<std::string, ParamEntry> paramHandlers =
   {
      // String with whitespace trim
      {"database_name",                { [&]() { trim_whitespace(szParamVal); char szFile[SIZE_FILE]; strcpy(szFile, szParamVal); pSearchMgr->SetParam("database_name", szFile, szFile); }}},
      {"peff_obo",                     { [&]() { trim_whitespace(szParamVal); char szFile[SIZE_FILE]; strcpy(szFile, szParamVal); pSearchMgr->SetParam("peff_obo", szFile, szFile); }}},
      {"spectral_library_name",        { [&]() { trim_whitespace(szParamVal); char szFile[SIZE_FILE]; strcpy(szFile, szParamVal); pSearchMgr->SetParam("spectral_library_name", szFile, szFile); }}},
      // Simple strings
      {"activation_method",            { [&]() { parse_string("activation_method", 23); }}},
      {"decoy_prefix",                 { [&]() { parse_string("decoy_prefix", 255); }}},
      {"output_suffix",                { [&]() { parse_string("output_suffix", 255); }}},
      {"pinfile_protein_delimiter",    { [&]() { parse_string("pinfile_protein_delimiter", 255); }}},
      {"protein_modslist_file",        { [&]() { parse_string("protein_modslist_file", 255); }}},
      {"text_file_extension",          { [&]() { parse_string("text_file_extension", 255); }}},
      // Integers
      {"allowed_missed_cleavage",      { [&]() { parse_int("allowed_missed_cleavage"); sscanf(szParamVal, "%d", &iAllowedMissedCleavages); }}},
      {"clip_nterm_aa",                { [&]() { parse_int("clip_nterm_aa"); }}},
      {"clip_nterm_methionine",        { [&]() { parse_int("clip_nterm_methionine"); }}},
      {"correct_mass",                 { [&]() { parse_int("correct_mass"); }}},
      {"decoy_search",                 { [&]() { parse_int("decoy_search"); }}},
      {"equal_I_and_L",                { [&]() { parse_int("equal_I_and_L"); }}},
      {"explicit_deltacn",             { [&]() { parse_int("explicit_deltacn"); }}},
      {"export_additional_pepxml_scores", { [&]() { parse_int("export_additional_pepxml_scores"); }}},
      {"fragindex_min_ions_report",    { [&]() { parse_int("fragindex_min_ions_report"); }}},
      {"fragindex_min_ions_score",     { [&]() { parse_int("fragindex_min_ions_score"); }}},
      {"fragindex_num_spectrumpeaks",  { [&]() { parse_int("fragindex_num_spectrumpeaks"); }}},
      {"fragindex_skipreadprecursors" ,{ [&]() { parse_int("fragindex_skipreadprecursors"); }}},
      {"isotope_error",                { [&]() { parse_int("isotope_error"); }}},
      {"mango_search",                 { [&]() { parse_int("mango_search"); }}},
      {"mass_type_fragment",           { [&]() { parse_int("mass_type_fragment"); }}},
      {"mass_type_parent",             { [&]() { parse_int("mass_type_parent"); }}},
      {"max_duplicate_proteins",       { [&]() { parse_int("max_duplicate_proteins"); }}},
      {"max_fragment_charge",          { [&]() { parse_int("max_fragment_charge"); }}},
      {"max_precursor_charge",         { [&]() { parse_int("max_precursor_charge"); }}},
      {"max_variable_mods_in_peptide", { [&]() { parse_int("max_variable_mods_in_peptide"); }}},
      {"min_precursor_charge",         { [&]() { parse_int("min_precursor_charge"); }}},
      {"minimum_peaks",                { [&]() { parse_int("minimum_peaks"); }}},
      {"ms_level",                     { [&]() { parse_int("ms_level"); }}},
      {"nucleotide_reading_frame",     { [&]() { parse_int("nucleotide_reading_frame"); }}},
      {"num_enzyme_termini",           { [&]() { parse_int("num_enzyme_termini"); }}},
      {"num_output_lines",             { [&]() { parse_int("num_output_lines"); }}},
      {"num_results",                  { [&]() { parse_int("num_results"); }}},
      {"num_threads",                  { [&]() { parse_int("num_threads"); }}},
      {"old_mods_encoding",            { [&]() { parse_int("old_mods_encoding"); }}},
      {"output_mzidentmlfile",         { [&]() { parse_int("output_mzidentmlfile"); }}},
      {"output_pepxmlfile",            { [&]() { parse_int("output_pepxmlfile"); }}},
      {"output_percolatorfile",        { [&]() { parse_int("output_percolatorfile"); bCurrentParamsFile = 1; }}},
      {"output_sqtfile",               { [&]() { parse_int("output_sqtfile"); }}},
      {"output_sqtstream",             { [&]() { parse_int("output_sqtstream"); }}},
      {"output_txtfile",               { [&]() { parse_int("output_txtfile"); }}},
      {"override_charge",              { [&]() { parse_int("override_charge"); }}},
      {"peff_format",                  { [&]() { parse_int("peff_format"); }}},
      {"peff_verbose_output",          { [&]() { parse_int("peff_verbose_output"); }}},
      {"peptide_mass_units",           { [&]() { parse_int("peptide_mass_units"); }}},
      {"precursor_tolerance_type",     { [&]() { parse_int("precursor_tolerance_type"); }}},
      {"print_expect_score",           { [&]() { parse_int("print_expect_score"); }}},
      {"print_ascorepro_score",        { [&]() { parse_int("print_ascorepro_score"); }}},
      {"remove_precursor_peak",        { [&]() { parse_int("remove_precursor_peak"); }}},
      {"require_variable_mod",         { [&]() { parse_int("require_variable_mod"); }}},
      {"resolve_fullpaths",            { [&]() { parse_int("resolve_fullpaths"); }}},
      {"sample_enzyme_number",         { [&]() { parse_int("sample_enzyme_number"); sscanf(szParamVal, "%d", &iSampleEnzymeNumber); }}},
      {"scale_fragmentNL",             { [&]() { parse_int("scale_fragmentNL"); }}},
      {"search_enzyme2_number",        { [&]() { parse_int("search_enzyme2_number"); sscanf(szParamVal, "%d", &iSearchEnzyme2Number); }}},
      {"search_enzyme_number",         { [&]() { parse_int("search_enzyme_number"); sscanf(szParamVal, "%d", &iSearchEnzymeNumber); }}},
      {"speclib_ms_level",             { [&]() { parse_int("speclib_ms_level"); }}},
      {"spectrum_batch_size",          { [&]() { parse_int("spectrum_batch_size"); }}},
      {"theoretical_fragment_ions",    { [&]() { parse_int("theoretical_fragment_ions"); }}},
      {"use_A_ions",                   { [&]() { parse_int("use_A_ions"); }}},
      {"use_B_ions",                   { [&]() { parse_int("use_B_ions"); }}},
      {"use_C_ions",                   { [&]() { parse_int("use_C_ions"); }}},
      {"use_NL_ions",                  { [&]() { parse_int("use_NL_ions"); }}},
      {"use_X_ions",                   { [&]() { parse_int("use_X_ions"); }}},
      {"use_Y_ions",                   { [&]() { parse_int("use_Y_ions"); }}},
      {"use_Z1_ions",                  { [&]() { parse_int("use_Z1_ions"); }}},
      {"use_Z_ions",                   { [&]() { parse_int("use_Z_ions"); }}},
      {"xcorr_processing_offset",      { [&]() { parse_int("xcorr_processing_offset"); }}},
      // Doubles
      {"add_A_alanine",                { [&]() { parse_double("add_A_alanine"); }}},
      {"add_B_user_amino_acid",        { [&]() { parse_double("add_B_user_amino_acid"); }}},
      {"add_C_cysteine",               { [&]() { parse_double("add_C_cysteine"); }}},
      {"add_Cterm_peptide",            { [&]() { parse_double("add_Cterm_peptide"); }}},
      {"add_Cterm_protein",            { [&]() { parse_double("add_Cterm_protein"); }}},
      {"add_D_aspartic_acid",          { [&]() { parse_double("add_D_aspartic_acid"); }}},
      {"add_E_glutamic_acid",          { [&]() { parse_double("add_E_glutamic_acid"); }}},
      {"add_F_phenylalanine",          { [&]() { parse_double("add_F_phenylalanine"); }}},
      {"add_G_glycine",                { [&]() { parse_double("add_G_glycine"); }}},
      {"add_H_histidine",              { [&]() { parse_double("add_H_histidine"); }}},
      {"add_I_isoleucine",             { [&]() { parse_double("add_I_isoleucine"); }}},
      {"add_J_user_amino_acid",        { [&]() { parse_double("add_J_user_amino_acid"); }}},
      {"add_K_lysine",                 { [&]() { parse_double("add_K_lysine"); }}},
      {"add_L_leucine",                { [&]() { parse_double("add_L_leucine"); }}},
      {"add_M_methionine",             { [&]() { parse_double("add_M_methionine"); }}},
      {"add_N_asparagine",             { [&]() { parse_double("add_N_asparagine"); }}},
      {"add_Nterm_peptide",            { [&]() { parse_double("add_Nterm_peptide"); }}},
      {"add_Nterm_protein",            { [&]() { parse_double("add_Nterm_protein"); }}},
      {"add_O_pyrrolysine",            { [&]() { parse_double("add_O_pyrrolysine"); }}},
      {"add_P_proline",                { [&]() { parse_double("add_P_proline"); }}},
      {"add_Q_glutamine",              { [&]() { parse_double("add_Q_glutamine"); }}},
      {"add_R_arginine",               { [&]() { parse_double("add_R_arginine"); }}},
      {"add_S_serine",                 { [&]() { parse_double("add_S_serine"); }}},
      {"add_T_threonine",              { [&]() { parse_double("add_T_threonine"); }}},
      {"add_U_selenocysteine",         { [&]() { parse_double("add_U_selenocysteine"); }}},
      {"add_V_valine",                 { [&]() { parse_double("add_V_valine"); }}},
      {"add_W_tryptophan",             { [&]() { parse_double("add_W_tryptophan"); }}},
      {"add_X_user_amino_acid",        { [&]() { parse_double("add_X_user_amino_acid"); }}},
      {"add_Y_tyrosine",               { [&]() { parse_double("add_Y_tyrosine"); }}},
      {"add_Z_user_amino_acid",        { [&]() { parse_double("add_Z_user_amino_acid"); }}},
      {"fragindex_max_fragmentmass",   { [&]() { parse_double("fragindex_max_fragmentmass"); }}},
      {"fragindex_min_fragmentmass",   { [&]() { parse_double("fragindex_min_fragmentmass"); }}},
      {"fragment_bin_offset",          { [&]() { parse_double("fragment_bin_offset"); }}},
      {"fragment_bin_tol",             { [&]() { parse_double("fragment_bin_tol"); }}},
      {"minimum_intensity",            { [&]() { parse_double("minimum_intensity"); }}},
      {"minimum_xcorr",                { [&]() { parse_double("minimum_xcorr"); }}},
      {"peptide_mass_tolerance",       { [&]() { parse_double("peptide_mass_tolerance"); }}},
      {"peptide_mass_tolerance_lower", { [&]() { parse_double("peptide_mass_tolerance_lower"); }}},
      {"peptide_mass_tolerance_upper", { [&]() { parse_double("peptide_mass_tolerance_upper"); }}},
      {"percentage_base_peak",         { [&]() { parse_double("percentage_base_peak"); }}},
      {"remove_precursor_tolerance",   { [&]() { parse_double("remove_precursor_tolerance"); }}},
      {"set_A_alanine",                { [&]() { parse_double("set_A_alanine"); }}},
      {"set_B_user_amino_acid",        { [&]() { parse_double("set_B_user_amino_acid"); }}},
      {"set_C_cysteine",               { [&]() { parse_double("set_C_cysteine"); }}},
      {"set_D_aspartic_acid",          { [&]() { parse_double("set_D_aspartic_acid"); }}},
      {"set_E_glutamic_acid",          { [&]() { parse_double("set_E_glutamic_acid"); }}},
      {"set_F_phenylalanine",          { [&]() { parse_double("set_F_phenylalanine"); }}},
      {"set_G_glycine",                { [&]() { parse_double("set_G_glycine"); }}},
      {"set_H_histidine",              { [&]() { parse_double("set_H_histidine"); }}},
      {"set_I_isoleucine",             { [&]() { parse_double("set_I_isoleucine"); }}},
      {"set_J_user_amino_acid",        { [&]() { parse_double("set_J_user_amino_acid"); }}},
      {"set_K_lysine",                 { [&]() { parse_double("set_K_lysine"); }}},
      {"set_L_leucine",                { [&]() { parse_double("set_L_leucine"); }}},
      {"set_M_methionine",             { [&]() { parse_double("set_M_methionine"); }}},
      {"set_N_asparagine",             { [&]() { parse_double("set_N_asparagine"); }}},
      {"set_O_pyrrolysine",            { [&]() { parse_double("set_O_pyrrolysine"); }}},
      {"set_P_proline",                { [&]() { parse_double("set_P_proline"); }}},
      {"set_Q_glutamine",              { [&]() { parse_double("set_Q_glutamine"); }}},
      {"set_R_arginine",               { [&]() { parse_double("set_R_arginine"); }}},
      {"set_S_serine",                 { [&]() { parse_double("set_S_serine"); }}},
      {"set_T_threonine",              { [&]() { parse_double("set_T_threonine"); }}},
      {"set_U_selenocysteine",         { [&]() { parse_double("set_U_selenocysteine"); }}},
      {"set_V_valine",                 { [&]() { parse_double("set_V_valine"); }}},
      {"set_W_tryptophan",             { [&]() { parse_double("set_W_tryptophan"); }}},
      {"set_X_user_amino_acid",        { [&]() { parse_double("set_X_user_amino_acid"); }}},
      {"set_Y_tyrosine",               { [&]() { parse_double("set_Y_tyrosine"); }}},
      {"set_Z_user_amino_acid",        { [&]() { parse_double("set_Z_user_amino_acid"); }}},
      // Long
      { "max_iterations",              { [&]() { parse_long("max_iterations"); }}},
      // Ranges
      {"clear_mz_range",               { [&]() { parse_double_range("clear_mz_range"); }}},
      {"digest_mass_range",            { [&]() { parse_double_range("digest_mass_range"); }}},
      {"ms1_mass_range",               { [&]() { parse_double_range("ms1_mass_range"); }} },
      {"peptide_length_range",         { [&]() { parse_int_range("peptide_length_range"); }}},
      {"precursor_charge",             { [&]() { parse_int_range("precursor_charge"); }}},
      {"scan_range",                   { [&]() { parse_int_range("scan_range"); }}},
      // Special: mass_offsets and precursor_NL_ions (vectors)
      {"mass_offsets",                 { [&]() {
         trim_whitespace(szParamVal);
         char szMassOffsets[512];
         std::vector<double> vectorSetMassOffsets;
         char* tok;
         char delims[] = " \t";
         double dMass;
         strcpy(szMassOffsets, szParamVal);
         tok = strtok(szParamVal, delims);
         while (tok != NULL)
         {
            if (sscanf(tok, "%lf", &dMass) == 1)
            {
               if (dMass >= 0.0)
                  vectorSetMassOffsets.push_back(dMass);
               tok = strtok(NULL, delims);
            }
         }
         sort(vectorSetMassOffsets.begin(), vectorSetMassOffsets.end());
         pSearchMgr->SetParam("mass_offsets", szMassOffsets, vectorSetMassOffsets);
      }}},
      {"precursor_NL_ions",            { [&]() {
          trim_whitespace(szParamVal);
          char szMassOffsets[512];
          std::vector<double> vectorPrecursorNLIons;
          char* tok;
          char delims[] = " \t";
          double dMass;
          strcpy(szMassOffsets, szParamVal);
          tok = strtok(szParamVal, delims);
          while (tok != NULL)
          {
             sscanf(tok, "%lf", &dMass);
             if (dMass >= 0.0)
                vectorPrecursorNLIons.push_back(dMass);
             tok = strtok(NULL, delims);
          }
          sort(vectorPrecursorNLIons.begin(), vectorPrecursorNLIons.end());
          pSearchMgr->SetParam("precursor_NL_ions", szMassOffsets, vectorPrecursorNLIons);
      }}}
   };

   // Main parameter parsing loop
   while (!feof(fp))
   {
      if (fgets(szParamBuf, SIZE_BUF, fp) != NULL)
      {
         if (!strncmp(szParamBuf, "[COMET_ENZYME_INFO]", 19))
            break;
         if ((pStr = strchr(szParamBuf, '#')) != NULL)
            *pStr = 0;
         if ((pStr = strchr(szParamBuf, '=')) != NULL)
         {
            strcpy(szParamVal, pStr + 1);
            *pStr = 0;
            sscanf(szParamBuf, "%127s", szParamName);

            // Handle variable_modNN block
            if (!strncmp(szParamName, "variable_mod", 12) && strlen(szParamName) == 14)
            {
               char szTmp[512], szTmp1[512];
               varModsParam.szVarModChar[0] = '\0';
               varModsParam.iMinNumVarModAAPerMod = 0;
               varModsParam.iMaxNumVarModAAPerMod = 0;
               sscanf(szParamVal, "%lf %31s %d %511s %d %d %d %s",
                  &varModsParam.dVarModMass,
                  varModsParam.szVarModChar,
                  &varModsParam.iBinaryMod,
                  szTmp,
                  &varModsParam.iVarModTermDistance,
                  &varModsParam.iWhichTerm,
                  &varModsParam.iRequireThisMod,
                  szTmp1);

               char* pStr;
               if ((pStr = strchr(szTmp1, ',')))
                  sscanf(szTmp1, "%lf,%lf", &varModsParam.dNeutralLoss, &varModsParam.dNeutralLoss2);
               else
                  sscanf(szTmp1, "%lf", &varModsParam.dNeutralLoss);

               if ((pStr = strchr(szTmp, ',')) == NULL)
                  sscanf(szTmp, "%d", &varModsParam.iMaxNumVarModAAPerMod);
               else
               {
                  *pStr = ' ';
                  sscanf(szTmp, "%d %d", &varModsParam.iMinNumVarModAAPerMod, &varModsParam.iMaxNumVarModAAPerMod);
               }

#ifdef _WIN32
               szParamVal[strlen(szParamVal) - 2] = '\0';
#else
               szParamVal[strlen(szParamVal) - 1] = '\0';
#endif
               snprintf(szParamStringVal, iSize, "%s", szParamVal);
               pSearchMgr->SetParam(szParamName, szParamStringVal, varModsParam);
               continue;
            }

            auto it = paramHandlers.find(szParamName);
            if (it != paramHandlers.end())
            {
               it->second.handler();
            }
            else
            {
               sprintf(szErrorMsg, " Warning - invalid parameter found: %s.  Parameter will be ignored.\n", szParamName);
               logout(szErrorMsg);
            }
         }
      }
   }

   if ((fgets(szParamBuf, SIZE_BUF, fp) == NULL))
   {
      sprintf(szErrorMsg, " Error - cannot fgets a line after expected [COMET_ENZYME_INFO]\n");
      logout(szErrorMsg);
   }

   // Get enzyme specificity.
   char szSearchEnzymeName[ENZYME_NAME_LEN];
   char szSearchEnzyme2Name[ENZYME_NAME_LEN];
   char szSampleEnzymeName[ENZYME_NAME_LEN];
   EnzymeInfo enzymeInformation;

   strcpy(szSearchEnzymeName, "-");
   strcpy(szSearchEnzyme2Name, "-");
   strcpy(szSampleEnzymeName, "-");

   std::string enzymeInfoStrVal;
   while (!feof(fp))
   {
      int iCurrentEnzymeNumber;
      sscanf(szParamBuf, "%d.", &iCurrentEnzymeNumber);
      enzymeInfoStrVal += szParamBuf;

      if (iCurrentEnzymeNumber == iSearchEnzymeNumber)
      {
         sscanf(szParamBuf, "%lf %47s %d %19s %19s\n",
            &dTempMass,
            enzymeInformation.szSearchEnzymeName,
            &enzymeInformation.iSearchEnzymeOffSet,
            enzymeInformation.szSearchEnzymeBreakAA,
            enzymeInformation.szSearchEnzymeNoBreakAA);
      }
      if (iCurrentEnzymeNumber == iSearchEnzyme2Number)
      {
         sscanf(szParamBuf, "%lf %47s %d %19s %19s\n",
            &dTempMass,
            enzymeInformation.szSearchEnzyme2Name,
            &enzymeInformation.iSearchEnzyme2OffSet,
            enzymeInformation.szSearchEnzyme2BreakAA,
            enzymeInformation.szSearchEnzyme2NoBreakAA);
      }
      if (iCurrentEnzymeNumber == iSampleEnzymeNumber)
      {
         sscanf(szParamBuf, "%lf %47s %d %19s %19s\n",
            &dTempMass,
            enzymeInformation.szSampleEnzymeName,
            &enzymeInformation.iSampleEnzymeOffSet,
            enzymeInformation.szSampleEnzymeBreakAA,
            enzymeInformation.szSampleEnzymeNoBreakAA);
      }
      fgets(szParamBuf, SIZE_BUF, fp);
   }
   fclose(fp);

   if (!bCurrentParamsFile)
   {
      string strErrorMsg = "\n Comet version " + g_sCometVersion + "\n\n"
         + " Error - outdated params file; generate an update params file using '-p' option.\n";
      logerr(strErrorMsg);
      exit(1);
   }

   if (!strcmp(enzymeInformation.szSearchEnzymeName, "-"))
   {
      string strErrorMsg = "\n Comet version " + g_sCometVersion + "\n\n"
         + " Error - search_enzyme_number " + std::to_string(iSearchEnzymeNumber) + " is missing definition in params file.\n";
      logerr(strErrorMsg);
      exit(1);
   }

   if (!strcmp(enzymeInformation.szSearchEnzyme2Name, "-"))
   {
      string strErrorMsg = "\n Comet version " + g_sCometVersion + "\n\n"
         + " Error - search_enzyme2_number " + std::to_string(iSearchEnzyme2Number) + " is missing definition in params file.\n";
      logerr(strErrorMsg);
      exit(1);
   }

   if (!strcmp(enzymeInformation.szSampleEnzymeName, "-"))
   {
      string strErrorMsg = "\n Comet version " + g_sCometVersion + "\n\n"
         + " Error - sample_enzyme_number " + std::to_string(iSampleEnzymeNumber) + " is missing definition in params file.\n";
      logerr(strErrorMsg);
      exit(1);
   }

   enzymeInformation.iAllowedMissedCleavage = iAllowedMissedCleavages;
   pSearchMgr->SetParam("[COMET_ENZYME_INFO]", enzymeInfoStrVal, enzymeInformation);
}


// Parses the command line and determines the type of analysis to perform.
bool ParseCmdLine(char *cmd, InputFileInfo *pInputFile, ICometSearchManager *pSearchMgr)
{
   char *tok;
   char *scan;

   pInputFile->iAnalysisType = 0;

   // Get the file name. Because Windows can have ":" in the file path,
   // we can't just use "strtok" to grab the filename.
   int i;
   int iCmdLen = (int)strlen(cmd);
   for (i = 0; i < iCmdLen; ++i)
   {
      if (cmd[i] == ':')
      {
         if ((i + 1) < iCmdLen)
         {
            if (cmd[i+1] != '\\' && cmd[i+1] != '/')
            {
               break;
            }
         }
      }
   }

   strncpy(pInputFile->szFileName, cmd, i);
   pInputFile->szFileName[i] = '\0';
   if (!ValidateInputFile(pInputFile->szFileName))
   {
      return false;
   }

   // Get additional filters.
   scan = strtok(cmd+i, ":\n");

   // Analyze entire file.
   if (scan == NULL)
   {
      IntRange scanRange;
      if (!pSearchMgr->GetParamValue("scan_range", scanRange))
      {
         scanRange.iStart = 0;
         scanRange.iEnd = 0;
      }

      if (scanRange.iStart == 0 && scanRange.iEnd == 0)
      {
         pInputFile->iAnalysisType = AnalysisType_EntireFile;
         return true;
      }
      else
      {
         pInputFile->iAnalysisType = AnalysisType_SpecificScanRange;

         pInputFile->iFirstScan = scanRange.iStart;
         pInputFile->iLastScan = scanRange.iEnd;

         return true;
      }
   }

   // Analyze a portion of the file.
   if (strchr(scan,'-') != NULL)
   {
      pInputFile->iAnalysisType = AnalysisType_SpecificScanRange;
      tok = strtok(scan, "-\n");
      if (tok != NULL)
         pInputFile->iFirstScan = atoi(tok);
      tok = strtok(NULL,"-\n");
      if (tok != NULL)
         pInputFile->iLastScan = atoi(tok);
   }
   else if (strchr(scan,'+') != NULL)
   {
      pInputFile->iAnalysisType = AnalysisType_SpecificScanRange;
      tok = strtok(scan,"+\n");
      if (tok != NULL)
         pInputFile->iFirstScan = atoi(tok);
      tok = strtok(NULL,"+\n");
      if (tok != NULL)
         pInputFile->iLastScan = pInputFile->iFirstScan + atoi(tok);
   }
   else
   {
      pInputFile->iAnalysisType = AnalysisType_SpecificScan;
      pInputFile->iFirstScan = atoi(scan);
      pInputFile->iLastScan = pInputFile->iFirstScan;
   }

   return true;
} // ParseCmdLine


void ProcessCmdLine(int argc,
                    char *argv[],
                    char *szParamsFile,
                    vector<InputFileInfo*> &pvInputFiles,
                    ICometSearchManager *pSearchMgr)
{
   int iPrintParams = false;
   int iStartInputFile = 1;
   char *arg;

   if (iStartInputFile == argc)
   {
      string strErrorMsg = "\n Comet version " + g_sCometVersion + "\n\n"
         + " Error - no input files specified so nothing to do.\n";
      logerr(strErrorMsg);
      exit(1);
   }

   strcpy(szParamsFile, "comet.params");

   arg = argv[iStartInputFile];

   // First process the command line options; do this only to see if an alternate
   // params file is specified before loading params file first.
   while ((iStartInputFile < argc) && (NULL != arg))
   {
      if (arg[0] == '-')
         SetOptions(arg, szParamsFile, &iPrintParams, pSearchMgr);

      arg = argv[++iStartInputFile];
   }

   if (iPrintParams)
   {
      PrintParams(iPrintParams);
      exit(0);
   }

   // Loads search parameters from comet.params file. This has to happen
   // after parsing command line arguments in case alt file is specified.
   LoadParameters(szParamsFile, pSearchMgr);

   // Now go through input arguments again.  Command line options will
   // override options specified in params file.
   iStartInputFile = 1;
   arg = argv[iStartInputFile];
   while ((iStartInputFile < argc) && (NULL != arg))
   {
      if (arg[0] == '-')
      {
         SetOptions(arg, szParamsFile, &iPrintParams, pSearchMgr);
      }
      else if (arg != NULL)
      {
         InputFileInfo *pInputFileInfo = new InputFileInfo();
         if (!ParseCmdLine(arg, pInputFileInfo, pSearchMgr))
         {
            string strErrorMsg = "\n Comet version " + g_sCometVersion + "\n\n"
               + " Error - input file \"" + std::string(pInputFileInfo->szFileName) + "\" not found.\n";
            logerr(strErrorMsg);
            pvInputFiles.clear();
            exit(1);
         }
         pvInputFiles.push_back(pInputFileInfo);
      }
      else
      {
         break;
      }

      arg = argv[++iStartInputFile];
   }
} // ProcessCmdLine


void PrintParams(int iPrintParams)
{
   FILE *fp;

   if ( (fp=fopen("comet.params.new", "w"))==NULL)
   {
      string strErrorMsg = "\n Comet version " + g_sCometVersion + "\n\n"
         + " Error - cannot write file comet.params.new\n";
      logerr(strErrorMsg);
      exit(1);
   }

   fprintf(fp, "# comet_version %s\n\
# Comet MS/MS search engine parameters file.\n\
# Everything following the '#' symbol is treated as a comment.\n", g_sCometVersion.c_str());

   fprintf(fp,
"#\n\
database_name = /some/path/db.fasta\n\
decoy_search = 0                       # 0=no (default), 1=internal decoy concatenated, 2=internal decoy separate\n\
\n\
num_threads = 0                        # 0=poll CPU to set num threads; else specify num threads directly (max %d)\n\n", MAX_THREADS);

   if (iPrintParams == 2)
   {
      fprintf(fp, "\nspectral_library_name = /some/path/speclib.file\n");
      fprintf(fp, "spectral_library_ms_level = 1\n\n");

      fprintf(fp,
"#\n\
# PEFF - PSI Extended FASTA Format\n\
#\n\
peff_format = 0                        # 0=no (normal fasta, default), 1=PEFF PSI-MOD, 2=PEFF Unimod\n\
peff_obo =                             # path to PSI Mod or Unimod OBO file\n\
\n\
#\n\
# fragment ion index; limited to 5 variable mods and up to 5 modified residues per mod\n\
#\n\
fragindex_min_ions_score = 3           # minimum number of matched fragment ion index peaks for scoring\n\
fragindex_min_ions_report = 3          # minimum number of matched fragment ion index peaks for reporting(>= fragindex_min_ions_score)\n\
fragindex_num_spectrumpeaks = 100      # number of peaks from spectrum to use for fragment ion index matching\n\
fragindex_min_fragmentmass = 200.0     # low mass cutoff for fragment ions\n\
fragindex_max_fragmentmass = 2000.0    # high mass cutoff for fragment ions\n\
fragindex_skipreadprecursors = 0       # 0=read precursors to limit fragment ion index, 1=skip reading precursors\n\n");
   }

   fprintf(fp,
"#\n\
# masses\n\
#\n\
peptide_mass_tolerance_upper = 20.0    # upper bound of the precursor mass tolerance\n\
peptide_mass_tolerance_lower = -20.0   # lower bound of the precursor mass tolerance; USUALLY NEGATIVE TO BE LOWER THAN 0\n\
peptide_mass_units = 2                 # 0=amu, 1=mmu, 2=ppm\n\
precursor_tolerance_type = 1           # 0=MH+ (default), 1=precursor m/z; only valid for amu/mmu tolerances\n\
isotope_error = 2                      # 0=off, 1=0/1 (C13 error), 2=0/1/2, 3=0/1/2/3, 4=-1/0/1/2/3, 5=-1/0/1\n");

   if (iPrintParams == 2)
   {
      fprintf(fp,
"mass_type_parent = 1                   # 0=average masses, 1=monoisotopic masses\n\
mass_type_fragment = 1                 # 0=average masses, 1=monoisotopic masses\n");
   }

   fprintf(fp,
"\n\
#\n\
# search enzyme\n\
#\n\
search_enzyme_number = 1               # choose from list at end of this params file\n\
search_enzyme2_number = 0              # second enzyme; set to 0 if no second enzyme\n\
sample_enzyme_number = 1               # specifies the sample enzyme which is possibly different than the one applied to the search;\n\
                                       # used by PeptideProphet to calculate NTT & NMC in pepXML output (default=1 for trypsin).\n\
num_enzyme_termini = 2                 # 1 (semi-digested), 2 (fully digested, default), 8 C-term unspecific , 9 N-term unspecific\n\
allowed_missed_cleavage = 2            # maximum value is 5; for enzyme search\n\
\n\
#\n\
# Up to 15 variable_mod entries are supported for a standard search; manually add additional entries as needed\n\
# format:  <mass> <residues> <0=variable/else binary> <max_mods_per_peptide> <term_distance> <n/c-term> <required> <neutral_loss>\n\
#     e.g. 79.966331 STY 0 3 -1 0 0 97.976896\n\
#\n\
variable_mod01 = 15.9949 M 0 3 -1 0 0 0.0\n\
variable_mod02 = 0.0 X 0 3 -1 0 0 0.0\n\
variable_mod03 = 0.0 X 0 3 -1 0 0 0.0\n\
variable_mod04 = 0.0 X 0 3 -1 0 0 0.0\n\
variable_mod05 = 0.0 X 0 3 -1 0 0 0.0\n");
   if (iPrintParams == 2)
   {
      fprintf(fp,
"variable_mod06 = 0.0 X 0 3 -1 0 0 0.0\n\
variable_mod07 = 0.0 X 0 3 -1 0 0 0.0\n\
variable_mod08 = 0.0 X 0 3 -1 0 0 0.0\n\
variable_mod09 = 0.0 X 0 3 -1 0 0 0.0\n\
variable_mod10 = 0.0 X 0 3 -1 0 0 0.0\n\
variable_mod11 = 0.0 X 0 3 -1 0 0 0.0\n\
variable_mod12 = 0.0 X 0 3 -1 0 0 0.0\n\
variable_mod13 = 0.0 X 0 3 -1 0 0 0.0\n\
variable_mod14 = 0.0 X 0 3 -1 0 0 0.0\n\
variable_mod15 = 0.0 X 0 3 -1 0 0 0.0\n");
   }

   fprintf(fp,
"max_variable_mods_in_peptide = 5\n\
require_variable_mod = 0\n");
   if (iPrintParams == 2)
   {
      fprintf(fp, "protein_modslist_file =                # limit variable mods to subset of specified proteins if this file is specified & present\n");
   }

   fprintf(fp,
"\n\
#\n\
# fragment ions\n\
#\n\
# ion trap ms/ms:  1.0005 tolerance, 0.4 offset (mono masses), theoretical_fragment_ions = 1\n\
# high res ms/ms:    0.02 tolerance, 0.0 offset (mono masses), theoretical_fragment_ions = 0, spectrum_batch_size = 15000\n\
#\n\
fragment_bin_tol = 0.02                # binning to use on fragment ions\n\
fragment_bin_offset = 0.0              # offset position to start the binning (0.0 to 1.0)\n\
theoretical_fragment_ions = 0          # 0=use flanking peaks, 1=M peak only\n\
use_A_ions = 0\n\
use_B_ions = 1\n\
use_C_ions = 0\n\
use_X_ions = 0\n\
use_Y_ions = 1\n\
use_Z_ions = 0\n\
use_Z1_ions = 0\n\
use_NL_ions = 0                        # 0=no, 1=yes to consider NH3/H2O neutral loss peaks\n\
\n\
#\n\
# output\n\
#\n\
output_sqtfile = 0                     # 0=no, 1=yes  write sqt file\n\
output_txtfile = 0                     # 0=no, 1=yes  write tab-delimited txt file\n\
output_pepxmlfile = 1                  # 0=no, 1=yes  write pepXML file\n\
output_mzidentmlfile = 0               # 0=no, 1=yes  write mzIdentML file\n\
output_percolatorfile = 0              # 0=no, 1=yes  write Percolator pin file\n");

   if (iPrintParams == 2)
   {
      fprintf(fp,
"print_expect_score = 1                 # 0=no, 1=yes to replace Sp with expect in out & sqt\n\
print_ascorepro_score = 1              # 0=no, 0 to 5 to localize variable_mod01 to _mod05; -1 to localize all variable mods\n");
   }
 
   fprintf(fp,
"num_output_lines = 5                   # num peptide results to show\n\
\n\
#\n\
# mzXML/mzML/raw file parameters\n\
#\n\
scan_range = 0 0                       # start and end scan range to search; either entry can be set independently\n\
precursor_charge = 0 0                 # precursor charge range to analyze; does not override any existing charge; 0 as 1st entry ignores parameter\n\
override_charge = 0                    # 0=no, 1=override precursor charge states, 2=ignore precursor charges outside precursor_charge range, 3=see online\n\
ms_level = 2                           # MS level to analyze, valid are levels 2 (default) or 3\n\
activation_method = ALL                # activation method; used if activation method set; allowed ALL, CID, ECD, ETD, ETD+SA, PQD, HCD, IRMPD, SID\n\
\n\
#\n\
# misc parameters\n\
#\n\
digest_mass_range = 600.0 5000.0       # MH+ peptide mass range to analyze\n\
peptide_length_range = 5 50            # minimum and maximum peptide length to analyze (default min 1 to allowed max %d)\n",
      MAX_PEPTIDE_LEN);

   if (iPrintParams == 2)
   {
      fprintf(fp,
"pinfile_protein_delimiter =            # blank = default 'tab' delimiter between proteins; enter a char/string to use in place of the tab; Percolator pin output only\n");
      fprintf(fp,
"num_results = 100                      # number of results to store internally for Sp rank only; if Sp rank is not used, set this to num_output_lines\n");
   }

fprintf(fp,
"max_duplicate_proteins = 10            # maximum number of additional duplicate protein names to report for each peptide ID; -1 reports all duplicates\n\
max_fragment_charge = 3                # set maximum fragment charge state to analyze (allowed max %d)\n\
min_precursor_charge = 1               # set minimum precursor charge state to analyze (1 if missing)\n\
max_precursor_charge = 6               # set maximum precursor charge state to analyze (allowed max %d)\n",
      MAX_FRAGMENT_CHARGE,
      MAX_PRECURSOR_CHARGE);

fprintf(fp,
"clip_nterm_methionine = 0              # 0=leave protein sequences as-is; 1=also consider sequence w/o N-term methionine\n\
spectrum_batch_size = 15000            # max. # of spectra to search at a time; 0 to search the entire scan range in one loop\n\
decoy_prefix = DECOY_                  # decoy entries are denoted by this string which is pre-pended to each protein accession\n\
equal_I_and_L = 1                      # 0=treat I and L as different; 1=treat I and L as same\n\
mass_offsets =                         # one or more mass offsets to search (values substracted from deconvoluted precursor mass)\n\
\n\
#\n\
# spectral processing\n\
#\n\
minimum_peaks = 10                     # required minimum number of peaks in spectrum to search (default 10)\n");

fprintf(fp,
"minimum_intensity = 0                 # minimum intensity value to read in\n\
remove_precursor_peak = 0              # 0=no, 1=yes, 2=all charge reduced precursor peaks (for ETD), 3=phosphate neutral loss peaks\n\
remove_precursor_tolerance = 1.5       # +- Da tolerance for precursor removal\n\
clear_mz_range = 0.0 0.0               # clear out all peaks in the specified m/z range e.g. remove reporter ion region of TMT spectra\n\
percentage_base_peak = 0.0             # specify a percentage (e.g. \"0.05\" for 5%%) of the base peak intensity as a minimum intensity threshold\n\
\n\
#\n\
# static modifications\n\
#\n\
add_Cterm_peptide = 0.0\n\
add_Nterm_peptide = 0.0\n\
add_Cterm_protein = 0.0\n\
add_Nterm_protein = 0.0\n\
\n\
add_G_glycine = 0.0000                 # added to G - avg.  57.0513, mono.  57.02146\n\
add_A_alanine = 0.0000                 # added to A - avg.  71.0779, mono.  71.03711\n\
add_S_serine = 0.0000                  # added to S - avg.  87.0773, mono.  87.03203\n\
add_P_proline = 0.0000                 # added to P - avg.  97.1152, mono.  97.05276\n\
add_V_valine = 0.0000                  # added to V - avg.  99.1311, mono.  99.06841\n\
add_T_threonine = 0.0000               # added to T - avg. 101.1038, mono. 101.04768\n\
add_C_cysteine = 57.021464             # added to C - avg. 103.1429, mono. 103.00918\n\
add_L_leucine = 0.0000                 # added to L - avg. 113.1576, mono. 113.08406\n\
add_I_isoleucine = 0.0000              # added to I - avg. 113.1576, mono. 113.08406\n\
add_N_asparagine = 0.0000              # added to N - avg. 114.1026, mono. 114.04293\n\
add_D_aspartic_acid = 0.0000           # added to D - avg. 115.0874, mono. 115.02694\n\
add_Q_glutamine = 0.0000               # added to Q - avg. 128.1292, mono. 128.05858\n\
add_K_lysine = 0.0000                  # added to K - avg. 128.1723, mono. 128.09496\n\
add_E_glutamic_acid = 0.0000           # added to E - avg. 129.1140, mono. 129.04259\n\
add_M_methionine = 0.0000              # added to M - avg. 131.1961, mono. 131.04048\n\
add_H_histidine = 0.0000               # added to H - avg. 137.1393, mono. 137.05891\n\
add_F_phenylalanine = 0.0000           # added to F - avg. 147.1739, mono. 147.06841\n\
add_U_selenocysteine = 0.0000          # added to U - avg. 150.0379, mono. 150.95363\n\
add_R_arginine = 0.0000                # added to R - avg. 156.1857, mono. 156.10111\n\
add_Y_tyrosine = 0.0000                # added to Y - avg. 163.0633, mono. 163.06333\n\
add_W_tryptophan = 0.0000              # added to W - avg. 186.0793, mono. 186.07931\n\
add_O_pyrrolysine = 0.0000             # added to O - avg. 237.2982, mono  237.14773\n\
add_B_user_amino_acid = 0.0000         # added to B - avg.   0.0000, mono.   0.00000\n\
add_J_user_amino_acid = 0.0000         # added to J - avg.   0.0000, mono.   0.00000\n\
add_X_user_amino_acid = 0.0000         # added to X - avg.   0.0000, mono.   0.00000\n\
add_Z_user_amino_acid = 0.0000         # added to Z - avg.   0.0000, mono.   0.00000\n\n");

   if (0)  // do not print these parameters out
   {
fprintf(fp,
"#\n\
# These set_X_residue parameters will override the default AA masses for both precursor and fragment calculations.\n\
# They are applied if the parameter value is not zero.\n\
#\n\
set_G_glycine = 0.0000\n\
set_A_alanine = 0.0000\n\
set_S_serine = 0.0000\n\
set_P_proline = 0.0000\n\
set_V_valine = 0.0000\n\
set_T_threonine = 0.0000\n\
set_C_cysteine = 0.0000\n\
set_L_leucine = 0.0000\n\
set_I_isoleucine = 0.0000\n\
set_N_asparagine = 0.0000\n\
set_D_aspartic_acid = 0.0000\n\
set_Q_glutamine = 0.0000\n\
set_K_lysine = 0.0000\n\
set_E_glutamic_acid = 0.0000\n\
set_M_methionine = 0.0000\n\
set_H_histidine = 0.0000\n\
set_F_phenylalanine = 0.0000\n\
set_U_selenocysteine = 0.0000\n\
set_R_arginine = 0.0000\n\
set_Y_tyrosine = 0.0000\n\
set_W_tryptophan = 0.0000\n\
set_O_pyrrolysine = 0.0000\n\
set_B_user_amino_acid = 0.0000\n\
set_J_user_amino_acid = 0.0000\n\
set_X_user_amino_acid = 0.0000\n\
set_Z_user_amino_acid = 0.0000\n\n");
   }

fprintf(fp,
"#\n\
# COMET_ENZYME_INFO _must_ be at the end of this parameters file\n\
# Enzyme entries can be added/deleted/edited\n\
#\n\
[COMET_ENZYME_INFO]\n\
0.  Cut_everywhere         0      -           -\n\
1.  Trypsin                1      KR          P\n\
2.  Trypsin/P              1      KR          -\n\
3.  Lys_C                  1      K           P\n\
4.  Lys_N                  0      K           -\n\
5.  Arg_C                  1      R           P\n\
6.  Asp_N                  0      DN          -\n\
7.  CNBr                   1      M           -\n\
8.  Asp-N_ambic            1      DE          -\n\
9.  PepsinA                1      FL          -\n\
10. Chymotrypsin           1      FWYL        P\n\
11. No_cut                 1      @           @\n\
\n");

   std::string sTmp = " Comet version \"" + g_sCometVersion + "\"\n " + copyright + "\n\n Created:  comet.params.new\n\n";
   logout(sTmp.c_str());
   fclose(fp);

} // PrintParams


bool ValidateInputFile(char *pszInputFileName)
{
   FILE *fp;
   if ((fp = fopen(pszInputFileName, "r")) == NULL)
   {
      return false;
   }
   fclose(fp);
   return true;
}
