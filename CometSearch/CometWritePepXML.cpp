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
                                   FILE *fpoutd,
                                   FILE *fpdb,
                                   int iNumSpectraSearched)
{
   int i;

   // Print out the separate decoy hits.
   if (g_staticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); ++i)
         PrintResults(i, 1, fpout, fpdb, iNumSpectraSearched);
      for (i=0; i<(int)g_pvQuery.size(); ++i)
         PrintResults(i, 2, fpoutd, fpdb, iNumSpectraSearched);
   }
   else
   {
      for (i=0; i<(int)g_pvQuery.size(); ++i)
         PrintResults(i, 0, fpout, fpdb, iNumSpectraSearched);
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
   char szResolvedInputBaseName[PATH_MAX];       // base name of szInputFile
   char szResolvedOutputBaseName[PATH_MAX];      // resolved path of szInputFile

   if (g_staticParams.options.bResolveFullPaths)
   {
      // realpath is #defined to _fullpath in WIN32
      if (!realpath(g_staticParams.inputFile.szFileName, szResolvedInputBaseName))
         strcpy(szResolvedInputBaseName, g_staticParams.inputFile.szFileName);
   }
   else
      strcpy(szResolvedInputBaseName, g_staticParams.inputFile.szFileName);

   // now remove extension from szRunSummaryResolvedPath to leave just the base name
   if ((pStr = strrchr(szResolvedInputBaseName, '.')))
      *pStr = '\0';

   if (g_staticParams.options.bResolveFullPaths)
   {
      // now get base name of output file; possibly different from above as could
      // be set using -N command line option
      std::string dir = string(g_staticParams.inputFile.szBaseName);
      std::size_t found = dir.find_last_of("/\\");
      std::string file = "";
      if (found != string::npos)
      {
         file = dir.substr(found);
         dir = dir.substr(0,found);
      }
      else
      {
         dir = ".";
#ifdef _WIN32
         file = "\\";
         file +=  g_staticParams.inputFile.szBaseName;
#else
         file = "/";
         file +=  g_staticParams.inputFile.szBaseName;
#endif
      }

      if (!realpath(g_staticParams.inputFile.szBaseName, szResolvedOutputBaseName))
      {
         if (!realpath(dir.c_str(), szResolvedOutputBaseName))
            strcpy(szResolvedOutputBaseName, g_staticParams.inputFile.szBaseName);
         else
            strcat(szResolvedOutputBaseName, file.c_str());
      }
   }
   else
      strcpy(szResolvedOutputBaseName, g_staticParams.inputFile.szBaseName);


   // Write out pepXML header.
   fprintf(fpout, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

   fprintf(fpout, " <msms_pipeline_analysis date=\"%s\" ", szDate);
   fprintf(fpout, "xmlns=\"http://regis-web.systemsbiology.net/pepXML\" ");
   fprintf(fpout, "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
   fprintf(fpout, "xsi:schemaLocation=\"http://sashimi.sourceforge.net/schema_revision/pepXML/pepXML_v120.xsd\" ");
   fprintf(fpout, "summary_xml=\"%s.pep.xml\">\n", szResolvedOutputBaseName);

   fprintf(fpout, " <msms_run_summary base_name=\"%s\" ", szResolvedInputBaseName);
   fprintf(fpout, "msManufacturer=\"%s\" ", szManufacturer);
   fprintf(fpout, "msModel=\"%s\" ", szModel);

   // Grab file extension from file name
   if ( (pStr = strrchr(g_staticParams.inputFile.szFileName, '.')) == NULL)
   {
      char szErrorMsg[SIZE_ERROR];
      sprintf(szErrorMsg,  " Error - in WriteXMLHeader missing last period in file name: %s\n",
            g_staticParams.inputFile.szFileName);
      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
      logerr(szErrorMsg);
      return false;
   }
   // MH: Check if the extension is gz, and if so, extend it back to the full file type.
   if( !strcmp(pStr,".gz"))
   {
      pStr--;
      while(*pStr!='.') pStr--;
   }

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

   fprintf(fpout, " <search_summary base_name=\"%s\"", szResolvedOutputBaseName);
   fprintf(fpout, " search_engine=\"Comet\" search_engine_version=\"%s%s\"", (g_staticParams.options.bMango?"Mango ":""), g_sCometVersion.c_str());
   fprintf(fpout, " precursor_mass_type=\"%s\"", g_staticParams.massUtility.bMonoMassesParent?"monoisotopic":"average");
   fprintf(fpout, " fragment_mass_type=\"%s\"", g_staticParams.massUtility.bMonoMassesFragment?"monoisotopic":"average");
   fprintf(fpout, " search_id=\"1\">\n");

   fprintf(fpout, "  <search_database local_path=\"%s\"", g_staticParams.databaseInfo.szDatabase);
   fprintf(fpout, " type=\"%s\"/>\n", g_staticParams.options.iWhichReadingFrame==0?"AA":"NA");

   fprintf(fpout, "  <enzymatic_search_constraint enzyme=\"%s\" max_num_internal_cleavages=\"%d\" min_number_termini=\"%d\"/>\n",
         (g_staticParams.enzymeInformation.bNoEnzymeSelected?"nonspecific":g_staticParams.enzymeInformation.szSearchEnzymeName),
         g_staticParams.enzymeInformation.iAllowedMissedCleavage,
         (g_staticParams.options.iEnzymeTermini==ENZYME_DOUBLE_TERMINI)?2:
         ((g_staticParams.options.iEnzymeTermini == ENZYME_SINGLE_TERMINI)
          || (g_staticParams.options.iEnzymeTermini == ENZYME_N_TERMINI)
          || (g_staticParams.options.iEnzymeTermini == ENZYME_C_TERMINI))?1:0);

   // Write modifications header; first amino acid mods
   for (int iWhichVariableMod = 0; iWhichVariableMod < VMODS; ++iWhichVariableMod)
   {
      WriteVariableMod(fpout, iWhichVariableMod, 0);  // third parameter writes amino acid mods
   }

   WriteStaticMod(fpout, searchMgr, "add_G_glycine");
   WriteStaticMod(fpout, searchMgr, "add_A_alanine");
   WriteStaticMod(fpout, searchMgr, "add_S_serine");
   WriteStaticMod(fpout, searchMgr, "add_P_proline");
   WriteStaticMod(fpout, searchMgr, "add_V_valine");
   WriteStaticMod(fpout, searchMgr, "add_T_threonine");
   WriteStaticMod(fpout, searchMgr, "add_C_cysteine");
   WriteStaticMod(fpout, searchMgr, "add_L_leucine");
   WriteStaticMod(fpout, searchMgr, "add_I_isoleucine");
   WriteStaticMod(fpout, searchMgr, "add_N_asparagine");
   WriteStaticMod(fpout, searchMgr, "add_O_ornithine");
   WriteStaticMod(fpout, searchMgr, "add_D_aspartic_acid");
   WriteStaticMod(fpout, searchMgr, "add_Q_glutamine");
   WriteStaticMod(fpout, searchMgr, "add_K_lysine");
   WriteStaticMod(fpout, searchMgr, "add_E_glutamic_acid");
   WriteStaticMod(fpout, searchMgr, "add_M_methionine");
   WriteStaticMod(fpout, searchMgr, "add_H_histidine");
   WriteStaticMod(fpout, searchMgr, "add_F_phenylalanine");
   WriteStaticMod(fpout, searchMgr, "add_R_arginine");
   WriteStaticMod(fpout, searchMgr, "add_Y_tyrosine");
   WriteStaticMod(fpout, searchMgr, "add_W_tryptophan");
   WriteStaticMod(fpout, searchMgr, "add_B_user_amino_acid");
   WriteStaticMod(fpout, searchMgr, "add_J_user_amino_acid");
   WriteStaticMod(fpout, searchMgr, "add_U_user_amino_acid");
   WriteStaticMod(fpout, searchMgr, "add_X_user_amino_acid");
   WriteStaticMod(fpout, searchMgr, "add_Z_user_amino_acid");

   // write terminal_modification which has to come after aminoacid_modification 
   for (int iWhichVariableMod = 0; iWhichVariableMod < VMODS; ++iWhichVariableMod)
   {
      WriteVariableMod(fpout, iWhichVariableMod, 1);
   }

   double dMass = 0.0;
   if (searchMgr.GetParamValue("add_Cterm_peptide", dMass))
   {
      if (!isEqual(dMass, 0.0))
      {
         fprintf(fpout, "  <terminal_modification terminus=\"C\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"N\" protein_terminus=\"N\"/>\n",
               dMass, g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS - g_staticParams.massUtility.pdAAMassFragment[(int)'h']);

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
               dMass, dMass + g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS - g_staticParams.massUtility.pdAAMassFragment[(int)'h']);

      }
   }

   dMass = 0.0;
   if (searchMgr.GetParamValue("add_Nterm_protein", dMass))
   {
      if (!isEqual(dMass, 0.0))
      {
         fprintf(fpout, "  <terminal_modification terminus=\"N\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"N\" protein_terminus=\"Y\"/>\n",
               dMass, dMass + g_staticParams.precalcMasses.dNtermProton - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment[(int)'h']);
      }
   }

   std::map<std::string, CometParam*> mapParams = searchMgr.GetParamsMap();
   for (std::map<std::string, CometParam*>::iterator it=mapParams.begin(); it!=mapParams.end(); ++it)
   {
      if (it->first != "[COMET_ENZYME_INFO]")
      {
         fprintf(fpout, "  <parameter name=\"%s\" value=\"%s\"/>\n", it->first.c_str(), it->second->GetStringValue().c_str());
      }
   }

   // write out amino acid mass of parent used; applicable for user defined AA masses
   for (int x = 65 ; x <= 90; ++x)
   {
      fprintf(fpout, "  <parameter name=\"residuemass_%c\" value=\"%lf\"/>\n", char(x), g_staticParams.massUtility.pdAAMassParent[x]);
   }


   fprintf(fpout, " </search_summary>\n");
   fflush(fpout);

   return true;
}


void CometWritePepXML::WriteVariableMod(FILE *fpout,
                                        int iWhichVariableMod,
                                        bool bWriteTerminalMods)
{
   VarMods varModsParam = g_staticParams.variableModParameters.varModList[iWhichVariableMod];

   if (!isEqual(varModsParam.dVarModMass, 0.0))
   {
      int iLen = (int)strlen(varModsParam.szVarModChar);

      for (int i=0; i<iLen; ++i)
      {
         if (bWriteTerminalMods)
         {
            if (varModsParam.szVarModChar[i] == 'n')
            {
               if (varModsParam.iVarModTermDistance == 0 && (varModsParam.iWhichTerm == 1 || varModsParam.iWhichTerm == 3))
               {
                  // ignore if N-term mod on C-term
               }
               else
               {
                  // print this if N-term protein variable mod or a generic N-term mod there's also N-term protein static mod
                  if (varModsParam.iWhichTerm == 0 && varModsParam.iVarModTermDistance == 0)
                  {
                     // massdiff = mod mass + h
                     fprintf(fpout, "  <terminal_modification terminus=\"N\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\" protein_terminus=\"Y\"/>\n",
                        varModsParam.dVarModMass,
                        varModsParam.dVarModMass
                        + g_staticParams.staticModifications.dAddNterminusProtein
                        + g_staticParams.precalcMasses.dNtermProton
                        - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment[(int)'h']);
                  }
                  // print this if non-protein N-term variable mod
                  else
                  {
                     fprintf(fpout, "  <terminal_modification terminus=\"N\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\" protein_terminus=\"N\"/>\n",
                        varModsParam.dVarModMass,
                        varModsParam.dVarModMass
                        + g_staticParams.precalcMasses.dNtermProton
                        - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment[(int)'h']);
                  }
               }
            }
            else if (varModsParam.szVarModChar[i] == 'c')
            {
               if (varModsParam.iVarModTermDistance == 0 && (varModsParam.iWhichTerm == 0 || varModsParam.iWhichTerm == 2))
               {
                  // ignore if C-term mod on N-term
               }
               else
               {
                  // print this if C-term protein variable mod or a generic C-term mod there's also C-term protein static mod
                  if (varModsParam.iWhichTerm == 1 && varModsParam.iVarModTermDistance == 0)
                  {
                     // massdiff = mod mass + oh
                     fprintf(fpout, "  <terminal_modification terminus=\"C\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\" protein_terminus=\"Y\"/>\n",
                        varModsParam.dVarModMass,
                        varModsParam.dVarModMass
                        + g_staticParams.staticModifications.dAddCterminusProtein
                        + g_staticParams.precalcMasses.dCtermOH2Proton
                        - PROTON_MASS
                        - g_staticParams.massUtility.pdAAMassFragment[(int)'h']);
                  }
                  // print this if non-protein C-term variable mod
                  else
                  {
                     fprintf(fpout, "  <terminal_modification terminus=\"C\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\" protein_terminus=\"N\"/>\n",
                        varModsParam.dVarModMass,
                        varModsParam.dVarModMass
                        + g_staticParams.precalcMasses.dCtermOH2Proton
                        - PROTON_MASS
                        - g_staticParams.massUtility.pdAAMassFragment[(int)'h']);
                  }
               }
            }
         }
         else if (varModsParam.szVarModChar[i]!='c' && varModsParam.szVarModChar[i]!='n')
         {
            fprintf(fpout, "  <aminoacid_modification aminoacid=\"%c\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\"%s/>\n",
                  varModsParam.szVarModChar[i],
                  varModsParam.dVarModMass,
                  g_staticParams.massUtility.pdAAMassParent[(int)varModsParam.szVarModChar[i]] + varModsParam.dVarModMass,
                  (varModsParam.iBinaryMod?" binary=\"Y\"":""));
         }
      }
   }
}


void CometWritePepXML::WriteStaticMod(FILE *fpout,
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
                                    int iPrintTargetDecoy,
                                    FILE *fpout,
                                    FILE *fpdb,
                                    int iNumSpectraSearched)
{
   int  i,
        iNumPrintLines,
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
   if (g_staticParams.options.bMango)   // Mango specific
   {
      fprintf(fpout, " <spectrum_query spectrum=\"%s_%s.%05d.%05d.%d\"",
            pStr,
            pQuery->_spectrumInfoInternal.szMango,
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iChargeState);
   }
   else
   {
      fprintf(fpout, " <spectrum_query spectrum=\"%s.%05d.%05d.%d\"",
            pStr,
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iScanNumber,
            pQuery->_spectrumInfoInternal.iChargeState);
   }

   if (pQuery->_spectrumInfoInternal.szNativeID[0]!='\0')
   {
      string sNativeID = pQuery->_spectrumInfoInternal.szNativeID;
      CometMassSpecUtils::EscapeString(sNativeID);
      fprintf(fpout, " spectrumNativeID=\"%s\"", sNativeID.c_str());
   }

   fprintf(fpout, " start_scan=\"%d\"", pQuery->_spectrumInfoInternal.iScanNumber);
   fprintf(fpout, " end_scan=\"%d\"", pQuery->_spectrumInfoInternal.iScanNumber);
   fprintf(fpout, " precursor_neutral_mass=\"%0.6f\"", pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS);
   fprintf(fpout, " assumed_charge=\"%d\"", pQuery->_spectrumInfoInternal.iChargeState);
   fprintf(fpout, " index=\"%d\"", iNumSpectraSearched + iWhichQuery + 1);

   if (pQuery->_spectrumInfoInternal.dRTime > 0.0)
      fprintf(fpout, " retention_time_sec=\"%0.1f\">\n", pQuery->_spectrumInfoInternal.dRTime);
   else
      fprintf(fpout, ">\n");

   fprintf(fpout, "  <search_result>\n");
   fflush(fpout);

   Results *pOutput;

   if (iPrintTargetDecoy == 2)
   {
      pOutput = pQuery->_pDecoys;
      iNumPrintLines = pQuery->iDecoyMatchPeptideCount;
   }
   else
   {
      pOutput = pQuery->_pResults;
      iNumPrintLines = pQuery->iMatchPeptideCount;
   }

   if (iNumPrintLines > (g_staticParams.options.iNumPeptideOutputLines))
      iNumPrintLines = (g_staticParams.options.iNumPeptideOutputLines);

   iMinLength = 999;
   for (i=0; i<iNumPrintLines; ++i)
   {
      int iLen = (int)strlen(pOutput[i].szPeptide);
      if (iLen == 0)
         break;
      if (iLen < iMinLength)
         iMinLength = iLen;
   }

   for (int iWhichResult=0; iWhichResult<iNumPrintLines; ++iWhichResult)
   {
      if (pOutput[iWhichResult].fXcorr > g_staticParams.options.dMinimumXcorr)
         PrintPepXMLSearchHit(iWhichQuery, iWhichResult, iPrintTargetDecoy, pOutput, fpout, fpdb);
   }

   fprintf(fpout, "  </search_result>\n");
   fprintf(fpout, " </spectrum_query>\n");
}


void CometWritePepXML::PrintPepXMLSearchHit(int iWhichQuery,
                                            int iWhichResult,
                                            int iPrintTargetDecoy,
                                            Results *pOutput,
                                            FILE *fpout,
                                            FILE *fpdb)
{
   int  i;
   int iNTT;
   int iNMC;
   unsigned int uiNumTotProteins = 0;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   CalcNTTNMC(pOutput, iWhichResult, &iNTT, &iNMC);

   std::vector<string> vProteinTargets;  // store vector of target protein names
   std::vector<string> vProteinDecoys;   // store vector of decoy protein names
   std::vector<string>::iterator it;

   bool bReturnFulProteinString = false;
   CometMassSpecUtils::GetProteinNameString(fpdb, iWhichQuery, iWhichResult, iPrintTargetDecoy, bReturnFulProteinString, &uiNumTotProteins, vProteinTargets, vProteinDecoys);

   fprintf(fpout, "   <search_hit hit_rank=\"%d\"", pOutput[iWhichResult].iRankXcorr);
   fprintf(fpout, " peptide=\"%s\"", pOutput[iWhichResult].szPeptide);
   fprintf(fpout, " peptide_prev_aa=\"%c\"", pOutput[iWhichResult].cPrevAA);
   fprintf(fpout, " peptide_next_aa=\"%c\"", pOutput[iWhichResult].cNextAA);

   string sTmp;

   if (iPrintTargetDecoy == 0)
   {
      if (vProteinTargets.size() > 0)
      {
         it = vProteinTargets.begin();
         sTmp = *it;
         CometMassSpecUtils::EscapeString(sTmp);
         fprintf(fpout, " protein=\"%s\"", sTmp.c_str());
         ++it;
      }
      else if (vProteinDecoys.size() > 0)
      {
         it = vProteinDecoys.begin();
         sTmp = g_staticParams.szDecoyPrefix;
         sTmp += *it;
         CometMassSpecUtils::EscapeString(sTmp);
         fprintf(fpout, " protein=\"%s\"", sTmp.c_str());
         ++it;
      }
   }
   else if (iPrintTargetDecoy == 1)
   {
      it = vProteinTargets.begin();
      sTmp = *it;
      CometMassSpecUtils::EscapeString(sTmp);
      fprintf(fpout, " protein=\"%s\"", sTmp.c_str());
      ++it;
   }
   else //if (iPrintTargetDecoy == 2)
   {
      it = vProteinDecoys.begin();
      sTmp = g_staticParams.szDecoyPrefix;
      sTmp += *it;
      CometMassSpecUtils::EscapeString(sTmp);
      fprintf(fpout, " protein=\"%s\"", sTmp.c_str());
      ++it;
   }

   fprintf(fpout, " num_tot_proteins=\"%u\"", uiNumTotProteins);
   fprintf(fpout, " num_matched_ions=\"%d\"", pOutput[iWhichResult].iMatchedIons);
   fprintf(fpout, " tot_num_ions=\"%d\"", pOutput[iWhichResult].iTotalIons);
   fprintf(fpout, " calc_neutral_pep_mass=\"%0.6f\"", pOutput[iWhichResult].dPepMass - PROTON_MASS);
   fprintf(fpout, " massdiff=\"%0.6f\"", pQuery->_pepMassInfo.dExpPepMass - pOutput[iWhichResult].dPepMass);
   fprintf(fpout, " num_tol_term=\"%d\"", iNTT);
   fprintf(fpout, " num_missed_cleavages=\"%d\"", iNMC);
   fprintf(fpout, " num_matched_peptides=\"%lu\"", (iPrintTargetDecoy == 2)?(pQuery->_uliNumMatchedDecoyPeptides):(pQuery->_uliNumMatchedPeptides));
   fprintf(fpout, ">\n");

   if (iPrintTargetDecoy != 2)  // if not decoy only, print target proteins
   {
      if (vProteinTargets.size() > 0)
      {
         for (; it != vProteinTargets.end(); ++it)
         {
            sTmp = *it;
            CometMassSpecUtils::EscapeString(sTmp);
            fprintf(fpout, "    <alternative_protein protein=\"%s\"/>\n", sTmp.c_str());
         }
      }
   }

   if (iPrintTargetDecoy != 1)  // if not target only, print decoy proteins
   {
      if (vProteinDecoys.size() > 0)
      {
         it = vProteinDecoys.begin();
         if (vProteinTargets.size() == 0)  // skip the first decoy entry if it would've been printed as main protein
            ++it;
         for (; it != vProteinDecoys.end(); ++it)
         {
            sTmp = *it;
            CometMassSpecUtils::EscapeString(sTmp);
            fprintf(fpout, "    <alternative_protein protein=\"%s\"/>\n", sTmp.c_str());
         }
      }
   }

   // check if peptide is modified
   bool bModified = 0;

   if (!isEqual(g_staticParams.staticModifications.dAddNterminusPeptide, 0.0)
         || !isEqual(g_staticParams.staticModifications.dAddCterminusPeptide, 0.0))
      bModified = 1;

   if (pOutput[iWhichResult].cPrevAA=='-' && !isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0))
      bModified = 1;
   if (pOutput[iWhichResult].cNextAA=='-' && !isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0))
      bModified = 1;

   //if (pOutput[iWhichResult].cPeffOrigResidue != '\0' && pOutput[iWhichResult].iPeffOrigResiduePosition != -9)
   if (!pOutput[iWhichResult].sPeffOrigResidues.empty() && pOutput[iWhichResult].iPeffOrigResiduePosition != NO_PEFF_VARIANT)
      bModified = 1;

   if (!bModified)
   {
      for (i=0; i<pOutput[iWhichResult].iLenPeptide; ++i)

      {
         if (!isEqual(g_staticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]], 0.0)
               || pOutput[iWhichResult].piVarModSites[i] != 0)
         {
            bModified = 1;
            break;
         }
      }

      // check n- and c-terminal variable mods
      i=pOutput[iWhichResult].iLenPeptide;
      if (pOutput[iWhichResult].piVarModSites[i] != 0  || pOutput[iWhichResult].piVarModSites[i+1] != 0)
         bModified = 1;
   }

   if (bModified)
   {
      // construct modified peptide string
      char szModPep[512];

      szModPep[0]='\0';

      bool bNterm = false;
      bool bNtermVariable = false;
      bool bCterm = false;
      bool bCtermVariable = false;
      double dNterm = 0.0;
      double dCterm = 0.0;

      // See if n-term mod (static and/or variable) needs to be reported
      if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide] > 0
            || !isEqual(g_staticParams.staticModifications.dAddNterminusPeptide, 0.0)
            || (pOutput[iWhichResult].cPrevAA=='-'
               && !isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0)) )
      {
         bNterm = true;

         // pepXML format reports modified term mass (vs. mass diff)
         dNterm = g_staticParams.precalcMasses.dNtermProton - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
         {
            dNterm += g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide]-1].dVarModMass;
            bNtermVariable = true;
         }

         if (pOutput[iWhichResult].cPrevAA=='-' && !isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0))
            dNterm += g_staticParams.staticModifications.dAddNterminusProtein;
      }

      // See if c-term mod (static and/or variable) needs to be reported
      if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0
            || !isEqual(g_staticParams.staticModifications.dAddCterminusPeptide, 0.0)
            || (pOutput[iWhichResult].cNextAA=='-'
               && !isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0)) )
      {
         bCterm = true;

         dCterm = g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS - g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0)
         {
            dCterm += g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].dVarModMass;
            bCtermVariable = true;
         }

         if (pOutput[iWhichResult].cNextAA=='-' && !isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0))
            dCterm += g_staticParams.staticModifications.dAddCterminusProtein;
      }

      // generate modified_peptide string
      if (bNtermVariable)
         sprintf(szModPep+strlen(szModPep), "n[%0.0f]", dNterm);
      for (i=0; i<pOutput[iWhichResult].iLenPeptide; ++i)
      {
         sprintf(szModPep+strlen(szModPep), "%c", pOutput[iWhichResult].szPeptide[i]);

         if (pOutput[iWhichResult].piVarModSites[i] != 0)
         {
            sprintf(szModPep+strlen(szModPep), "[%0.0f]",
                  pOutput[iWhichResult].pdVarModSites[i] + g_staticParams.massUtility.pdAAMassFragment[(int)pOutput[iWhichResult].szPeptide[i]]);
         }
      }
      if (bCtermVariable)
         sprintf(szModPep+strlen(szModPep), "c[%0.0f]", dCterm);

      fprintf(fpout, "    <modification_info modified_peptide=\"%s\"", szModPep);
      if (bNterm)
         fprintf(fpout, " mod_nterm_mass=\"%0.6f\"", dNterm);
      if (bCterm)
         fprintf(fpout, " mod_cterm_mass=\"%0.6f\"", dCterm);
      fprintf(fpout, ">\n");

      for (i=0; i<pOutput[iWhichResult].iLenPeptide; ++i)
      {
         if (!isEqual(g_staticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]], 0.0)
               || pOutput[iWhichResult].piVarModSites[i] != 0)
         {
            int iResidue = (int)pOutput[iWhichResult].szPeptide[i];
            double dStaticMass = g_staticParams.staticModifications.pdStaticMods[iResidue];

            fprintf(fpout, "     <mod_aminoacid_mass position=\"%d\" mass=\"%0.6f\"",
                  i+1,
                  g_staticParams.massUtility.pdAAMassFragment[iResidue] + pOutput[iWhichResult].pdVarModSites[i]);
            
            if (!isEqual(dStaticMass, 0.0))
               fprintf(fpout, " static=\"%0.6f\"", dStaticMass);

            if (pOutput[iWhichResult].piVarModSites[i] != 0)
               fprintf(fpout, " variable=\"%0.6f\"", pOutput[iWhichResult].pdVarModSites[i]);

            if (pOutput[iWhichResult].piVarModSites[i] < 0)
            {
               fprintf(fpout, " source=\"peff\" id=\"%s\"/>\n", pOutput[iWhichResult].pszMod[i]);
            }
            else if (pOutput[iWhichResult].piVarModSites[i] > 0)
               fprintf(fpout, " source=\"param\"/>\n");
            else
               fprintf(fpout, "/>\n");
         }
      }

      // Report PEFF substitution
      if (!pOutput[iWhichResult].sPeffOrigResidues.empty() && pOutput[iWhichResult].iPeffOrigResiduePosition != NO_PEFF_VARIANT)
      {
         if (pOutput[iWhichResult].iPeffOrigResiduePosition < 0)
         {
            if (pOutput[iWhichResult].iPeffOrigResiduePosition == -1 && pOutput[iWhichResult].sPeffOrigResidues.size() == 1 && pOutput[iWhichResult].iPeffNewResidueCount == 1) // single aa substitution
            {
               // case where a single amino acid substitution one prior to the start of the peptide caused the peptide sequence (i.e. creation of an enzyme cut site)
               fprintf(fpout, "     <aminoacid_substitution peptide_prev_aa=\"%c\" orig_aa=\"%s\"/>\n",
                     pOutput[iWhichResult].cPrevAA, pOutput[iWhichResult].sPeffOrigResidues.c_str());
            }
            else 
            {
               // where more than one amino acid was involved prior to the peptide: i.e. deletion or insertion
               int iPepPos = pOutput[iWhichResult].iPeffOrigResiduePosition + pOutput[iWhichResult].iPeffNewResidueCount;
               if (iPepPos == 0 || iPepPos <= (int)strlen(pOutput[iWhichResult].szPeptide))
               { 
                  fprintf(fpout, "     <aminoacid_substitution peptide_prev_aa=\"%c\" orig_aa=\"%c\"/>\n",
                        pOutput[iWhichResult].cPrevAA, pOutput[iWhichResult].sPeffOrigResidues.back());
               } 
               if (iPepPos > 0)
               {
                  if (iPepPos > (int)strlen(pOutput[iWhichResult].szPeptide))
                     iPepPos = (int)strlen(pOutput[iWhichResult].szPeptide);
                  fprintf(fpout, "     <sequence_substitution position=\"1\" num_aas=\"%d\" orig_aas=\"\"/>\n",
                        iPepPos);
               }
            }
         }
         else if (pOutput[iWhichResult].iPeffOrigResiduePosition == pOutput[iWhichResult].iLenPeptide)
         {
            // case where a single amino acid substitution one after the end of the peptide caused the peptide sequence (i.e. creation of an enzyme cut site)
            fprintf(fpout, "     <aminoacid_substitution peptide_next_aa=\"%c\" orig_aa=\"%c\"/>\n",
                  pOutput[iWhichResult].cNextAA, pOutput[iWhichResult].sPeffOrigResidues[0]);
         }
         else if (pOutput[iWhichResult].sPeffOrigResidues.size() == 1 && pOutput[iWhichResult].iPeffNewResidueCount == 1) // single aa substitution
         {
            fprintf(fpout, "     <aminoacid_substitution position=\"%d\" orig_aa=\"%c\"/>\n",
                  pOutput[iWhichResult].iPeffOrigResiduePosition + 1, pOutput[iWhichResult].sPeffOrigResidues[0]);
         }
         else  //insertion or deletion within the peptide sequence
         {
            int rc = pOutput[iWhichResult].iPeffNewResidueCount;
            if (rc > pOutput[iWhichResult].iPeffOrigResiduePosition + 1)
               rc = pOutput[iWhichResult].iPeffOrigResiduePosition + 1;
            fprintf(fpout, "     <sequence_substitution position=\"%d\" num_aas=\"%d\" orig_aas=\"%s\"/>\n",
                  pOutput[iWhichResult].iPeffOrigResiduePosition + 1, rc, pOutput[iWhichResult].sPeffOrigResidues.c_str());
                 
         }
      }


      fprintf(fpout, "    </modification_info>\n");
   }

   fprintf(fpout, "    <search_score name=\"xcorr\" value=\"%0.4f\"/>\n", pOutput[iWhichResult].fXcorr);
   fprintf(fpout, "    <search_score name=\"deltacn\" value=\"%0.4f\"/>\n", pOutput[iWhichResult].fDeltaCn);
   fprintf(fpout, "    <search_score name=\"spscore\" value=\"%0.1f\"/>\n", pOutput[iWhichResult].fScoreSp);
   fprintf(fpout, "    <search_score name=\"sprank\" value=\"%d\"/>\n", pOutput[iWhichResult].iRankSp);
   fprintf(fpout, "    <search_score name=\"expect\" value=\"%0.2E\"/>\n", pOutput[iWhichResult].dExpect);

   if (g_staticParams.options.bExportAdditionalScoresPepXML)
   {
      fprintf(fpout, "    <search_score name=\"lnrSp\" value=\"%0.4f\"/>\n", log((double)pOutput[iWhichResult].iRankSp));
      fprintf(fpout, "    <search_score name=\"deltLCn\" value=\"%0.4f\"/>\n", pOutput[iWhichResult].fLastDeltaCn);
      fprintf(fpout, "    <search_score name=\"lnExpect\" value=\"%0.4f\"/>\n", log(pOutput[iWhichResult].dExpect));
      fprintf(fpout, "    <search_score name=\"IonFrac\" value=\"%0.4f\"/>\n", (double)pOutput[iWhichResult].iMatchedIons / pOutput[iWhichResult].iTotalIons);

      unsigned long uliNumMatch;
      if (iPrintTargetDecoy == 2)
         uliNumMatch = pQuery->_uliNumMatchedDecoyPeptides;
      else
         uliNumMatch = pQuery->_uliNumMatchedPeptides;
      if (uliNumMatch > 0)
         fprintf(fpout, "    <search_score name=\"lnNumSP\" value=\"%0.4f\"/>\n",  log((double)(uliNumMatch)));
      else
         fprintf(fpout, "    <search_score name=\"lnNumSP\" value=\"%0.4f\"/>\n",  -20.0);
   }

   fprintf(fpout, "   </search_hit>\n");
}


