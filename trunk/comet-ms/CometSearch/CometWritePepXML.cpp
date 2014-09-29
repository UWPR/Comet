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
#include "CometDataInternal.h"
#include "CometMassSpecUtils.h"
#include "CometWritePepXML.h"
#include "CometSearchManager.h"
#include "CometStatus.h"

#include "limits.h"
#include "stdlib.h"


CometWritePepXML::CometWritePepXML()
{
}


CometWritePepXML::~CometWritePepXML()
{
}


void CometWritePepXML::WritePepXML(FILE *fpout,
                                   FILE *fpoutd)
{
   int i;

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      PrintResults(i, 0, fpout);
   }

   // Print out the separate decoy hits.
   if (g_staticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); i++)
      {
         PrintResults(i, 1, fpoutd);
      }
   }

   fflush(fpout);
}

bool CometWritePepXML::WritePepXMLHeader(FILE *fpout,
                                         CometSearchManager &searchMgr)
{
   time_t tTime;
   char szDate[48];
   char szManufacturer[SIZE_FILE];
   char szModel[SIZE_FILE];

   time(&tTime);
   strftime(szDate, 46, "%Y-%m-%dT%H:%M:%S", localtime(&tTime));

   // Get msModel + msManufacturer from mzXML. Easy way to get from mzML too?
   ReadInstrument(szManufacturer, szModel);

   // The msms_run_summary base_name must be the base name to mzXML input.
   // This might not be the case with -N command line option.
   // So get base name from g_staticParams.inputFile.szFileName here to be sure
   char *pStr;
   char szRunSummaryBaseName[SIZE_FILE];         // base name of szInputFile
   char szRunSummaryResolvedPath[SIZE_FILE];     // resolved path of szInputFile
   int  iLen = strlen(g_staticParams.inputFile.szFileName);
   strcpy(szRunSummaryBaseName, g_staticParams.inputFile.szFileName);
   if ( (pStr = strrchr(szRunSummaryBaseName, '.')))
      *pStr = '\0';

   if (!STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 9, ".mzXML.gz")
         || !STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 8, ".mzML.gz"))
   {
      if ( (pStr = strrchr(szRunSummaryBaseName, '.')))
         *pStr = '\0';
   }

#ifdef _WIN32
   char resolvedPathBaseName[SIZE_FILE];
   _fullpath(resolvedPathBaseName, g_staticParams.inputFile.szBaseName, SIZE_FILE);
   _fullpath(szRunSummaryResolvedPath, szRunSummaryBaseName, SIZE_FILE);
#else
   char resolvedPathBaseName[PATH_MAX];
   realpath(g_staticParams.inputFile.szBaseName, resolvedPathBaseName);
   realpath(szRunSummaryBaseName, szRunSummaryResolvedPath);
