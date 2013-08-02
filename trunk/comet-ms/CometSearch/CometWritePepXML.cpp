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
                                   char *szOutputDecoy)
{
   int i;

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
   {
      PrintResults(i, 0, fpout, szOutput);
   }

   // Print out the separate decoy hits.
   if (g_staticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); i++)
      {
         PrintResults(i, 1, fpoutd, szOutputDecoy);
      }
   }

   fflush(fpout);
}

void CometWritePepXML::WritePepXMLHeader(FILE *fpout,
                                      const char *szParamsFile)
{
   time_t tTime;
   char szDate[48];
   char szManufacturer[SIZE_FILE];
   char szModel[SIZE_FILE];

   time(&tTime);
   strftime(szDate, 46, "%Y-%m-%dT%H:%M:%S", localtime(&tTime));

   // Get msModel + msManufacturer from mzXML. Easy way to get from mzML too?
   ReadInstrument(szManufacturer, szModel);

   char *pStr;   // base name w/o path
   char *pStr2;  // file extension

#ifdef _WIN32
   if ( (pStr = strrchr(g_staticParams.inputFile.szBaseName, '\\')) == NULL)
#else
   if ( (pStr = strrchr(g_staticParams.inputFile.szBaseName, '/')) == NULL)
#endif
      pStr = g_staticParams.inputFile.szBaseName;
   else
      pStr++;  // skip separation character

   // Write out pepXML header.
   fprintf(fpout, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
   fprintf(fpout, " <?xml-stylesheet type=\"text/xsl\" href=\"http://localhost/tpp/schema/pepXML_std.xsl\"?>\n");
   
   fprintf(fpout, " <msms_pipeline_analysis date=\"%s\" ", szDate);
   fprintf(fpout, "xmlns=\"http://regis-web.systemsbiology.net/pepXML\" ");
   fprintf(fpout, "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
   fprintf(fpout, "xsi:schemaLocation=\"http://sashimi.sourceforge.net/schema_revision/pepXML/pepXML_v117.xsd\" ");
   fprintf(fpout, "summary_xml=\"%s.pep.xml\">\n", pStr);
   fprintf(fpout, " <msms_run_summary base_name=\"%s\" ", g_staticParams.inputFile.szBaseName);

   fprintf(fpout, "msManufacturer=\"%s\" ", szManufacturer);
   fprintf(fpout, "msModel=\"%s\" ", szModel);

   // Grab file extension from file name
   if ( (pStr2 = strrchr(g_staticParams.inputFile.szFileName, '.')) == NULL)
   {
      printf(" Error - in WriteXMLHeader missing last period in file name: %s\n", g_staticParams.inputFile.szFileName);
      exit(1);
   }
   pStr2++;
   fprintf(fpout, "raw_data=\"%s\" ", pStr2);
   fprintf(fpout, "raw_data_type=\"%s\">\n", pStr2);

   fprintf(fpout, " <sample_enzyme name=\"%s\">\n",
         (g_staticParams.options.bNoEnzymeSelected?"nonspecific":g_staticParams.enzymeInformation.szSearchEnzymeName));
   fprintf(fpout, "  <specificity cut=\"%s\" no_cut=\"%s\" sense=\"%c\"/>\n",
         g_staticParams.enzymeInformation.szSampleEnzymeBreakAA,
         g_staticParams.enzymeInformation.szSampleEnzymeNoBreakAA,
         g_staticParams.enzymeInformation.iSearchEnzymeOffSet?'C':'N');
   fprintf(fpout, " </sample_enzyme>\n");

   fprintf(fpout, " <search_summary base_name=\"%s\"", g_staticParams.inputFile.szBaseName);
   fprintf(fpout, " search_engine=\"Comet\" search_engine_version=\"%s\"", comet_version);
   fprintf(fpout, " precursor_mass_type=\"%s\"", g_staticParams.massUtility.bMonoMassesParent?"monoisotopic":"average");
   fprintf(fpout, " fragment_mass_type=\"%s\"", g_staticParams.massUtility.bMonoMassesFragment?"monoisotopic":"average");
   fprintf(fpout, " search_id=\"1\">\n");

   fprintf(fpout, "  <search_database local_path=\"%s\"", g_staticParams.databaseInfo.szDatabase);
   fprintf(fpout, " type=\"%s\"/>\n", g_staticParams.options.iWhichReadingFrame==0?"AA":"NA");

   FILE *fp;
   char szParamBuf[SIZE_BUF];
   if ((fp=fopen(szParamsFile, "r")) == NULL)
   {
      fprintf(stderr, " Error - cannot open parameter file %s.\n\n", szParamsFile);
      exit(1);
   }

   fprintf(fpout, "  <enzymatic_search_constraint enzyme=\"%s\" max_num_internal_cleavages=\"%d\" min_number_termini=\"%d\"/>\n",
         (g_staticParams.options.bNoEnzymeSelected?"nonspecific":g_staticParams.enzymeInformation.szSearchEnzymeName),
         g_staticParams.enzymeInformation.iAllowedMissedCleavage,
         (g_staticParams.options.iEnzymeTermini==ENZYME_DOUBLE_TERMINI)?2:
            ((g_staticParams.options.iEnzymeTermini == ENZYME_SINGLE_TERMINI)
            || (g_staticParams.options.iEnzymeTermini == ENZYME_N_TERMINI)
            || (g_staticParams.options.iEnzymeTermini == ENZYME_C_TERMINI))?1:0);

   // Write out properly encoded mods
   bool bPrintVariableC = 0;
   bool bPrintVariableN = 0;
   bool bProteinTerminusC = 0;
   bool bProteinTerminusN = 0;
   char szVariableCterm[256];
   char szVariableNterm[256];
   while (fgets(szParamBuf, SIZE_BUF, fp))
   {
      if ((pStr = strchr(szParamBuf, '#')))  // remove comments
         *pStr='\0';
      if (strchr(szParamBuf, '=') && (strstr(szParamBuf, "variable_") || strstr(szParamBuf, "add_")))
      {
         double dMass;
         char szParamName[128];
         char szParamVal[128];

         sscanf(szParamBuf, "%128s", szParamName);
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
            char cSymbol;

            sscanf(szParamVal, "%lf %40s %d", &dMass, szModChar, &bBinary);

            cSymbol = '-';
            if (szParamName[12]=='1')
               cSymbol = g_staticParams.variableModParameters.cModCode[0];
            else if (szParamName[12]=='2')
               cSymbol = g_staticParams.variableModParameters.cModCode[1];
            else if (szParamName[12]=='3')
               cSymbol = g_staticParams.variableModParameters.cModCode[2];
            else if (szParamName[12]=='4')
               cSymbol = g_staticParams.variableModParameters.cModCode[3];
            else if (szParamName[12]=='5')
               cSymbol = g_staticParams.variableModParameters.cModCode[4];
            else if (szParamName[12]=='6')
               cSymbol = g_staticParams.variableModParameters.cModCode[5];

            if (cSymbol != '-' && dMass!=0.0)
            {
               int i;
               for (i=0; i<(int)strlen(szModChar); i++)
               {
                  fprintf(fpout, "  <aminoacid_modification aminoacid=\"%c\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"Y\" %ssymbol=\"%c\"/>\n",
                        szModChar[i],
                        dMass,
                        g_staticParams.massUtility.pdAAMassParent[(int)szModChar[i]] + dMass,
                        (bBinary?"binary=\"Y\" ":""),
                        cSymbol);
               }
            }
         }

         else if (!strcmp(szParamName, "variable_C_terminus"))
         {
            if (g_staticParams.variableModParameters.dVarModMassC != 0.0)
            {
               sprintf(szVariableCterm, "  <terminal_modification terminus=\"C\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"Y\" protein_terminus=\"",
                     g_staticParams.variableModParameters.dVarModMassC,
                     g_staticParams.variableModParameters.dVarModMassC + g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS);
               bPrintVariableC = 1;
            }
         }

         else if (!strcmp(szParamName, "variable_N_terminus"))
         {
            if (g_staticParams.variableModParameters.dVarModMassN != 0.0)
            {
               sprintf(szVariableNterm, "  <terminal_modification terminus=\"N\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"Y\" protein_terminus=\"",
                     g_staticParams.variableModParameters.dVarModMassN,
                     g_staticParams.variableModParameters.dVarModMassN + g_staticParams.precalcMasses.dNtermProton - PROTON_MASS
                     + g_staticParams.massUtility.pdAAMassFragment['h']);
               bPrintVariableN = 1;
            }
         }

         else if (!strcmp(szParamName, "variable_N_terminus_distance"))
         {
            int iDistance;
            sscanf(szParamVal, "%d", &iDistance);
            if (iDistance == 0)
               bProteinTerminusN = true;
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
                     dMass, g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS);
            }
         }
         else if (!strcmp(szParamName, "add_Nterm_peptide"))
         {
            sscanf(szParamVal, "%lf", &dMass);
            if (dMass != 0.)
            {
               fprintf(fpout, "  <terminal_modification terminus=\"N\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"N\" protein_terminus=\"N\"/>\n",
                     dMass, g_staticParams.precalcMasses.dNtermProton - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment['h']);
            }
         }
         else if (!strcmp(szParamName, "add_Cterm_protein"))
         {
            sscanf(szParamVal, "%lf", &dMass);
            if (dMass != 0.0)
            {
               fprintf(fpout, "  <terminal_modification terminus=\"C\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"N\" protein_terminus=\"Y\"/>\n",
                     dMass, dMass + g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS);
            }
         }
         else if (!strcmp(szParamName, "add_Nterm_protein"))
         {
            sscanf(szParamVal, "%lf", &dMass);
            if (dMass != 0.0)
            {
               fprintf(fpout, "  <terminal_modification terminus=\"N\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"N\" protein_terminus=\"Y\"/>\n",
                     dMass, dMass + g_staticParams.precalcMasses.dNtermProton - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment['h']); //FIX??
            }
         }

         else if (!strncmp(szParamName, "add_", 4) && szParamName[5]=='_')  // add_?_
         {
            sscanf(szParamVal, "%lf", &dMass);
            if (dMass != 0.0)
            {
               fprintf(fpout, "  <aminoacid_modification aminoacid=\"%c\" massdiff=\"%0.4f\" mass=\"%0.4f\" variable=\"N\"/>\n",
                     szParamName[4], dMass, g_staticParams.massUtility.pdAAMassParent[(int)szParamName[4]]);
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
      if ((pStr = strchr(szParamBuf, '#')))  // remove comments
         *pStr='\0';
      if (strchr(szParamBuf, '='))
      {
         char szParamName[128];
         char szParamVal[128];

         sscanf(szParamBuf, "%128s", szParamName);
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

void CometWritePepXML::WritePepXMLEndTags(FILE *fpout)
{
   fprintf(fpout, " </msms_run_summary>\n");
   fprintf(fpout, "</msms_pipeline_analysis>\n");
   fflush(fpout);
}

void CometWritePepXML::PrintResults(int iWhichQuery,
                                    bool bDecoy,
                                    FILE *fpout,
                                    char *szOutput)
{
   int  i,
        iDoXcorrCount,
        iRankXcorr,
        iMinLength;
   char *pStr;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

#ifdef _WIN32
   if ( (pStr = strrchr(g_staticParams.inputFile.szBaseName, '\\')) == NULL)
#else
   if ( (pStr = strrchr(g_staticParams.inputFile.szBaseName, '/')) == NULL)
#endif
      pStr = g_staticParams.inputFile.szBaseName;
   else
      pStr++;  // skip separation character

   // Print spectrum_query element. 
   fprintf(fpout, " <spectrum_query spectrum=\"%s.%05d.%05d.%d\"",
         pStr,
         pQuery->_spectrumInfoInternal.iScanNumber,
         pQuery->_spectrumInfoInternal.iScanNumber,
         pQuery->_spectrumInfoInternal.iChargeState);
   fprintf(fpout, " start_scan=\"%d\"", pQuery->_spectrumInfoInternal.iScanNumber);
   fprintf(fpout, " end_scan=\"%d\"", pQuery->_spectrumInfoInternal.iScanNumber);
   fprintf(fpout, " precursor_neutral_mass=\"%0.4f\"", pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS);
   fprintf(fpout, " assumed_charge=\"%d\"", pQuery->_spectrumInfoInternal.iChargeState);
   fprintf(fpout, " index=\"%d\"", iWhichQuery+1);

   if (mzXML)
      fprintf(fpout, " retention_time_sec=\"%0.1f\">\n", pQuery->_spectrumInfoInternal.dRTime);
   else
      fprintf(fpout, ">\n");

   fprintf(fpout, "  <search_result>\n");

   if (bDecoy)
      iDoXcorrCount = pQuery->iDoDecoyXcorrCount;
   else
      iDoXcorrCount = pQuery->iDoXcorrCount;

   // Print out each sequence line.
   if (iDoXcorrCount > (g_staticParams.options.iNumPeptideOutputLines))
      iDoXcorrCount = (g_staticParams.options.iNumPeptideOutputLines);

   Results *pOutput;

   if (bDecoy)
      pOutput = pQuery->_pDecoys;
   else
      pOutput = pQuery->_pResults;

   iRankXcorr = 1;

   iMinLength = 999;
   for (i=0; i<iDoXcorrCount; i++)
   {
      int iLen = (int)strlen(pOutput[i].szPeptide);
      if (iLen == 0)
         break;
      if (iLen < iMinLength)
         iMinLength = iLen;
   }

   for (i=0; i<iDoXcorrCount; i++)
   {
      int j;
      bool bNoDeltaCnYet = true;
      bool bDeltaCnStar = false;
      double dDeltaCn;       // this is deltaCn between i and first dissimilar peptide
      double dDeltaCnStar;   // this is explicit deltaCn between i and i+1 hits or 0.0 ...
                             // I honestly don't understand the logic in the deltacnstar convention being used in TPP

      for (j=i+1; j<iDoXcorrCount; j++)
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
            dDeltaCn = (pOutput[i].fXcorr - pOutput[j].fXcorr) / pOutput[i].fXcorr;
            bNoDeltaCnYet = 0;
   
            if (j - i > 1)
               bDeltaCnStar = true;
            break;
         }
      }

      if (bNoDeltaCnYet)
         dDeltaCn = 1.0;

      if (i > 0 && pOutput[i].fXcorr != pOutput[i-1].fXcorr)
         iRankXcorr++;

      if (pOutput[i].fXcorr > 0.0)
      {
         if (bDeltaCnStar && i+1<iDoXcorrCount)
            dDeltaCnStar = (pOutput[i].fXcorr - pOutput[i+1].fXcorr)/pOutput[i].fXcorr;
         else
            dDeltaCnStar = 0.0;

         PrintPepXMLSearchHit(iWhichQuery, iRankXcorr, i, iDoXcorrCount, bDecoy, pOutput, fpout, dDeltaCn, dDeltaCnStar);
      }
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
   fprintf(fpout, " calc_neutral_pep_mass=\"%0.4f\"", pOutput[iWhichResult].dPepMass - PROTON_MASS);
   fprintf(fpout, " massdiff=\"%0.4f\"", pQuery->_pepMassInfo.dExpPepMass - pOutput[iWhichResult].dPepMass);

   CalcNTTNMC(pOutput, iWhichResult, &iNTT, &iNMC);

   fprintf(fpout, " num_tol_term=\"%d\"", iNTT);
   fprintf(fpout, " num_missed_cleavages=\"%d\"", iNMC); 
   fprintf(fpout, " num_matched_peptides=\"%lu\"",
         bDecoy?(pQuery->_liNumMatchedDecoyPeptides):(pQuery->_liNumMatchedPeptides));
   fprintf(fpout, ">\n");

   // check if peptide is modified
   bool bModified = 0;

   if (g_staticParams.staticModifications.dAddNterminusPeptide != 0.0
         || g_staticParams.staticModifications.dAddCterminusPeptide != 0.0)
      bModified = 1;

   if (pOutput[iWhichResult].szPrevNextAA[0]=='-' && g_staticParams.staticModifications.dAddNterminusProtein != 0.0)
      bModified = 1;
   if (pOutput[iWhichResult].szPrevNextAA[1]=='-' && g_staticParams.staticModifications.dAddCterminusProtein != 0.0)
      bModified = 1;

   if (!bModified)
   {
      for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
      {
         if (g_staticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]] != 0.0
               || pOutput[iWhichResult].pcVarModSites[i] > 0)
         {
            bModified = 1;
            break;
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
               g_staticParams.variableModParameters.dVarModMassN
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
              g_staticParams.variableModParameters.dVarModMassC
              + g_staticParams.precalcMasses.dCtermOH2Proton);
      }

      fprintf(fpout, "    <modification_info modified_peptide=\"%s\"", szModPep);

      if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] == 1
            || g_staticParams.staticModifications.dAddNterminusPeptide != 0.0
            || (pOutput[iWhichResult].szPrevNextAA[0]=='-' && g_staticParams.staticModifications.dAddNterminusProtein != 0.0) )
      {
         // static peptide n-term mod already accounted for here
         double dMass = g_staticParams.precalcMasses.dNtermProton - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment['h'];

         if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide] == 1)
            dMass += g_staticParams.variableModParameters.dVarModMassN;

         if (pOutput[iWhichResult].szPrevNextAA[0]=='-' && g_staticParams.staticModifications.dAddNterminusProtein != 0.0)
            dMass += g_staticParams.staticModifications.dAddNterminusProtein;

         fprintf(fpout, " mod_nterm_mass=\"%0.4f\"", dMass);
      }

      if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] == 1
            || g_staticParams.staticModifications.dAddCterminusPeptide != 0.0
            || (pOutput[iWhichResult].szPrevNextAA[1]=='-' && g_staticParams.staticModifications.dAddCterminusProtein != 0.0) )
      {
         // static peptide c-term mod already accounted for here
         double dMass = g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS;

         if (pOutput[iWhichResult].pcVarModSites[pOutput[iWhichResult].iLenPeptide+1] == 1)
            dMass += g_staticParams.variableModParameters.dVarModMassC;

         if (pOutput[iWhichResult].szPrevNextAA[1]=='-' && g_staticParams.staticModifications.dAddCterminusProtein != 0.0)
            dMass += g_staticParams.staticModifications.dAddCterminusProtein;

         fprintf(fpout, " mod_cterm_mass=\"%0.4f\"", dMass);
      }
      fprintf(fpout, ">\n");

      for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
      {
         if (g_staticParams.staticModifications.pdStaticMods[(int)pOutput[iWhichResult].szPeptide[i]] != 0.0
               || pOutput[iWhichResult].pcVarModSites[i] > 0)
         {
            double dMass = g_staticParams.massUtility.pdAAMassFragment[(int)pOutput[iWhichResult].szPeptide[i]];

            if (pOutput[iWhichResult].pcVarModSites[i] > 0)
               dMass += g_staticParams.variableModParameters.varModList[pOutput[iWhichResult].pcVarModSites[i]-1].dVarModMass;

            fprintf(fpout, "     <mod_aminoacid_mass position=\"%d\" mass=\"%0.4f\"/>\n", i+1, dMass);
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
