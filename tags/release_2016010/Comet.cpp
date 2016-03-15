/*
   Copyright 2012 University of Washington

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include "Common.h"
#include "CometData.h"
#include "CometInterfaces.h"

#include <algorithm>

using namespace CometInterfaces;

void Usage(char *pszCmd);
void ProcessCmdLine(int argc,
                    char *argv[],
                    char *szParamsFile,
                    vector<InputFileInfo*> &pvInputFiles,
                    ICometSearchManager *pSearchMgr);
void SetOptions(char *arg,
                char *szParamsFile,
                bool *bPrintParams,
                ICometSearchManager *pSearchMgr);
void LoadParameters(char *pszParamsFile, ICometSearchManager *pSearchMgr);
void PrintParams();
bool ValidateInputMsMsFile(char *pszInputFileName);


int main(int argc, char *argv[])
{
   if (argc < 2)
      Usage(argv[0]);

   vector<InputFileInfo*> pvInputFiles;
   ICometSearchManager* pCometSearchMgr = GetCometSearchManager();
   char szParamsFile[SIZE_FILE];

   ProcessCmdLine(argc, argv, szParamsFile, pvInputFiles, pCometSearchMgr);
   pCometSearchMgr->AddInputFiles(pvInputFiles);

   bool bSearchSucceeded = pCometSearchMgr->DoSearch();

   ReleaseCometSearchManager();

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
   char szErrorMsg[128];

   logout("\n");
   sprintf(szErrorMsg, " Comet version \"%s\"\n %s\n", comet_version, copyright);
   logout(szErrorMsg);
   logout("\n");
   sprintf(szErrorMsg, " Comet usage:  %s [options] <input_files>\n", pszCmd);
   logout(szErrorMsg);
   logout("\n");
   logout(" Supported input formats include mzXML, mzXML, mgf, and ms2 variants (cms2, bms2, ms2)\n");
   logout("\n");
   logout("       options:  -p         to print out a comet.params file (named comet.params.new)\n");
   logout("                 -P<params> to specify an alternate parameters file (default comet.params)\n");
   logout("                 -N<name>   to specify an alternate output base name; valid only with one input file\n");
   logout("                 -D<dbase>  to specify a sequence database, overriding entry in parameters file\n");
   logout("                 -F<num>    to specify the first/start scan to search, overriding entry in parameters file\n");
   logout("                 -L<num>    to specify the last/end scan to search, overriding entry in parameters file\n");
   logout("                            (-L option is required if -F option is used)\n");
   logout("\n");
   sprintf(szErrorMsg, "       example:  %s file1.mzXML file2.mzXML\n", pszCmd);
   logout(szErrorMsg);
   sprintf(szErrorMsg, "            or   %s -F1000 -L1500 file1.mzXML    <- to search scans 1000 through 1500\n", pszCmd);
   logout(szErrorMsg);
   sprintf(szErrorMsg, "            or   %s -pParams.txt *.mzXML         <- use parameters in the file 'Params.txt'\n", pszCmd);
   logout(szErrorMsg);

   logout("\n");

   exit(1);
}


void SetOptions(char *arg,
      char *szParamsFile,
      bool *bPrintParams,
      ICometSearchManager *pSearchMgr)
{
   char szTmp[512];
   char szErrorMsg[512];

   switch (arg[1])
   {
      case 'D':   // Alternate sequence database.
         if (sscanf(arg+2, "%512s", szTmp) == 0)
         {
            sprintf(szErrorMsg, "Cannot read command line database: '%s'.  Ignored.\n", szTmp);
            logerr(szErrorMsg);
         }
         else
            pSearchMgr->SetParam("database_name", szTmp, szTmp);
         break;
      case 'P':   // Alternate parameters file.
         if (sscanf(arg+2, "%512s", szTmp) == 0 )
            logerr("Missing text for parameter option -P<params>.  Ignored.\n");
         else
            strcpy(szParamsFile, szTmp);
         break;
      case 'N':   // Set basename of output file (for .out, SQT, and pepXML)
         if (sscanf(arg+2, "%512s", szTmp) == 0 )
            logerr("Missing text for parameter option -N<basename>.  Ignored.\n");
         else
            pSearchMgr->SetOutputFileBaseName(szTmp);
         break;
      case 'F':
         if (sscanf(arg+2, "%512s", szTmp) == 0 )
            logerr("Missing text for parameter option -F<num>.  Ignored.\n");
         else
         {
            char szParamStringVal[512];
            IntRange iScanRange;
            pSearchMgr->GetParamValue("scan_range", iScanRange);
            iScanRange.iStart = atoi(szTmp);
            szParamStringVal[0] = '\0';
            sprintf(szParamStringVal, "%d %d", iScanRange.iStart, iScanRange.iEnd);
            pSearchMgr->SetParam("scan_range", szParamStringVal, iScanRange);
         }
         break;
      case 'L':
         if (sscanf(arg+2, "%512s", szTmp) == 0 )
            logerr("Missing text for parameter option -L<num>.  Ignored.\n");
         else
         {
            char szParamStringVal[512];
            IntRange iScanRange;
            pSearchMgr->GetParamValue("scan_range", iScanRange);
            iScanRange.iEnd = atoi(szTmp);
            szParamStringVal[0] = '\0';
            sprintf(szParamStringVal, "%d %d", iScanRange.iStart, iScanRange.iEnd);
            pSearchMgr->SetParam("scan_range", szParamStringVal, iScanRange);
         }
         break;
      case 'B':
         if (sscanf(arg+2, "%512s", szTmp) == 0 )
            logerr("Missing text for parameter option -B<num>.  Ignored.\n");
         else
         {
            char szParamStringVal[512];
            int iSpectrumBatchSize = atoi(szTmp);
            szParamStringVal[0] = '\0';
            sprintf(szParamStringVal, "%d", iSpectrumBatchSize);
            pSearchMgr->SetParam("spectrum_batch_size", szParamStringVal, iSpectrumBatchSize);
         }
         break;
      case 'p':
         *bPrintParams = true;
         break;
      default:
         break;
   }
}


// Reads comet.params parameter file.
void LoadParameters(char *pszParamsFile,
      ICometSearchManager *pSearchMgr)
{
   double dTempMass,
          dDoubleParam;
   int   iSearchEnzymeNumber = 1,
         iSampleEnzymeNumber = 1,
         iIntParam,
         iAllowedMissedCleavages = 2;

   char  szParamBuf[SIZE_BUF],
         szParamName[128],
         szParamVal[512],
         szParamStringVal[512],
         szVersion[128],
         szErrorMsg[512];
   FILE  *fp;
   bool  bCurrentParamsFile = 0, // Track a parameter to make sure present.
         bValidParamsFile;
   char *pStr;
   VarMods varModsParam;
   IntRange intRangeParam;
   DoubleRange doubleRangeParam;

   if ((fp=fopen(pszParamsFile, "r")) == NULL)
   {
      sprintf(szErrorMsg, "\n Comet version %s\n\n", comet_version);
      sprintf(szErrorMsg+strlen(szErrorMsg), " Error - cannot open parameter file \"%s\".\n", pszParamsFile);
      logerr(szErrorMsg);
      exit(1);
   }

   // validate not using incompatible params file by checking "# comet_version" in first line of file
   strcpy(szVersion, "unknown");
   bValidParamsFile = false;
   while (!feof(fp))
   {
      if (fgets(szParamBuf, SIZE_BUF, fp) != NULL)
      {
         if (!strncmp(szParamBuf, "# comet_version ", 16))
         {
            char szRev1[12],
                 szRev2[12];

            sscanf(szParamBuf, "%*s %*s %128s %12s %12s", szVersion, szRev1, szRev2);

            if (pSearchMgr->IsValidCometVersion(string(szVersion)))
            {
               bValidParamsFile = true;
               sprintf(szVersion, "%s %s %s", szVersion, szRev1, szRev2);
               pSearchMgr->SetParam("# comet_version ", szVersion, szVersion);
               break;
            }
         }
      }
   }

   if (!bValidParamsFile)
   {
      sprintf(szErrorMsg, "\n Comet version %s\n\n", comet_version);
      sprintf(szErrorMsg+strlen(szErrorMsg), " The comet.params file is from version %s\n", szVersion);
      sprintf(szErrorMsg+strlen(szErrorMsg), " Please update your comet.params file.  You can generate\n");
      sprintf(szErrorMsg+strlen(szErrorMsg), " a new parameters file using \"comet -p\"\n\n");
      logerr(szErrorMsg);
      exit(1);
   }

   rewind(fp);

   // now parse the parameter entries
   while (!feof(fp))
   {
      if (fgets(szParamBuf, SIZE_BUF, fp) != NULL)
      {
         if (!strncmp(szParamBuf, "[COMET_ENZYME_INFO]", 19))
            break;

         if ( (pStr = strchr(szParamBuf, '#')) != NULL)  // take care of comments
            *pStr = 0;

         if ( (pStr = strchr(szParamBuf, '=')) != NULL)
         {
            strcpy(szParamVal, pStr + 1);  // Copy over value.
            *pStr = 0;                     // Null rest of szParamName at equal char.

            sscanf(szParamBuf, "%128s", szParamName);

            if (!strcmp(szParamName, "database_name"))
            {
               char szDatabase[512];

               // Support parsing a database string from params file that
               // includes spaces in the path.

               // Remove white spaces at beginning/end of szParamVal
               int iLen = strlen(szParamVal);
               char *szTrimmed = szParamVal;

               while (isspace(szTrimmed[iLen -1]))  // trim end
                  szTrimmed[--iLen] = 0;
               while (*szTrimmed && isspace(*szTrimmed))  // trim beginning
               {
                  ++szTrimmed;
                  --iLen;
               }

               memmove(szParamVal, szTrimmed, iLen+1);

               strcpy(szDatabase, szParamVal);
               pSearchMgr->SetParam("database_name", szDatabase, szDatabase);
            }
            else if (!strcmp(szParamName, "decoy_prefix"))
            {
               char szDecoyPrefix[256];
               szDecoyPrefix[0] = '\0';
               sscanf(szParamVal, "%256s", szDecoyPrefix);
               pSearchMgr->SetParam("decoy_prefix", szDecoyPrefix, szDecoyPrefix);

            }
            else if (!strcmp(szParamName, "output_suffix"))
            {
               char szOutputSuffix[256];
               szOutputSuffix[0] = '\0';
               sscanf(szParamVal, "%256s", szOutputSuffix);
               pSearchMgr->SetParam("output_suffix", szOutputSuffix, szOutputSuffix);
            }
            else if (!strcmp(szParamName, "mass_offsets"))
            {
               // Remove white spaces at beginning/end of szParamVal
               int iLen = strlen(szParamVal);
               char *szTrimmed = szParamVal;

               while (isspace(szTrimmed[iLen -1]))  // trim end
                  szTrimmed[--iLen] = 0;
               while (*szTrimmed && isspace(*szTrimmed))  // trim beginning
               {
                  ++szTrimmed;
                  --iLen;
               }

               memmove(szParamVal, szTrimmed, iLen+1);

               char szMassOffsets[256];
               vector<double> vectorSetMassOffsets;

               char *tok;
               char delims[] = " \t";  // tokenize by space and tab
               double dMass;

               strcpy(szMassOffsets, szParamVal);  // need to copy as strtok below destroys szParamVal

               tok = strtok(szParamVal, delims);
               while (tok != NULL)
               {
                  sscanf(tok, "%lf", &dMass);
                  if (dMass >= 0.0)
                     vectorSetMassOffsets.push_back(dMass);
                  tok = strtok(NULL, delims);
               }

               sort(vectorSetMassOffsets.begin(), vectorSetMassOffsets.end());

               pSearchMgr->SetParam("mass_offsets", szMassOffsets, vectorSetMassOffsets);
            }
            else if (!strcmp(szParamName, "nucleotide_reading_frame"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("nucleotide_reading_frame", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "mass_type_parent"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("mass_type_parent", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "mass_type_fragment"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("mass_type_fragment", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "show_fragment_ions"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("show_fragment_ions", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "num_threads"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("num_threads", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "clip_nterm_methionine"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("clip_nterm_methionine", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "theoretical_fragment_ions"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("theoretical_fragment_ions", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "use_A_ions"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("use_A_ions", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "use_B_ions"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("use_B_ions", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "use_C_ions"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("use_C_ions", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "use_X_ions"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("use_X_ions", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "use_Y_ions"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("use_Y_ions", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "use_Z_ions"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("use_Z_ions", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "use_NL_ions"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("use_NL_ions", szParamStringVal, iIntParam);
            }
            else if (!strncmp(szParamName, "variable_mod", 12) && strlen(szParamName)==14)
            {
               varModsParam.szVarModChar[0] = '\0';
               sscanf(szParamVal, "%lf %20s %d %d %d %d %d",
                     &varModsParam.dVarModMass,
                     varModsParam.szVarModChar,
                     &varModsParam.iBinaryMod,
                     &varModsParam.iMaxNumVarModAAPerMod,
                     &varModsParam.iVarModTermDistance,
                     &varModsParam.iWhichTerm,
                     &varModsParam.bRequireThisMod);
               sprintf(szParamStringVal, "%lf %s %d %d %d %d %d",
                     varModsParam.dVarModMass,
                     varModsParam.szVarModChar,
                     varModsParam.iBinaryMod,
                     varModsParam.iMaxNumVarModAAPerMod,
                     varModsParam.iVarModTermDistance,
                     varModsParam.iWhichTerm,
                     varModsParam.bRequireThisMod);
               pSearchMgr->SetParam(szParamName, szParamStringVal, varModsParam);
            }
            else if (!strcmp(szParamName, "max_variable_mods_in_peptide"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("max_variable_mods_in_peptide", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "require_variable_mod"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("require_variable_mod", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "fragment_bin_tol"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("fragment_bin_tol", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "fragment_bin_offset"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("fragment_bin_offset", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "peptide_mass_tolerance"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("peptide_mass_tolerance", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "precursor_tolerance_type"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("precursor_tolerance_type", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "peptide_mass_units"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("peptide_mass_units", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "isotope_error"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("isotope_error", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "num_output_lines"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("num_output_lines", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "num_results"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("num_results", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "remove_precursor_peak"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("remove_precursor_peak", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "remove_precursor_tolerance"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("remove_precursor_tolerance", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "clear_mz_range"))
            {
               doubleRangeParam.dStart = 0.0;
               doubleRangeParam.dEnd = 0.0;
               sscanf(szParamVal, "%lf %lf", &doubleRangeParam.dStart, &doubleRangeParam.dEnd);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf %lf", doubleRangeParam.dStart, doubleRangeParam.dEnd);
               pSearchMgr->SetParam("clear_mz_range", szParamStringVal, doubleRangeParam);
            }
            else if (!strcmp(szParamName, "print_expect_score"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("print_expect_score", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "output_sqtstream"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("output_sqtstream", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "output_sqtfile"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("output_sqtfile", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "output_txtfile"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("output_txtfile", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "output_pepxmlfile"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("output_pepxmlfile", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "output_percolatorfile"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("output_percolatorfile", szParamStringVal, iIntParam);

               bCurrentParamsFile = 1;  // this is the new parameter; if this is missing then complain & exit
            }
            else if (!strcmp(szParamName, "output_outfiles"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("output_outfiles", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "skip_researching"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("skip_researching", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "add_Cterm_peptide"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_Cterm_peptide", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_Nterm_peptide"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_Nterm_peptide", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_Cterm_protein"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_Cterm_protein", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_Nterm_protein"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_Nterm_protein", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_G_glycine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_G_glycine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_A_alanine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_A_alanine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_S_serine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_S_serine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_P_proline"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_P_proline", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_V_valine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_V_valine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_T_threonine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_T_threonine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_C_cysteine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_C_cysteine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_U_selenocysteine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_U_celenocysteinee", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_L_leucine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_L_leucine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_I_isoleucine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_I_isoleucine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_N_asparagine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_N_asparagine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_O_ornithine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_O_ornithine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_D_aspartic_acid"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_D_aspartic_acid", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_Q_glutamine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_Q_glutamine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_K_lysine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_K_lysine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_E_glutamic_acid"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_E_glutamic_acid", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_M_methionine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_M_methionine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_H_histidine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_H_histidine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_F_phenylalanine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_F_phenylalanine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_R_arginine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_R_arginine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_Y_tyrosine"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_Y_tyrosine", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_W_tryptophan"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_W_tryptophan", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_B_user_amino_acid"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_B_user_amino_acid", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_J_user_amino_acid"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_J_user_amino_acid", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_X_user_amino_acid"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_X_user_amino_acid", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "add_Z_user_amino_acid"))
            {
               sscanf(szParamVal, "%lf", &dDoubleParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf", dDoubleParam);
               pSearchMgr->SetParam("add_Z_user_amino_acid", szParamStringVal, dDoubleParam);
            }
            else if (!strcmp(szParamName, "search_enzyme_number"))
            {
               sscanf(szParamVal, "%d", &iSearchEnzymeNumber);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iSearchEnzymeNumber);
               pSearchMgr->SetParam("search_enzyme_number", szParamStringVal, iSearchEnzymeNumber);
            }
            else if (!strcmp(szParamName, "sample_enzyme_number"))
            {
               sscanf(szParamVal, "%d", &iSampleEnzymeNumber);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iSampleEnzymeNumber);
               pSearchMgr->SetParam("sample_enzyme_number", szParamStringVal, iSampleEnzymeNumber);
            }
            else if (!strcmp(szParamName, "num_enzyme_termini"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("num_enzyme_termini", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "allowed_missed_cleavage"))
            {
               sscanf(szParamVal, "%d", &iAllowedMissedCleavages);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iAllowedMissedCleavages);
               pSearchMgr->SetParam("allowed_missed_cleavage", szParamStringVal, iAllowedMissedCleavages);
            }
            else if (!strcmp(szParamName, "scan_range"))
            {
               intRangeParam.iStart = 0;
               intRangeParam.iEnd = 0;
               sscanf(szParamVal, "%d %d", &intRangeParam.iStart, &intRangeParam.iEnd);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d %d", intRangeParam.iStart, intRangeParam.iEnd);
               pSearchMgr->SetParam("scan_range", szParamStringVal, intRangeParam);
            }
            else if (!strcmp(szParamName, "spectrum_batch_size"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("spectrum_batch_size", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "minimum_peaks"))
            {
               iIntParam = 0;
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("minimum_peaks", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "precursor_charge"))
            {
               intRangeParam.iStart = 0;
               intRangeParam.iEnd = 0;
               sscanf(szParamVal, "%d %d", &intRangeParam.iStart, &intRangeParam.iEnd);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d %d", intRangeParam.iStart, intRangeParam.iEnd);
               pSearchMgr->SetParam("precursor_charge", szParamStringVal, intRangeParam);
            }
            else if (!strcmp(szParamName, "override_charge"))
            {
               iIntParam = 0;
               sscanf(szParamVal, "%d",  &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("override_charge", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "max_fragment_charge"))
            {
               iIntParam = 0;
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("max_fragment_charge", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "max_precursor_charge"))
            {
               iIntParam = 0;
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("max_precursor_charge", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "digest_mass_range"))
            {
               doubleRangeParam.dStart = 0.0;
               doubleRangeParam.dEnd = 0.0;
               sscanf(szParamVal, "%lf %lf", &doubleRangeParam.dStart, &doubleRangeParam.dEnd);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%lf %lf", doubleRangeParam.dStart, doubleRangeParam.dEnd);
               pSearchMgr->SetParam("digest_mass_range", szParamStringVal, doubleRangeParam);
            }
            else if (!strcmp(szParamName, "ms_level"))
            {
               iIntParam = 0;
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("ms_level", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "activation_method"))
            {
               char szActivationMethod[24];
               szActivationMethod[0] = '\0';
               sscanf(szParamVal, "%24s", szActivationMethod);
               szActivationMethod[23] = '\0';
               pSearchMgr->SetParam("activation_method", szActivationMethod, szActivationMethod);
            }
            else if (!strcmp(szParamName, "minimum_intensity"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("minimum_intensity", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "decoy_search"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("decoy_search", szParamStringVal, iIntParam);
            }
            else if (!strcmp(szParamName, "xcorr_processing_offset"))
            {
               sscanf(szParamVal, "%d", &iIntParam);
               szParamStringVal[0] = '\0';
               sprintf(szParamStringVal, "%d", iIntParam);
               pSearchMgr->SetParam("xcorr_processing_offset", szParamStringVal, iIntParam);
            }
            else
            {
               sprintf(szErrorMsg, " Warning - invalid parameter found: %s.  Parameter will be ignored.\n", szParamName);
               logout(szErrorMsg);
            }
         }
      }
   } // while

   fgets(szParamBuf, SIZE_BUF, fp);

   // Get enzyme specificity.
   char szSearchEnzymeName[ENZYME_NAME_LEN];
   szSearchEnzymeName[0] = '\0';
   char szSampleEnzymeName[ENZYME_NAME_LEN];
   szSampleEnzymeName[0] = '\0';
   EnzymeInfo enzymeInformation;

   strcpy(szSearchEnzymeName, "-");
   strcpy(szSampleEnzymeName, "-");

   string enzymeInfoStrVal;
   while (!feof(fp))
   {
      int iCurrentEnzymeNumber;

      sscanf(szParamBuf, "%d.", &iCurrentEnzymeNumber);
      enzymeInfoStrVal += szParamBuf;

      if (iCurrentEnzymeNumber == iSearchEnzymeNumber)
      {
         sscanf(szParamBuf, "%lf %48s %d %20s %20s\n",
               &dTempMass,
               enzymeInformation.szSearchEnzymeName,
               &enzymeInformation.iSearchEnzymeOffSet,
               enzymeInformation.szSearchEnzymeBreakAA,
               enzymeInformation.szSearchEnzymeNoBreakAA);
      }

      if (iCurrentEnzymeNumber == iSampleEnzymeNumber)
      {
         sscanf(szParamBuf, "%lf %48s %d %20s %20s\n",
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
      char szErrorMsg[256];
      sprintf(szErrorMsg, "\n Comet version %s\n\n", comet_version);
      sprintf(szErrorMsg+strlen(szErrorMsg), " Error - outdated params file; generate an update params file using '-p' option.\n");
      logerr(szErrorMsg);
      exit(1);
   }

   if (!strcmp(enzymeInformation.szSearchEnzymeName, "-"))
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg, "\n Comet version %s\n\n", comet_version);
      sprintf(szErrorMsg+strlen(szErrorMsg), " Error - search enzyme number %d is missing definition in params file.\n", iSearchEnzymeNumber);
      logerr(szErrorMsg);
      exit(1);
   }

   if (!strcmp(enzymeInformation.szSampleEnzymeName, "-"))
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg, "\n Comet version %s\n\n", comet_version);
      sprintf(szErrorMsg+strlen(szErrorMsg), " Error - sample enzyme number %d is missing definition in params file.\n", iSampleEnzymeNumber);
      logerr(szErrorMsg);
      exit(1);
   }

   enzymeInformation.iAllowedMissedCleavage = iAllowedMissedCleavages;
   pSearchMgr->SetParam("[COMET_ENZYME_INFO]", enzymeInfoStrVal, enzymeInformation);

} // LoadParameters


// Parses the command line and determines the type of analysis to perform.
bool ParseCmdLine(char *cmd, InputFileInfo *pInputFile, ICometSearchManager *pSearchMgr)
{
   char *tok;
   char *scan;

   pInputFile->iAnalysisType = 0;

   // Get the file name. Because Windows can have ":" in the file path,
   // we can't just use "strtok" to grab the filename.
   int i;
   int iCmdLen = strlen(cmd);
   for (i=0; i < iCmdLen; i++)
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
   if (!ValidateInputMsMsFile(pInputFile->szFileName))
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

         if (pInputFile->iFirstScan == 0)  // this means iEndScan is specified only
            pInputFile->iFirstScan = 1;    // so start with 1st scan in file

         // but if iEndScan == 0 then only iStartScan is specified; in this
         // case search iStartScan through end of file (handled in CometPreprocess)

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
   bool bPrintParams = false;
   int iStartInputFile = 1;
   char *arg;

   if (iStartInputFile == argc)
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg, "\n Comet version %s\n\n", comet_version);
      sprintf(szErrorMsg+strlen(szErrorMsg), " Error - no input files specified so nothing to do.\n");
      logerr(szErrorMsg);
      exit(1);
   }

   strcpy(szParamsFile, "comet.params");

   arg = argv[iStartInputFile];

   // First process the command line options; do this only to see if an alternate
   // params file is specified before loading params file first.
   while ((iStartInputFile < argc) && (NULL != arg))
   {
      if (arg[0] == '-')
         SetOptions(arg, szParamsFile, &bPrintParams, pSearchMgr);

      arg = argv[++iStartInputFile];
   }

   if (bPrintParams)
   {
      PrintParams();
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
         SetOptions(arg, szParamsFile, &bPrintParams, pSearchMgr);
      }
      else if (arg != NULL)
      {
         InputFileInfo *pInputFileInfo = new InputFileInfo();
         if (!ParseCmdLine(arg, pInputFileInfo, pSearchMgr))
         {
            char szErrorMsg[256];
            sprintf(szErrorMsg, "\n Comet version %s\n\n", comet_version);
            sprintf(szErrorMsg+strlen(szErrorMsg), " Error - input file \"%s\" not found.\n", pInputFileInfo->szFileName);
            logerr(szErrorMsg);
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


void PrintParams(void)
{
   FILE *fp;

   if ( (fp=fopen("comet.params.new", "w"))==NULL)
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg, "\n Comet version %s\n\n", comet_version);
      sprintf(szErrorMsg+strlen(szErrorMsg), " Error - cannot write file comet.params.new\n");
      logerr(szErrorMsg);
      exit(1);
   }

   fprintf(fp, "# comet_version %s\n\
# Comet MS/MS search engine parameters file.\n\
# Everything following the '#' symbol is treated as a comment.\n", comet_version);

   fprintf(fp,
"\n\
database_name = /some/path/db.fasta\n\
decoy_search = 0                       # 0=no (default), 1=concatenated search, 2=separate search\n\
\n\
num_threads = 0                        # 0=poll CPU to set num threads; else specify num threads directly (max %d)\n\
\n", MAX_THREADS);

   fprintf(fp,
"#\n\
# masses\n\
#\n\
peptide_mass_tolerance = 3.00\n\
peptide_mass_units = 0                 # 0=amu, 1=mmu, 2=ppm\n\
mass_type_parent = 1                   # 0=average masses, 1=monoisotopic masses\n\
mass_type_fragment = 1                 # 0=average masses, 1=monoisotopic masses\n\
precursor_tolerance_type = 0           # 0=MH+ (default), 1=precursor m/z; only valid for amu/mmu tolerances\n\
isotope_error = 0                      # 0=off, 1=on -1/0/1/2/3 (standard C13 error), 2= -8/-4/0/4/8 (for +4/+8 labeling)\n\
\n\
#\n\
# search enzyme\n\
#\n\
search_enzyme_number = 1               # choose from list at end of this params file\n\
num_enzyme_termini = 2                 # 1 (semi-digested), 2 (fully digested, default), 8 C-term unspecific , 9 N-term unspecific\n\
allowed_missed_cleavage = 2            # maximum value is 5; for enzyme search\n\
\n\
#\n\
# Up to 9 variable modifications are supported\n\
# format:  <mass> <residues> <0=variable/else binary> <max_mods_per_peptide> <term_distance> <n/c-term> <required>\n\
#     e.g. 79.966331 STY 0 3 -1 0 0\n\
#\n\
variable_mod01 = 15.9949 M 0 3 -1 0 0\n\
variable_mod02 = 0.0 X 0 3 -1 0 0\n\
variable_mod03 = 0.0 X 0 3 -1 0 0\n\
variable_mod04 = 0.0 X 0 3 -1 0 0\n\
variable_mod05 = 0.0 X 0 3 -1 0 0\n\
variable_mod06 = 0.0 X 0 3 -1 0 0\n\
variable_mod07 = 0.0 X 0 3 -1 0 0\n\
variable_mod08 = 0.0 X 0 3 -1 0 0\n\
variable_mod09 = 0.0 X 0 3 -1 0 0\n\
max_variable_mods_in_peptide = 5\n\
require_variable_mod = 0\n\
\n\
#\n\
# fragment ions\n\
#\n\
# ion trap ms/ms:  1.0005 tolerance, 0.4 offset (mono masses), theoretical_fragment_ions = 1\n\
# high res ms/ms:    0.02 tolerance, 0.0 offset (mono masses), theoretical_fragment_ions = 0\n\
#\n\
fragment_bin_tol = 1.0005              # binning to use on fragment ions\n\
fragment_bin_offset = 0.4              # offset position to start the binning (0.0 to 1.0)\n\
theoretical_fragment_ions = 1          # 0=use flanking peaks, 1=M peak only\n\
use_A_ions = 0\n\
use_B_ions = 1\n\
use_C_ions = 0\n\
use_X_ions = 0\n\
use_Y_ions = 1\n\
use_Z_ions = 0\n\
use_NL_ions = 0                        # 0=no, 1=yes to consider NH3/H2O neutral loss peaks\n\
\n\
#\n\
# output\n\
#\n\
output_sqtstream = 0                   # 0=no, 1=yes  write sqt to standard output\n\
output_sqtfile = 0                     # 0=no, 1=yes  write sqt file\n\
output_txtfile = 0                     # 0=no, 1=yes  write tab-delimited txt file\n\
output_pepxmlfile = 1                  # 0=no, 1=yes  write pep.xml file\n\
output_percolatorfile = 0              # 0=no, 1=yes  write Percolator tab-delimited input file\n\
output_outfiles = 0                    # 0=no, 1=yes  write .out files\n\
print_expect_score = 1                 # 0=no, 1=yes to replace Sp with expect in out & sqt\n\
num_output_lines = 5                   # num peptide results to show\n\
show_fragment_ions = 0                 # 0=no, 1=yes for out files only\n\
\n\
sample_enzyme_number = 1               # Sample enzyme which is possibly different than the one applied to the search.\n\
                                       # Used to calculate NTT & NMC in pepXML output (default=1 for trypsin).\n\
\n\
#\n\
# mzXML parameters\n\
#\n\
scan_range = 0 0                       # start and scan scan range to search; 0 as 1st entry ignores parameter\n\
precursor_charge = 0 0                 # precursor charge range to analyze; does not override any existing charge; 0 as 1st entry ignores parameter\n\
override_charge = 0                    # 0=no, 1=override precursor charge states, 2=ignore precursor charges outside precursor_charge range, 3=see online\n\
ms_level = 2                           # MS level to analyze, valid are levels 2 (default) or 3\n\
activation_method = ALL                # activation method; used if activation method set; allowed ALL, CID, ECD, ETD, PQD, HCD, IRMPD\n\
\n\
#\n\
# misc parameters\n\
#\n\
digest_mass_range = 600.0 5000.0       # MH+ peptide mass range to analyze\n\
num_results = 100                      # number of search hits to store internally\n\
skip_researching = 1                   # for '.out' file output only, 0=search everything again (default), 1=don't search if .out exists\n\
max_fragment_charge = 3                # set maximum fragment charge state to analyze (allowed max %d)\n\
max_precursor_charge = 6               # set maximum precursor charge state to analyze (allowed max %d)\n",
      MAX_FRAGMENT_CHARGE,
      MAX_PRECURSOR_CHARGE);

fprintf(fp,
"nucleotide_reading_frame = 0           # 0=proteinDB, 1-6, 7=forward three, 8=reverse three, 9=all six\n\
clip_nterm_methionine = 0              # 0=leave sequences as-is; 1=also consider sequence w/o N-term methionine\n\
spectrum_batch_size = 0                # max. # of spectra to search at a time; 0 to search the entire scan range in one loop\n\
decoy_prefix = DECOY_                  # decoy entries are denoted by this string which is pre-pended to each protein accession\n\
output_suffix =                        # add a suffix to output base names i.e. suffix \"-C\" generates base-C.pep.xml from base.mzXML input\n\
mass_offsets =                         # one or more mass offsets to search (values substracted from deconvoluted precursor mass)\n\
\n\
#\n\
# spectral processing\n\
#\n\
minimum_peaks = 10                     # required minimum number of peaks in spectrum to search (default 10)\n");

fprintf(fp,
"minimum_intensity = 0                  # minimum intensity value to read in\n\
remove_precursor_peak = 0              # 0=no, 1=yes, 2=all charge reduced precursor peaks (for ETD)\n\
remove_precursor_tolerance = 1.5       # +- Da tolerance for precursor removal\n\
clear_mz_range = 0.0 0.0               # for iTRAQ/TMT type data; will clear out all peaks in the specified m/z range\n\
\n\
#\n\
# additional modifications\n\
#\n\
\n\
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
add_O_ornithine = 0.0000               # added to O - avg. 132.1610, mono  132.08988\n\
add_H_histidine = 0.0000               # added to H - avg. 137.1393, mono. 137.05891\n\
add_F_phenylalanine = 0.0000           # added to F - avg. 147.1739, mono. 147.06841\n\
add_U_selenocysteine = 0.0000          # added to U - avg. 150.3079, mono. 150.95363\n\
add_R_arginine = 0.0000                # added to R - avg. 156.1857, mono. 156.10111\n\
add_Y_tyrosine = 0.0000                # added to Y - avg. 163.0633, mono. 163.06333\n\
add_W_tryptophan = 0.0000              # added to W - avg. 186.0793, mono. 186.07931\n\
add_B_user_amino_acid = 0.0000         # added to B - avg.   0.0000, mono.   0.00000\n\
add_J_user_amino_acid = 0.0000         # added to J - avg.   0.0000, mono.   0.00000\n\
add_X_user_amino_acid = 0.0000         # added to X - avg.   0.0000, mono.   0.00000\n\
add_Z_user_amino_acid = 0.0000         # added to Z - avg.   0.0000, mono.   0.00000\n\
\n\
#\n\
# COMET_ENZYME_INFO _must_ be at the end of this parameters file\n\
#\n\
[COMET_ENZYME_INFO]\n\
0.  No_enzyme              0      -           -\n\
1.  Trypsin                1      KR          P\n\
2.  Trypsin/P              1      KR          -\n\
3.  Lys_C                  1      K           P\n\
4.  Lys_N                  0      K           -\n\
5.  Arg_C                  1      R           P\n\
6.  Asp_N                  0      D           -\n\
7.  CNBr                   1      M           -\n\
8.  Glu_C                  1      DE          P\n\
9.  PepsinA                1      FL          P\n\
10. Chymotrypsin           1      FWYL        P\n\
\n");

   logout("\n Created:  comet.params.new\n\n");
   fclose(fp);

} // PrintParams


bool ValidateInputMsMsFile(char *pszInputFileName)
{
   FILE *fp;
   if ((fp = fopen(pszInputFileName, "r")) == NULL)
   {
      return false;
   }
   fclose(fp);
   return true;
}