#endif

   // Write out pepXML header.
   fprintf(fpout, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
   
   fprintf(fpout, " <msms_pipeline_analysis date=\"%s\" ", szDate);
   fprintf(fpout, "xmlns=\"http://regis-web.systemsbiology.net/pepXML\" ");
   fprintf(fpout, "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
   fprintf(fpout, "xsi:schemaLocation=\"http://sashimi.sourceforge.net/schema_revision/pepXML/pepXML_v117.xsd\" ");
   fprintf(fpout, "summary_xml=\"%s.pep.xml\">\n", resolvedPathBaseName);

   fprintf(fpout, " <msms_run_summary base_name=\"%s\" ", szRunSummaryResolvedPath);
   fprintf(fpout, "msManufacturer=\"%s\" ", szManufacturer);
   fprintf(fpout, "msModel=\"%s\" ", szModel);

   // Grab file extension from file name
   if ( (pStr = strrchr(g_staticParams.inputFile.szFileName, '.')) == NULL)
   {
      char szErrorMsg[256];
      sprintf(szErrorMsg,  " Error - in WriteXMLHeader missing last period in file name: %s", g_staticParams.inputFile.szFileName);
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr("%s\n", szErrorMsg);
      return false;
   }
   //FIX:  if going to support .mzXML.gz, .mzML.gz files, need to back a second period

   fprintf(fpout, "raw_data_type=\"raw\" ");
   fprintf(fpout, "raw_data=\"%s\">\n", pStr);

   if (!strncmp(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, "-", 1)
         && !strncmp(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, "-", 1))
   {
      fprintf(fpout, " <sample_enzyme name=\"nonspecific\">\n");
   }
   else
   {
      fprintf(fpout, " <sample_enzyme name=\"%s\">\n", g_staticParams.enzymeInformation.szSampleEnzymeName);
   }
   fprintf(fpout, "  <specificity cut=\"%s\" no_cut=\"%s\" sense=\"%c\"/>\n",
         g_staticParams.enzymeInformation.szSampleEnzymeBreakAA,
         g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA,
         g_staticParams.enzymeInformation.iSampleEnzymeOffSet?'C':'N');
   fprintf(fpout, " </sample_enzyme>\n");

   fprintf(fpout, " <search_summary base_name=\"%s\"", resolvedPathBaseName);
   fprintf(fpout, " search_engine=\"Comet\" search_engine_version=\"%s\"", comet_version);
   fprintf(fpout, " precursor_mass_type=\"%s\"", g_staticParams.massUtility.bMonoMassesParent?"monoisotopic":"average");
   fprintf(fpout, " fragment_mass_type=\"%s\"", g_staticParams.massUtility.bMonoMassesFragment?"monoisotopic":"average");
   fprintf(fpout, " search_id=\"1\">\n");

   fprintf(fpout, "  <search_database local_path=\"%s\"", g_staticParams.databaseInfo.szDatabase);
   fprintf(fpout, " type=\"%s\"/>\n", g_staticParams.options.iWhichReadingFrame==0?"AA":"NA");

   fprintf(fpout, "  <enzymatic_search_constraint enzyme=\"%s\" max_num_internal_cleavages=\"%d\" min_number_termini=\"%d\"/>\n",
         (g_staticParams.options.bNoEnzymeSelected?"nonspecific":g_staticParams.enzymeInformation.szSearchEnzymeName),
         g_staticParams.enzymeInformation.iAllowedMissedCleavage,
         (g_staticParams.options.iEnzymeTermini==ENZYME_DOUBLE_TERMINI)?2:
         ((g_staticParams.options.iEnzymeTermini == ENZYME_SINGLE_TERMINI)
          || (g_staticParams.options.iEnzymeTermini == ENZYME_N_TERMINI)
          || (g_staticParams.options.iEnzymeTermini == ENZYME_C_TERMINI))?1:0);

   // Write out properly encoded mods
   WriteVariableMod(fpout, searchMgr, "variable_mod01");
   WriteVariableMod(fpout, searchMgr, "variable_mod02");
   WriteVariableMod(fpout, searchMgr, "variable_mod03");
   WriteVariableMod(fpout, searchMgr, "variable_mod04");
   WriteVariableMod(fpout, searchMgr, "variable_mod05");
   WriteVariableMod(fpout, searchMgr, "variable_mod06");
   WriteVariableMod(fpout, searchMgr, "variable_mod07");
   WriteVariableMod(fpout, searchMgr, "variable_mod08");
   WriteVariableMod(fpout, searchMgr, "variable_mod09");

   double dMass = 0.0;
   if (searchMgr.GetParamValue("add_Cterm_peptide", dMass))
   {
      if (!isEqual(dMass, 0.0))
      {
         fprintf(fpout, "  <terminal_modification terminus=\"C\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"N\" protein_terminus=\"N\"/>\n",
               dMass, g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS);
      }
   }

   dMass = 0.0;
   if (searchMgr.GetParamValue("add_Nterm_peptide", dMass))
   {
      if (!isEqual(dMass, 0.0))
      {
         fprintf(fpout, "  <terminal_modification terminus=\"N\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"N\" protein_terminus=\"N\"/>\n",
               dMass, g_staticParams.precalcMasses.dNtermProton - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment[(int)'h']);
      }
   }

   dMass = 0.0;
   if (searchMgr.GetParamValue("add_Cterm_protein", dMass))
   {
      if (!isEqual(dMass, 0.0))
      {
         fprintf(fpout, "  <terminal_modification terminus=\"C\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"N\" protein_terminus=\"Y\"/>\n",
               dMass, dMass + g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS);
      }
   }

   dMass = 0.0;
   if (searchMgr.GetParamValue("add_Nterm_protein", dMass))
   {
      if (!isEqual(dMass, 0.0))
      {
         fprintf(fpout, "  <terminal_modification terminus=\"N\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"N\" protein_terminus=\"Y\"/>\n",
               dMass, dMass + g_staticParams.precalcMasses.dNtermProton - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment[(int)'h']); //FIX??
      }
   }

   WriteAddAminoAcid(fpout, searchMgr, "add_G_glycine");
   WriteAddAminoAcid(fpout, searchMgr, "add_A_alanine");
   WriteAddAminoAcid(fpout, searchMgr, "add_S_serine");
   WriteAddAminoAcid(fpout, searchMgr, "add_P_proline");
   WriteAddAminoAcid(fpout, searchMgr, "add_V_valine");
   WriteAddAminoAcid(fpout, searchMgr, "add_T_threonine");
   WriteAddAminoAcid(fpout, searchMgr, "add_C_cysteine");
   WriteAddAminoAcid(fpout, searchMgr, "add_L_leucine");
   WriteAddAminoAcid(fpout, searchMgr, "add_I_isoleucine");
   WriteAddAminoAcid(fpout, searchMgr, "add_N_asparagine");
   WriteAddAminoAcid(fpout, searchMgr, "add_O_ornithine");
   WriteAddAminoAcid(fpout, searchMgr, "add_D_aspartic_acid");
   WriteAddAminoAcid(fpout, searchMgr, "add_Q_glutamine");
   WriteAddAminoAcid(fpout, searchMgr, "add_K_lysine");
   WriteAddAminoAcid(fpout, searchMgr, "add_E_glutamic_acid");
   WriteAddAminoAcid(fpout, searchMgr, "add_M_methionine");
   WriteAddAminoAcid(fpout, searchMgr, "add_H_histidine");
   WriteAddAminoAcid(fpout, searchMgr, "add_F_phenylalanine");
   WriteAddAminoAcid(fpout, searchMgr, "add_R_arginine");
   WriteAddAminoAcid(fpout, searchMgr, "add_Y_tyrosine");
   WriteAddAminoAcid(fpout, searchMgr, "add_W_tryptophan");
   WriteAddAminoAcid(fpout, searchMgr, "add_B_user_amino_acid");
   WriteAddAminoAcid(fpout, searchMgr, "add_J_user_amino_acid");
   WriteAddAminoAcid(fpout, searchMgr, "add_U_user_amino_acid");
   WriteAddAminoAcid(fpout, searchMgr, "add_X_user_amino_acid");
   WriteAddAminoAcid(fpout, searchMgr, "add_Z_user_amino_acid");

   std::map<std::string, CometParam*> mapParams = searchMgr.GetParamsMap();
   for (std::map<std::string, CometParam*>::iterator it=mapParams.begin(); it!=mapParams.end(); ++it)
   {
      if (it->first != "[COMET_ENZYME_INFO]")
      {
         fprintf(fpout, "  <parameter name=\"%s\" value=\"%s\"/>\n", it->first.c_str(), it->second->GetStringValue().c_str());
      }
   }

   fprintf(fpout, " </search_summary>\n");
   fflush(fpout);

   return true;
}


void CometWritePepXML::WriteVariableMod(FILE *fpout,
                                        CometSearchManager &searchMgr,
                                        string varModName)
{
   VarMods varModsParam;
   if (searchMgr.GetParamValue(varModName, varModsParam))
   {
      char cSymbol = '-';
      if (varModName[13]=='1')
         cSymbol = g_staticParams.variableModParameters.cModCode[0];
      else if (varModName[13]=='2')
         cSymbol = g_staticParams.variableModParameters.cModCode[1];
      else if (varModName[13]=='3')
         cSymbol = g_staticParams.variableModParameters.cModCode[2];
      else if (varModName[13]=='4')
         cSymbol = g_staticParams.variableModParameters.cModCode[3];
      else if (varModName[13]=='5')
         cSymbol = g_staticParams.variableModParameters.cModCode[4];
      else if (varModName[13]=='6')
         cSymbol = g_staticParams.variableModParameters.cModCode[5];
      else if (varModName[13]=='7')
         cSymbol = g_staticParams.variableModParameters.cModCode[6];
      else if (varModName[13]=='8')
         cSymbol = g_staticParams.variableModParameters.cModCode[7];
      else if (varModName[13]=='9')
         cSymbol = g_staticParams.variableModParameters.cModCode[8];

      if (cSymbol != '-' && !isEqual(varModsParam.dVarModMass, 0.0))
      {
         int iLen = (int)strlen(varModsParam.szVarModChar);
         for (int i=0; i<iLen; i++)
         {
            if (varModsParam.szVarModChar[i]=='n')
            {
               fprintf(fpout, "  <terminal_modification terminus=\"N\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\"",
                     varModsParam.dVarModMass,
                     varModsParam.dVarModMass +  g_staticParams.precalcMasses.dNtermProton - PROTON_MASS);

               if (varModsParam.iVarModTermDistance == 0)
                  fprintf(fpout, " protein_terminus=\"Y\" />\n");
               else
                  fprintf(fpout, " protein_terminus=\"N\" />\n");

               fprintf(fpout, " symbol=\"%c\"/>\n", cSymbol);
            }
            else if (varModsParam.szVarModChar[i]=='c')
            {
               fprintf(fpout, "  <terminal_modification terminus=\"C\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\"",
                     varModsParam.dVarModMass,
                     varModsParam.dVarModMass +  g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS);

               if (varModsParam.iVarModTermDistance == 0)
                  fprintf(fpout, " protein_terminus=\"Y\" />\n");
               else
                  fprintf(fpout, " protein_terminus=\"N\" />\n");

               fprintf(fpout, " symbol=\"%c\"/>\n", cSymbol);
            }
            else
            {
               fprintf(fpout, "  <aminoacid_modification aminoacid=\"%c\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\" %ssymbol=\"%c\"/>\n",
                     varModsParam.szVarModChar[i],
                     varModsParam.dVarModMass,
                     g_staticParams.massUtility.pdAAMassParent[(int)varModsParam.szVarModChar[i]] + varModsParam.dVarModMass,
                     (varModsParam.bBinaryMod?"binary=\"Y\" ":""),
                     cSymbol);
            }
         }
      }
   }
}


void CometWritePepXML::WriteAddAminoAcid(FILE *fpout,
                                         CometSearchManager &searchMgr,
                                         string paramName)
{
   double dMass = 0.0;
   if (searchMgr.GetParamValue(paramName, dMass))
   {
      if (!isEqual(dMass, 0.0))
      {
         fprintf(fpout, "  <aminoacid_modification aminoacid=\"%c\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"N\"/>\n",
               paramName[4], dMass, g_staticParams.massUtility.pdAAMassParent[(int)paramName[4]]);
      }
   }
}

void CometWritePepXML::WritePepXMLEndTags(FILE *fpout)
{
   fprintf(fpout, " </msms_run_summary>\n");
   fprintf(fpout, "</msms_pipeline_analysis>\n");
   fflush(fpout);
}

void CometWritePepXML::PrintResults(int iWhichQuery,
                                    bool bDecoy,
                                    FILE *fpout)
{
   int  i,
        iNumPrintLines,
        iRankXcorr,
        iMinLength;
   char *pStr;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   // look for either \ or / separator so valid for Windows or Linux
   if ((pStr = strrchr(g_staticParams.inputFile.szBaseName, '\\')) == NULL
         && (pStr = strrchr(g_staticParams.inputFile.szBaseName, '/')) == NULL)
      pStr = g_staticParams.inputFile.szBaseName;
   else
      pStr++;  // skip separation character

   // Print spectrum_query element. 
   fprintf(fpout, " <spectrum_query spectrum=\"%s.%05d.%05d.%d\"",
         pStr,
         pQuery->_spectrumInfoInternal.iScanNumber,
         pQuery->_spectrumInfoInternal.iScanNumber,
         pQuery->_spectrumInfoInternal.iChargeState);

   if (pQuery->_spectrumInfoInternal.szNativeID[0]!='\0')
      fprintf(fpout, " spectrumNativeID=\"%s\"", pQuery->_spectrumInfoInternal.szNativeID);

   fprintf(fpout, " start_scan=\"%d\"", pQuery->_spectrumInfoInternal.iScanNumber);
   fprintf(fpout, " end_scan=\"%d\"", pQuery->_spectrumInfoInternal.iScanNumber);
   fprintf(fpout, " precursor_neutral_mass=\"%0.6f\"", pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS);
   fprintf(fpout, " assumed_charge=\"%d\"", pQuery->_spectrumInfoInternal.iChargeState);
   fprintf(fpout, " index=\"%d\"", iWhichQuery+1);

   if (mzXML)
      fprintf(fpout, " retention_time_sec=\"%0.1f\">\n", pQuery->_spectrumInfoInternal.dRTime);
   else
      fprintf(fpout, ">\n");

   fprintf(fpout, "  <search_result>\n");

   if (bDecoy)
      iNumPrintLines = pQuery->iDecoyMatchPeptideCount;
   else
      iNumPrintLines = pQuery->iMatchPeptideCount;

   // Print out each sequence line.
   if (iNumPrintLines > (g_staticParams.options.iNumPeptideOutputLines))
      iNumPrintLines = (g_staticParams.options.iNumPeptideOutputLines);

   Results *pOutput;

   if (bDecoy)
      pOutput = pQuery->_pDecoys;
   else
      pOutput = pQuery->_pResults;

   iRankXcorr = 1;

   iMinLength = 999;
   for (i=0; i<iNumPrintLines; i++)
   {
      int iLen = (int)strlen(pOutput[i].szPeptide);
      if (iLen == 0)
         break;
      if (iLen < iMinLength)
         iMinLength = iLen;
   }

   for (i=0; i<iNumPrintLines; i++)
   {
      int j;
      bool bNoDeltaCnYet = true;
      bool bDeltaCnStar = false;
      double dDeltaCn;       // this is deltaCn between i and first dissimilar peptide
      double dDeltaCnStar;   // this is explicit deltaCn between i and i+1 hits or 0.0 ...
                             // I honestly don't understand the logic in the deltacnstar convention being used in TPP

      for (j=i+1; j<iNumPrintLines; j++)
      {
         // very poor way of calculating peptide similarity but it's what we have for now
         int iDiffCt = 0;

         for (int k=0; k<iMinLength; k++)
         {
            // I-L and Q-K are same for purposes here
            if (pOutput[i].szPeptide[k] != pOutput[j].szPeptide[k])
            {
               if (!((pOutput[i].szPeptide[k] == 'K' || pOutput[i].szPeptide[k] == 'Q')
                       && (pOutput[j].szPeptide[k] == 'K' || pOutput[j].szPeptide[k] == 'Q'))
                     && !((pOutput[i].szPeptide[k] == 'I' || pOutput[i].szPeptide[k] == 'L')
                        && (pOutput[j].szPeptide[k] == 'I' || pOutput[j].szPeptide[k] == 'L')))
               {
                  iDiffCt++;
               }
            }
         }

         // calculate deltaCn only if sequences are less than 0.75 similar
         if ( ((double) (iMinLength - iDiffCt)/iMinLength) < 0.75)
         {
            if (pOutput[i].fXcorr > 0.0 && pOutput[j].fXcorr >= 0.0)
               dDeltaCn = 1.0 - pOutput[j].fXcorr/pOutput[i].fXcorr;
            else if (pOutput[i].fXcorr > 0.0 && pOutput[j].fXcorr < 0.0)
               dDeltaCn = 1.0;
            else
               dDeltaCn = 0.0;

            bNoDeltaCnYet = 0;
   
            if (j - i > 1)
               bDeltaCnStar = true;
            break;
         }
      }

      if (bNoDeltaCnYet)
         dDeltaCn = 1.0;

      if (i > 0 && !isEqual(pOutput[i].fXcorr, pOutput[i-1].fXcorr))
         iRankXcorr++;

      if (pOutput[i].fXcorr > XCORR_CUTOFF)
      {
         if (bDeltaCnStar && i+1<iNumPrintLines)
         {
            if (pOutput[i].fXcorr > 0.0 && pOutput[i+1].fXcorr >= 0.0)
               dDeltaCnStar = 1.0 - pOutput[i+1].fXcorr/pOutput[i].fXcorr;
            else if (pOutput[i].fXcorr > 0.0 && pOutput[i+1].fXcorr < 0.0)
               dDeltaCnStar = 1.0;
            else
               dDeltaCnStar = 0.0;
         }
         else
            dDeltaCnStar = 0.0;

         PrintPepXMLSearchHit(iWhichQuery, i, bDecoy, pOutput, fpout, dDeltaCn, dDeltaCnStar);
      }
   } 

   fprintf(fpout, "  </search_result>\n");
   fprintf(fpout, " </spectrum_query>\n");
}


void CometWritePepXML::PrintPepXMLSearchHit(int iWhichQuery,
                                            int iWhichResult,
                                            bool bDecoy,
                                            Results *pOutput,
                                            FILE *fpout,
                                            double dDeltaCn,
                                            double dDeltaCnStar)
{
   int  i;
   int iNTT;
   int iNMC;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   fprintf(fpout, "   <search_hit hit_rank=\"%d\"", iWhichResult+1);
   fprintf(fpout, " peptide=\"%s\"", pOutput[iWhichResult].szPeptide);
   fprintf(fpout, " peptide_prev_aa=\"%c\"", pOutput[iWhichResult].szPrevNextAA[0]);
   fprintf(fpout, " peptide_next_aa=\"%c\"", pOutput[iWhichResult].szPrevNextAA[1]);
   fprintf(fpout, " protein=\"%s\"", pOutput[iWhichResult].szProtein);
   fprintf(fpout, " num_tot_proteins=\"%d\"", pOutput[iWhichResult].iDuplicateCount+1);
   fprintf(fpout, " num_matched_ions=\"%d\"", pOutput[iWhichResult].iMatchedIons);
   fprintf(fpout, " tot_num_ions=\"%d\"", pOutput[iWhichResult].iTotalIons);
   fprintf(fpout, " calc_neutral_pep_mass=\"%0.6f\"", pOutput[iWhichResult].dPepMass - PROTON_MASS);
   fprintf(fpout, " massdiff=\"%0.6f\"", pQuery->_pepMassInfo.dExpPepMass - pOutput[iWhichResult].dPepMass);

   CalcNTTNMC(pOutput, iWhichResult, &iNTT, &iNMC);

   fprintf(fpout, " num_tol_term=\"%d\"", iNTT);
   fprintf(fpout, " num_missed_cleavages=\"%d\"", iNMC); 
   fprintf(fpout, " num_matched_peptides=\"%lu\"",
         bDecoy?(pQuery->_uliNumMatchedDecoyPeptides):(pQuery->_uliNumMatchedPeptides));
   fprintf(fpout, ">\n");

   // check if peptide is modified
   bool bModified = 0;

   if (!isEqual(g_staticParams.staticModifications.dAddNterminusPeptide, 0.0)
         || !isEqual(g_staticParams.staticModifications.dAddCterminusPeptide, 0.0))
      bModified = 1;

   if (pOutput[iWhichResult].szPrevNextAA[0]=='-' && !isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0))
      bModified = 1;
   if (pOutput[iWhichResult].szPrevNextAA[1]=='-' && !isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0))
      bModified = 1;

   if (!bModified)
   {
      for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
      {
         if (!isEqual(g_staticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]], 0.0)
               || pOutput[iWhichResult].pcVarModSites[i] > 0)
         {
            bModified = 1;
            break;
         }
      }

      // check n- and c-terminal variable mods
      i=pOutput[iWhichResult].iLenPeptide;
      if (pOutput[iWhichResult].pcVarModSites[i] != 0  || pOutput[iWhichResult].pcVarModSites[i+1] != 0)
         bModified = 1;
   }

   if (bModified)
   {
      // construct modified peptide string
      char szModPep[512];
      
      szModPep[0]='\0';

      if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] == 1)
      {
         sprintf(szModPep+strlen(szModPep), "n[%0.0f]",
               g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1].dVarModMass
               + g_staticParams.precalcMasses.dNtermProton);
      }

      for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
      {
         sprintf(szModPep+strlen(szModPep), "%c", pOutput[iWhichResult].szPeptide[i]);

         if (pOutput[iWhichResult].pcVarModSites[i] > 0)
         {
            sprintf(szModPep+strlen(szModPep), "[%0.0f]",
                  g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass
                  + g_staticParams.massUtility.pdAAMassFragment[(int)pOutput[iWhichResult].szPeptide[i]]);
         }
      }

      if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] == 1)
      {
         sprintf(szModPep+strlen(szModPep), "c[%0.0f]",
               g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].dVarModMass
               + g_staticParams.precalcMasses.dCtermOH2Proton);
      }

      fprintf(fpout, "    <modification_info modified_peptide=\"%s\"", szModPep);

      if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0
            || !isEqual(g_staticParams.staticModifications.dAddNterminusPeptide, 0.0)
            || (pOutput[iWhichResult].szPrevNextAA[0]=='-'
               && !isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0)) )
      {
         // static peptide n-term mod already accounted for here
         double dMass = g_staticParams.precalcMasses.dNtermProton - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

         if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
            dMass += g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide]-1].dVarModMass;

         if (pOutput[iWhichResult].szPrevNextAA[0]=='-' && !isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0))
            dMass += g_staticParams.staticModifications.dAddNterminusProtein;

         fprintf(fpout, " mod_nterm_mass=\"%0.6f\"", dMass);
      }

      if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0
            || !isEqual(g_staticParams.staticModifications.dAddCterminusPeptide, 0.0)
            || (pOutput[iWhichResult].szPrevNextAA[1]=='-'
               && !isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0)) )
      {
         // static peptide c-term mod already accounted for here
         double dMass = g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS;

         if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0)
            dMass += g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].dVarModMass;

         if (pOutput[iWhichResult].szPrevNextAA[1]=='-' && !isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0))
            dMass += g_staticParams.staticModifications.dAddCterminusProtein;

         fprintf(fpout, " mod_cterm_mass=\"%0.6f\"", dMass);
      }
      fprintf(fpout, ">\n");

      for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
      {
         if (!isEqual(g_staticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]], 0.0)
               || pOutput[iWhichResult].pcVarModSites[i] > 0)
         {
            double dMass = g_staticParams.massUtility.pdAAMassFragment[(int)pOutput[iWhichResult].szPeptide[i]];

            if (pOutput[iWhichResult].pcVarModSites[i] > 0)
               dMass += g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass;

            fprintf(fpout, "     <mod_aminoacid_mass position=\"%d\" mass=\"%0.6f\"/>\n", i+1, dMass);
         }
      }

      fprintf(fpout, "    </modification_info>\n");
   }

   fprintf(fpout, "    <search_score name=\"xcorr\" value=\"%0.3f\"/>\n", pOutput[iWhichResult].fXcorr);

   fprintf(fpout, "    <search_score name=\"deltacn\" value=\"%0.3f\"/>\n", dDeltaCn);
   fprintf(fpout, "    <search_score name=\"deltacnstar\" value=\"%0.3f\"/>\n", dDeltaCnStar);

   fprintf(fpout, "    <search_score name=\"spscore\" value=\"%0.1f\"/>\n", pOutput[iWhichResult].fScoreSp);
   fprintf(fpout, "    <search_score name=\"sprank\" value=\"%d\"/>\n", pOutput[iWhichResult].iRankSp);
   fprintf(fpout, "    <search_score name=\"expect\" value=\"%0.2E\"/>\n", pOutput[iWhichResult].dExpect);
   fprintf(fpout, "   </search_hit>\n");
}