void CometWritePepXML::ReadInstrument(char *szManufacturer,
                                      char *szModel)
{
   strcpy(szManufacturer, "UNKNOWN");
   strcpy(szModel, "UNKNOWN");

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
   if (pOutput[iWhichResult].cPrevAA=='-')
   {
      *iNTT += 1;
   }
   else if (g_staticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].cPrevAA)
            && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[0]))
      {
         *iNTT += 1;
      }
   }
   else
   {
      if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[0])
            && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].cPrevAA))
      {
         *iNTT += 1;
      }
   }

   if (pOutput[iWhichResult].cNextAA=='-')
   {
      *iNTT += 1;
   }
   else if (g_staticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[pOutput[iWhichResult].iLenPeptide -1])
            && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].cNextAA))
      {
         *iNTT += 1;
      }
   }
   else
   {
      if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].cNextAA)
            && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[pOutput[iWhichResult].iLenPeptide -1]))
      {
         *iNTT += 1;
      }
   }

   // Calculate number of missed cleavage (NMC) sites based on sample_enzyme
   if (g_staticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      for (i=0; i<pOutput[iWhichResult].iLenPeptide-1; ++i)
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
      for (i=1; i<pOutput[iWhichResult].iLenPeptide; ++i)
      {
         if (strchr(g_staticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[i])
               && !strchr(g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[i-1]))
         {
            *iNMC += 1;
         }
      }
   }

}
