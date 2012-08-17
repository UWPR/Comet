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
#include "CometMassSpecUtils.h"
#include "CometWritePepXML.h"


CometWritePepXML::CometWritePepXML()
{
}


CometWritePepXML::~CometWritePepXML()
{
}


void CometWritePepXML::WritePepXML(FILE *fpout,
                                   FILE *fpoutd,
                                   char *szOutput,
                                   char *szOutputDecoy,
                                   char *szParamsFile)
{

   int i;

   WriteXMLHeader(fpout, szParamsFile);

   if (g_StaticParams.options.iDecoySearch == 2)
      WriteXMLHeader(fpoutd, szParamsFile);

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      PrintResults(i, 0, fpout, szOutput);
   }

   // Print out the separate decoy hits.
   if (g_StaticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); i++)
      {
         PrintResults(i, 1, fpoutd, szOutputDecoy);
      }
   }

   fprintf(fpout, " </msms_run_summary>\n");
   fprintf(fpout, "</msms_pipeline_analysis>\n");
}


void CometWritePepXML::WriteXMLHeader(FILE *fpout,
                                      char *szParamsFile)
{
   time_t tTime;
   char szDate[48];
   char szManufacturer[SIZE_FILE];
   char szModel[SIZE_FILE];

   time(&tTime);
   strftime(szDate, 46, "%Y-%m-%dT%H:%M:%S", localtime(&tTime));

   // Get msModel + msManufacturer from mzXML. Easy way to get from mzML too?
   ReadInstrument(szManufacturer, szModel);

   // Write out pepXML header.
   fprintf(fpout, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
   fprintf(fpout, " <?xml-stylesheet type=\"text/xsl\" href=\"http://localhost/tpp/schema/pepXML_std.xsl\"?>\n");
   
   fprintf(fpout, " <msms_pipeline_analysis date=\"%s\" ", szDate);
   fprintf(fpout, "xmlns=\"http://regis-web.systemsbiology.net/pepXML\" ");
   fprintf(fpout, "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
   fprintf(fpout, "xsi:schemaLocation=\"http://sashimi.sourceforge.net/schema_revision/pepXML/pepXML_v117.xsd\" ");
   fprintf(fpout, "summary_xml=\"%s.pep.xml\">\n", g_StaticParams.inputFile.szBaseName);
   fprintf(fpout, " <msms_run_summary base_name=\"%s\" ", g_StaticParams.inputFile.szBaseName);

   fprintf(fpout, "msManufacturer=\"%s\" ", szManufacturer);
   fprintf(fpout, "msModel=\"%s\" ", szModel);

   // Grab file extension from file name
   char *pStr = strrchr(g_StaticParams.inputFile.szFileName, '.');
   fprintf(fpout, "raw_data=\"%s\" ", pStr+1);
   fprintf(fpout, "raw_data_type=\"%s\">\n", pStr+1);

   fprintf(fpout, " <sample_enzyme name=\"%s\">\n", g_StaticParams.enzymeInformation.szSampleEnzymeName);
   fprintf(fpout, "  <specificity cut=\"%s\" no_cut=\"%s\" sense=\"%c\"/>\n",
         g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA,
         g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA,
         g_StaticParams.enzymeInformation.iSearchEnzymeOffSet?'C':'N');
   fprintf(fpout, " </sample_enzyme>\n");

   fprintf(fpout, " <search_summary base_name=\"%s\"", g_StaticParams.inputFile.szBaseName);
   fprintf(fpout, " search_engine=\"Comet\" search_engine_version=\"%s\"", version);
   fprintf(fpout, " precursor_mass_type=\"%s\"", g_StaticParams.massUtility.bMonoMassesParent?"monoisotopic":"average");
   fprintf(fpout, " fragment_mass_type=\"%s\"", g_StaticParams.massUtility.bMonoMassesFragment?"monoisotopic":"average");
   fprintf(fpout, " search_id=\"1\">\n");

   fprintf(fpout, "  <search_database local_path=\"%s\"", g_StaticParams.databaseInfo.szDatabase);
   fprintf(fpout, " type=\"%s\"/>\n", g_StaticParams.options.iWhichReadingFrame==0?"AA":"NA");

   FILE *fp;
   char szParamBuf[SIZE_BUF];
   if ((fp=fopen(szParamsFile, "r")) == NULL)
   {
      fprintf(stderr, " Error - cannot open parameter file %s.\n\n", szParamsFile);
      exit(1);
   }

   fprintf(fpout, "  <enzymatic_search_constraint enzyme=\"%s\" max_num_internal_cleavages=\"%d\" min_number_termini=\"%d\"/>\n",
         g_StaticParams.enzymeInformation.szSearchEnzymeName,
         g_StaticParams.enzymeInformation.iAllowedMissedCleavage,
         (g_StaticParams.options.iEnzymeTermini==ENZYME_DOUBLE_TERMINI)?2:
            ((g_StaticParams.options.iEnzymeTermini == ENZYME_SINGLE_TERMINI)
            || (g_StaticParams.options.iEnzymeTermini == ENZYME_N_TERMINI)
            || (g_StaticParams.options.iEnzymeTermini == ENZYME_C_TERMINI))?1:0);

   // Write out properly encoded mods
   bool bPrintVariableC = false;
   bool bPrintVariableN = false;
   bool bProteinTerminusC = false;
   bool bProteinTerminusN = false;
   char szVariableCterm[256];
   char szVariableNterm[256];
   while (fgets(szParamBuf, SIZE_BUF, fp))
   {
      char szParamName[128];
      char szParamVal[128];

      if ((pStr = strchr(szParamBuf, '#')))  // remove comments
         *pStr='\0';
      if (strchr(szParamBuf, '=') && (strstr(szParamBuf, "variable_") || strstr(szParamBuf, "add_")))
      {
         double dMass;

         sscanf(szParamBuf, "%s", szParamName);
         pStr = strchr(szParamBuf, '=');
         while (isspace(*(pStr+1)))  // remove beginning white space
            pStr++;
         strcpy(szParamVal, pStr+1);
         while (isspace(szParamVal[strlen(szParamVal)-1])) // remove trailing white space
            szParamVal[strlen(szParamVal)-1]='\0';

         if (strstr(szParamName, "variable_mod") && strlen(szParamName)==13)
         {
            char szModChar[40];
            int bBinary;
            int i;
            char cSymbol;

            sscanf(szParamVal, "%lf %s %d", &dMass, szModChar, &bBinary);

            cSymbol = '-';
            if (szParamName[12]=='1')
               cSymbol = g_StaticParams.variableModParameters.cModCode[0];
            else if (szParamName[12]=='2')
               cSymbol = g_StaticParams.variableModParameters.cModCode[1];
            else if (szParamName[12]=='3')
               cSymbol = g_StaticParams.variableModParameters.cModCode[2];
            else if (szParamName[12]=='4')
               cSymbol = g_StaticParams.variableModParameters.cModCode[3];
            else if (szParamName[12]=='5')
               cSymbol = g_StaticParams.variableModParameters.cModCode[4];
            else if (szParamName[12]=='6')
               cSymbol = g_StaticParams.variableModParameters.cModCode[5];

            if (cSymbol != '-' && dMass!=0.0)
            {
               for (i=0; i<(int)strlen(szModChar); i++)
               {
                  fprintf(fpout, "  <aminoacid_modification aminoacid=\"%c\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"Y\" %ssymbol=\"%c\"/>\n",
                        szModChar[i],
                        dMass,
                        dMass + g_StaticParams.massUtility.pdAAMassParent[(int)szModChar[i]],
                        (bBinary?"binary=\"Y\" ":""),
                        cSymbol);
               }
            }
         }

         else if (!strcmp(szParamName, "variable_C_terminus"))
         {
            if (g_StaticParams.variableModParameters.dVarModMassC != 0.0)
            {
               sprintf(szVariableCterm, "  <terminal_modification terminus=\"C\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"Y\" protein_terminus=\"",
                     g_StaticParams.variableModParameters.dVarModMassC,
                     g_StaticParams.variableModParameters.dVarModMassC + g_StaticParams.precalcMasses.dCtermOH2Proton);
               bPrintVariableC = 1;
            }
         }

         else if (!strcmp(szParamName, "variable_N_terminus"))
         {
            if (g_StaticParams.variableModParameters.dVarModMassN != 0.0)
            {
               sprintf(szVariableNterm, "  <terminal_modification terminus=\"N\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"Y\" protein_terminus=\"",
                     g_StaticParams.variableModParameters.dVarModMassN,
                     g_StaticParams.variableModParameters.dVarModMassN + g_StaticParams.precalcMasses.dNtermProton);
               bPrintVariableN = 1;
            }
         }

         else if (!strcmp(szParamName, "variable_N_terminus_distance"))
         {
            int iDistance;
            sscanf(szParamVal, "%d", &iDistance);
            if (iDistance == 0)
               bProteinTerminusN = 1;
         }
         else if (!strcmp(szParamName, "variable_C_terminus_distance"))
         {
            int iDistance;
            sscanf(szParamVal, "%d", &iDistance);
            if (iDistance == 0)
               bProteinTerminusC = 1;
         }

         else if (!strcmp(szParamName, "add_Cterm_peptide"))
         {
            sscanf(szParamVal, "%lf", &dMass);
            if (dMass != 0.0)
            {
               fprintf(fpout, "  <terminal_modification terminus=\"C\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"N\" protein_terminus=\"N\"/>\n",
                     dMass, dMass + g_StaticParams.precalcMasses.dCtermOH2Proton);
            }
         }
         else if (!strcmp(szParamName, "add_Nterm_peptide"))
         {
            sscanf(szParamVal, "%lf", &dMass);
            if (dMass != 0.)
            {
               fprintf(fpout, "  <terminal_modification terminus=\"N\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"N\" protein_terminus=\"N\"/>\n",
                     dMass, dMass + g_StaticParams.precalcMasses.dNtermProton);
            }
         }
         else if (!strcmp(szParamName, "add_Cterm_protein"))
         {
            sscanf(szParamVal, "%lf", &dMass);
            if (dMass != 0.0)
            {
               fprintf(fpout, "  <terminal_modification terminus=\"C\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"N\" protein_terminus=\"Y\"/>\n",
                     dMass, dMass + g_StaticParams.precalcMasses.dCtermOH2Proton);
            }
         }
         else if (!strcmp(szParamName, "add_Nterm_protein"))
         {
            sscanf(szParamVal, "%lf", &dMass);
            if (dMass != 0.0)
            {
               fprintf(fpout, "  <terminal_modification terminus=\"N\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"N\" protein_terminus=\"Y\"/>\n",
                     dMass, dMass + g_StaticParams.precalcMasses.dNtermProton);
            }
         }

         else if (!strncmp(szParamName, "add_", 4) && szParamName[5]=='_')  // add_?_
         {
            sscanf(szParamVal, "%lf", &dMass);
            if (dMass != 0.0)
            {
               fprintf(fpout, "  <aminoacid_modification aminoacid=\"%c\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"N\"/>\n",
                     szParamName[4], dMass, dMass + g_StaticParams.massUtility.pdAAMassParent[(int)szParamName[4]]);
            }
         }

      }
   }

   if (bPrintVariableC)
   {
      fprintf(fpout, "%s%c\"/>\n", szVariableCterm, (bProteinTerminusC?'Y':'N'));
   }

   if (bPrintVariableN)
   {
      fprintf(fpout, "%s%c\"/>\n", szVariableNterm, (bProteinTerminusN?'Y':'N'));
   }

   // Write out all parameters from params file
   rewind(fp);
   while (fgets(szParamBuf, SIZE_BUF, fp))
   {
      char szParamName[128];
      char szParamVal[128];

      if ((pStr = strchr(szParamBuf, '#')))  // remove comments
         *pStr='\0';
      if (strchr(szParamBuf, '='))
      {
         sscanf(szParamBuf, "%s", szParamName);
         pStr = strchr(szParamBuf, '=');
         while (isspace(*(pStr+1)))  // remove beginning white space
            pStr++;
         strcpy(szParamVal, pStr+1);
         while (isspace(szParamVal[strlen(szParamVal)-1])) // remove trailing white space
            szParamVal[strlen(szParamVal)-1]='\0';

         fprintf(fpout, "  <parameter name=\"%s\" value=\"%s\"/>\n", szParamName, szParamVal);
      }
   }
   fclose(fp);

   fprintf(fpout, " </search_summary>\n");
   fflush(fpout);
}


void CometWritePepXML::PrintResults(int iWhichQuery,
                                    bool bDecoy,
                                    FILE *fpout,
                                    char *szOutput)
{
   int  i,
        iDoXcorrCount,
        iRankXcorr;

   // Print spectrum_query element. 
   fprintf(fpout, " <spectrum_query spectrum=\"%s.%05d.%05d.%d\"",
         g_StaticParams.inputFile.szBaseName,
         g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber,
         g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber,
         g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iChargeState);
   fprintf(fpout, " start_scan=\"%d\"", g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber);
   fprintf(fpout, " end_scan=\"%d\"", g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iScanNumber);
   fprintf(fpout, " precursor_neutral_mass=\"%0.4f\"", g_pvQuery.at(iWhichQuery)->_pepMassInfo.dExpPepMass - PROTON_MASS);
   fprintf(fpout, " assumed_charge=\"%d\"", g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.iChargeState);
   fprintf(fpout, " index=\"%d\"", iWhichQuery+1);
   if (mzXML)
      fprintf(fpout, " retention_time_sec=\"%0.1f\">\n", g_pvQuery.at(iWhichQuery)->_spectrumInfoInternal.dRTime);
   else
      fprintf(fpout, ">\n");

   fprintf(fpout, "  <search_result>\n");

   if (bDecoy)
      iDoXcorrCount = g_pvQuery.at(iWhichQuery)->iDoDecoyXcorrCount;
   else
      iDoXcorrCount = g_pvQuery.at(iWhichQuery)->iDoXcorrCount;

   // Print out each sequence line.
   if (iDoXcorrCount > (g_StaticParams.options.iNumPeptideOutputLines))
      iDoXcorrCount = (g_StaticParams.options.iNumPeptideOutputLines);

   Results *pOutput;

   if (bDecoy)
      pOutput = g_pvQuery.at(iWhichQuery)->_pDecoys;
   else
      pOutput = g_pvQuery.at(iWhichQuery)->_pResults;

   iRankXcorr = 1;

   for (i=0; i<iDoXcorrCount; i++)
   {
      if ((i > 0) && (pOutput[i].fXcorr != pOutput[i-1].fXcorr))
         iRankXcorr++;

      if (pOutput[i].fXcorr > 0)
         PrintPepXMLSearchHit(iWhichQuery, iRankXcorr, i, iDoXcorrCount, bDecoy, pOutput, fpout);
   } 

   fprintf(fpout, "  </search_result>\n");
   fprintf(fpout, " </spectrum_query>\n");
}


void CometWritePepXML::PrintPepXMLSearchHit(int iWhichQuery,
                                            int iRankXcorr,
                                            int iWhichResult,
                                            int iDoXcorrCount,
                                            bool bDecoy,
                                            Results *pOutput,
                                            FILE *fpout)
{
   int  i;
   int iNTT;
   int iNMC;



   fprintf(fpout, "   <search_hit hit_rank=\"%d\"", iWhichResult+1);
   fprintf(fpout, " peptide=\"%s\"", pOutput[iWhichResult].szPeptide);
   fprintf(fpout, " peptide_prev_aa=\"%c\"", pOutput[iWhichResult].szPrevNextAA[0]);
   fprintf(fpout, " peptide_next_aa=\"%c\"", pOutput[iWhichResult].szPrevNextAA[1]);
   fprintf(fpout, " protein=\"%s\"", pOutput[iWhichResult].szProtein);
   fprintf(fpout, " num_tot_proteins=\"%d\"", pOutput[iWhichResult].iDuplicateCount+1);
   fprintf(fpout, " num_matched_ions=\"%d\"", pOutput[iWhichResult].iMatchedIons);
   fprintf(fpout, " tot_num_ions=\"%d\"", pOutput[iWhichResult].iTotalIons);
   fprintf(fpout, " calc_neutral_pep_mass=\"%0.4f\"", pOutput[iWhichResult].dPepMass - PROTON_MASS);
   fprintf(fpout, " massdiff=\"%0.4f\"", g_pvQuery.at(iWhichQuery)->_pepMassInfo.dExpPepMass -pOutput[iWhichResult].dPepMass);

   CalcNTTNMC(pOutput, iWhichResult, &iNTT, &iNMC);

   fprintf(fpout, " num_tol_term=\"%d\"", iNTT);
   fprintf(fpout, " num_missed_cleavages=\"%d\"", iNMC); 
   fprintf(fpout, " num_matched_peptides=\"%ld\"",
         bDecoy?g_pvQuery.at(iWhichQuery)->_liNumMatchedDecoyPeptides:g_pvQuery.at(iWhichQuery)->_liNumMatchedPeptides);
   fprintf(fpout, ">\n");

   // check if peptide is modified
   bool bModified = 0;
   if (g_StaticParams.variableModParameters.bVarModSearch)
   {
      if (!bModified)
      {
         for (i=0; i<pOutput[iWhichResult].iLenPeptide+2; i++)
         {
            if (pOutput[iWhichResult].pcVarModSites[i] > 0)
            {
               bModified = 1;
               break;
            }
         }
      }
   }

   if (bModified)
   {
      // construct modified peptide string
      char szModPep[256];
      
      szModPep[0]='\0';

      if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] == 1)
      {
         sprintf(szModPep+strlen(szModPep), "n[%0.0f]",
               g_StaticParams.variableModParameters.dVarModMassN
               + g_StaticParams.precalcMasses.dNtermProton);
      }

      for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
      {
         sprintf(szModPep+strlen(szModPep), "%c", pOutput[iWhichResult].szPeptide[i]);

         if (pOutput[iWhichResult].pcVarModSites[i] > 0)
         {
            sprintf(szModPep+strlen(szModPep), "[%0.0f]",
                  g_StaticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass
                  + g_StaticParams.massUtility.pdAAMassFragment[(int)pOutput[iWhichResult].szPeptide[i]]);
         }
      }

      if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] == 1)
      {
         sprintf(szModPep+strlen(szModPep), "c[%0.0f]",
              g_StaticParams.variableModParameters.dVarModMassC
              + g_StaticParams.precalcMasses.dCtermOH2Proton);
      }

      fprintf(fpout, "    <modification_info modified_peptide=\"%s\"", szModPep);

      if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] == 1)
      {
         fprintf(fpout, " mod_nterm_mass=\"%0.4f\"",
              g_StaticParams.variableModParameters.dVarModMassN
              + g_StaticParams.precalcMasses.dNtermProton);
      }
      if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] == 1)
      {
         fprintf(fpout, " mod_cterm_mass=\"%0.4f\"",
              g_StaticParams.variableModParameters.dVarModMassC
              + g_StaticParams.precalcMasses.dCtermOH2Proton);
      }
      fprintf(fpout, ">\n");

      for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
      {
         if (pOutput[iWhichResult].pcVarModSites[i] > 0)
         {
            fprintf(fpout, "     <mod_aminoacid_mass position=\"%d\" mass=\"%0.4f\"/>\n", i+1,
                  g_StaticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass
                  + g_StaticParams.massUtility.pdAAMassFragment[(int)pOutput[iWhichResult].szPeptide[i]]);
         }
      }

      fprintf(fpout, "    </modification_info>\n");
   }

   fprintf(fpout, "    <search_score name=\"xcorr\" value=\"%0.3f\"/>\n", pOutput[iWhichResult].fXcorr);

   if (iWhichResult+1 < iDoXcorrCount)
      fprintf(fpout, "    <search_score name=\"deltacn\" value=\"%0.3f\"/>\n", 1.0 - pOutput[iWhichResult+1].fXcorr/pOutput[0].fXcorr);
   else
      fprintf(fpout, "    <search_score name=\"deltacn\" value=\"%0.3f\"/>\n", 1.0);

   fprintf(fpout, "    <search_score name=\"deltacnstar\" value=\"%0.3f\"/>\n", 0.0);  // FIX
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

   if (g_StaticParams.inputFile.iInputType == InputType_MZXML)
   {
      char szBuf[SIZE_BUF];
      FILE *fp;

      if ((fp = fopen(g_StaticParams.inputFile.szFileName, "r")) != NULL)
      {
         char szMsInstrumentElement[SIZE_BUF];

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
   else if (g_StaticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      if (strchr(g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPrevNextAA[0])
            && !strchr(g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[0]))
      {
         *iNTT += 1;
      }
   }
   else
   {
      if (strchr(g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[0])
            && !strchr(g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPrevNextAA[0]))
      {
         *iNTT += 1;
      }
   }

   if (pOutput[iWhichResult].szPrevNextAA[1]=='-')
   {
      *iNTT += 1;
   }
   else if (g_StaticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      if (strchr(g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[pOutput[iWhichResult].iLenPeptide -1])
            && !strchr(g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPrevNextAA[1]))
      {
         *iNTT += 1;
      }
   }
   else
   {
      if (strchr(g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPrevNextAA[1])
            && !strchr(g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[pOutput[iWhichResult].iLenPeptide -1]))
      {
         *iNTT += 1;
      }
   }

   // Calculate number of missed cleavage (NMC) sites based on sample_enzyme
   if (g_StaticParams.enzymeInformation.iSampleEnzymeOffSet == 1)
   {
      for (i=0; i<pOutput[iWhichResult].iLenPeptide-1; i++)
      {
         if (strchr(g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[i])
               && !strchr(g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[i+1]))
         {
            *iNMC += 1;
         }
      }
   }
   else
   {
      for (i=0; i<pOutput[iWhichResult].iLenPeptide-1; i++)
      {
         if (strchr(g_StaticParams.enzymeInformation.szSampleEnzymeBreakAA, pOutput[iWhichResult].szPeptide[i])
               && !strchr(g_StaticParams.enzymeInformation.szSampleEnzymeNoBreakAA, pOutput[iWhichResult].szPeptide[i+1]))
         {
            *iNMC += 1;
         }
      }
   }

}