void CometWritePepXML::ReadInstrument(char *szManufacturer,
                                      char *szModel)
{
   strcpy(szManufacturer, "unknown");
   strcpy(szModel, "unknown");

   if (g_staticParams.inputFile.iInputType == InputType_MZXML)
   {
      FILE *fp;

      if ((fp = fopen(g_staticParams.inputFile.szFileName, "r")) != NULL)
      {
         char szMsInstrumentElement[SIZE_BUF];
         char szBuf[SIZE_BUF];

         szMsInstrumentElement[0]='\0';
         while (fgets(szBuf, SIZE_BUF, fp))
         {
            if (strstr(szBuf, "<scan") || strstr(szBuf, "mslevel"))
               break;

            // Grab entire msInstrument element.
            if (strstr(szBuf, "<msInstrument"))
            {
               strcat(szMsInstrumentElement, szBuf);

               while (fgets(szBuf, SIZE_BUF, fp))
               {
                  if (strlen(szMsInstrumentElement)+strlen(szBuf)<8192)
                     strcat(szMsInstrumentElement, szBuf);
                  if (strstr(szBuf, "</msInstrument>"))
                  {
                     GetVal(szMsInstrumentElement, "\"msModel\" value", szModel);
                     GetVal(szMsInstrumentElement, "\"msManufacturer\" value", szManufacturer);
                     break;
                  }
               }
            }
         }

         fclose(fp);
      }
   }
}


void CometWritePepXML::GetVal(char *szElement,
                              char *szAttribute,
                              char *szAttributeVal)
{
   char *pStr;

   if ((pStr=strstr(szElement, szAttribute)))
   {
      strncpy(szAttributeVal, pStr+strlen(szAttribute)+2, SIZE_FILE);  // +2 to skip ="
      szAttributeVal[SIZE_FILE-1] = '\0';

      if ((pStr=strchr(szAttributeVal, '"')))
      {
         *pStr='\0';
         return;
      }
      else
      {
         strcpy(szAttributeVal, "unknown");  // Error - expecting an end quote in szAttributeVal.
         return;
      }
   }
   else
   {
      strcpy(szAttributeVal, "unknown"); // Attribute not found.
      return;
   }
}


void CometWritePepXML::CalcNTTNMC(Results *pOutput,
                                  int iWhichResult,
                                  int *iNTT,
                                  int *iNMC)
{
   int i;
   *iNTT=0;
   *iNMC=0;

   // Calculate number of tolerable termini (NTT) based on sample_enzyme
   if (pOutput[iWhichResult].szPrevNextAA[0]=='-')
   {
      *iNTT += 1;
   }
   else if (g_staticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPrevNextAA[0])
            && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[0]))
      {
         *iNTT += 1;
      }
   }
   else
   {
      if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[0])
            && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPrevNextAA[0]))
      {
         *iNTT += 1;
      }
   }

   if (pOutput[iWhichResult].szPrevNextAA[1]=='-')
   {
      *iNTT += 1;
   }
   else if (g_staticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[pOutput[iWhichResult].iLenPeptide -1])
            && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPrevNextAA[1]))
      {
         *iNTT += 1;
      }
   }
   else
   {
      if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPrevNextAA[1])
            && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[pOutput[iWhichResult].iLenPeptide -1]))
      {
         *iNTT += 1;
      }
   }

   // Calculate number of missed cleavage (NMC) sites based on sample_enzyme
   if (g_staticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      for (i=0; i<pOutput[iWhichResult].iLenPeptide-1; i++)
      {
         if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[i])
               && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[i+1]))
         {
            *iNMC += 1;
         }
      }
   }
   else
   {
      for (i=1; i<pOutput[iWhichResult].iLenPeptide; i++)
      {
         if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[i])
               && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[i-1]))
         {
            *iNMC += 1;
         }
      }
   }

}
