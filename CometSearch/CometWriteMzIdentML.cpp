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
#include "CometWriteMzIdentML.h"
#include "CometSearchManager.h"
#include "CometStatus.h"
#include "CometWritePepXML.h"

#include "limits.h"
#include "stdlib.h"


CometWriteMzIdentML::CometWriteMzIdentML()
{
}


CometWriteMzIdentML::~CometWriteMzIdentML()
{
}


void CometWriteMzIdentML::WriteMzIdentMLTmp(FILE *fpout,
                                            FILE *fpoutd,
                                            int iBatchNum)
{
   int i;

   // Print temporary results in tab-delimited file
   if (g_staticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); ++i)
         PrintTmpPSM(i, 1, iBatchNum, fpout);
      for (i=0; i<(int)g_pvQuery.size(); ++i)
         PrintTmpPSM(i, 2, iBatchNum, fpoutd);
   }
   else
   {
      for (i=0; i<(int)g_pvQuery.size(); ++i)
         PrintTmpPSM(i, 0, iBatchNum, fpout);
   }

   fflush(fpout);
}


void CometWriteMzIdentML::WriteMzIdentML(FILE *fpout,
                                         FILE *fpdb,
                                         char *szTmpFile,
                                         CometSearchManager &searchMgr)
{
   WriteMzIdentMLHeader(fpout);

   // now loop through szTmpFile file, wr
   ParseTmpFile(fpout, fpdb, szTmpFile, searchMgr);

   fprintf(fpout, "</MzIdentML>\n");
}


bool CometWriteMzIdentML::WriteMzIdentMLHeader(FILE *fpout)
{
   time_t tTime;
   char szDate[48];
   char szManufacturer[SIZE_FILE];
   char szModel[SIZE_FILE];

   time(&tTime);
   strftime(szDate, 46, "%Y-%m-%dT%H:%M:%S", localtime(&tTime));

   // Get msModel + msManufacturer from mzXML. Easy way to get from mzML too?
   CometWritePepXML::ReadInstrument(szManufacturer, szModel);

   // Write out pepXML header.
   fprintf(fpout, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

   fprintf(fpout, "<MzIdentML id=\"Comet %s\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"https://psidev.info/mzidentml#mzid12 https://github.com/HUPO-PSI/mzIdentML/blob/master/schema/mzIdentML1.2.0.xsd\" xmlns=\"http://psidev.info/psi/pi/mzIdentML/1.2\" version=\"1.2.0\" creationDate=\"%s\">\n", g_sCometVersion.c_str(), szDate);
   fprintf(fpout, " <cvList>\n");
   fprintf(fpout, "  <cv id=\"PSI-MS\" uri=\"https://raw.githubusercontent.com/HUPO-PSI/psi-ms-CV/master/psi-ms.obo\" fullName=\"PSI-MS\" />\n");
   fprintf(fpout, "  <cv id=\"UNIMOD\" uri=\"http://www.unimod.org/obo/unimod.obo\" fullName=\"UNIMOD\" />\n");
   fprintf(fpout, "  <cv id=\"UO\" uri=\"https://raw.githubusercontent.com/bio-ontology-research-group/unit-ontology/master/unit.obo\" fullName=\"UNIT-ONTOLOGY\" />\n");
   fprintf(fpout, "  <cv id=\"PRIDE\" uri=\"https://github.com/PRIDE-Utilities/pride-ontology/blob/master/pride_cv.obo\" fullName=\"PRIDE\" />\n");
   fprintf(fpout, " </cvList>\n");


   fprintf(fpout, " <AnalysisSoftwareList>\n");
   fprintf(fpout, "  <AnalysisSoftware id=\"Comet\" name=\"Comet\" version=\"%s\">\n", g_sCometVersion.c_str());
   fprintf(fpout, "   <SoftwareName><cvParam cvRef=\"PSI-MS\" accession=\"MS:1002251\" name=\"Comet\" value=\"\" /></SoftwareName>\n");
   fprintf(fpout, "  </AnalysisSoftware>\n");
   fprintf(fpout, " </AnalysisSoftwareList>\n");

   fflush(fpout);

   return true;
}


bool CometWriteMzIdentML::ParseTmpFile(FILE *fpout,
                                       FILE *fpdb,
                                       char *szTmpFile,
                                       CometSearchManager &searchMgr)
{
   std::vector<MzidTmpStruct> vMzidTmp; // vector to store entire tmp output
   std::vector<long> vProteinTargets;   // store vector of target protein file offsets
   std::vector<long> vProteinDecoys;    // store vector of decoy protein file offsets
   std::vector<string> vstrPeptides;          // vector of peptides of format "QITQMSNSSDLADGLNFDEGDELLK;2:79.9969;4:15.9949;"
   std::vector<string> vstrPeptideEvidence;   // vector of peptides + target&decoy prots, space delimited "QITQMSNSSDLADGLNFDEGDELLK 1;38;75;112; 149;185;221;257;"

   fprintf(fpout, " <SequenceCollection xmlns=\"http://psidev.info/psi/pi/mzIdentML/1.2\">\n");

   // get all protein file positions by parsing through fpout_tmp
   // column 15 is target proteins, column 16 is decoy proteins

   std::ifstream ifsTmpFile(szTmpFile);

   if (!ifsTmpFile.is_open())
   {
      printf(" Error cannot read tmp file \"%s\"\n", szTmpFile);
      exit(1);
   }

   std::string strLine;  // line
   std::string strTmpPep;
   std::string strLocal;

   while (std::getline(ifsTmpFile, strLine))
   {
      struct MzidTmpStruct Stmp;

      std::string field;
      std::istringstream isString(strLine);

      int iWhichField = 0;
      while ( std::getline(isString, field, '\t') )
      {
         switch(iWhichField)
         {
            case 0:
               Stmp.iScanNumber = std::stoi(field);
               break;
            case 1:
               Stmp.iBatchNum = std::stoi(field);
               break;
            case 2:
               Stmp.iXcorrRank = std::stoi(field);
               break;
            case 3:
               Stmp.iCharge = std::stoi(field);
               break;
            case 4:
               Stmp.dExpMass= std::stod(field);
               break;
            case 5:
               Stmp.dCalcMass = std::stod(field);
               break;
            case 6:
               Stmp.dExpect = std::stod(field);
               break;
            case 7:
               Stmp.fXcorr = std::stof(field);
               break;
            case 8:
               Stmp.fCn = std::stof(field);
               break;
            case 9:
               Stmp.fSp = std::stof(field);
               break;
            case 10:
               Stmp.iRankSp = std::stoi(field);
               break;
            case 11:
               Stmp.iMatchedIons = std::stoi(field);
               break;
            case 12:
               Stmp.iTotalIons = std::stoi(field);
               break;
            case 13:
               Stmp.strPeptide = field;
               break;
            case 14:
               Stmp.cPrevNext[0] = field.at(0);
               Stmp.cPrevNext[1] = field.at(1);
               break;
            case 15:
               Stmp.strMods = field;
               break;
            case 16:
               Stmp.strProtsTarget = field;
               break;
            case 17:
               Stmp.strProtsDecoy = field;
               break;
            case 18:
               Stmp.iWhichQuery = std::stoi(field);
               break;
            case 19:
               Stmp.iWhichResult = std::stoi(field);
               break;
            case 20:
               Stmp.dRTime = std::stod(field);
               break;
            default:
               char szErrorMsg[SIZE_ERROR];
               sprintf(szErrorMsg,  " Error parsing mzid temp file (%d): %s\n", iWhichField, strLine.c_str());
               string strErrorMsg(szErrorMsg);
               g_cometStatus.SetStatus(CometResult_Failed, strErrorMsg);
               logerr(szErrorMsg);
               ifsTmpFile.close();
               return false;
         }
         iWhichField++;
      }

      vMzidTmp.push_back(Stmp);

      string strProteinOffset;

      // first grab all of the target protein offsets
      if (Stmp.strProtsTarget.length() > 0)
      {
         std::istringstream isString(Stmp.strProtsTarget);

         while ( std::getline(isString, strLocal, ';') )  // strLocal contains "offset:iStartResidue" pair
         {
            std::istringstream isString2(strLocal);
            std::getline(isString2, strProteinOffset, ':'); // get the offset before colon
            vProteinTargets.push_back(atol(strProteinOffset.c_str()));
         }
      }

      if (Stmp.strProtsDecoy.length() > 0)
      {
         std::istringstream isString(Stmp.strProtsDecoy);

         while ( std::getline(isString, strLocal, ';') )
         {
            std::istringstream isString2(strLocal);
            std::getline(isString2, strProteinOffset, ':'); // get the offset before colon
            vProteinDecoys.push_back(atol(strLocal.c_str()));
         }
      }

      // vstrPeptides contains "peptide;mods" string
      strTmpPep = Stmp.strPeptide;
      strTmpPep.append(";");
      strTmpPep.append(Stmp.strMods);
      vstrPeptides.push_back(strTmpPep);

      // vstrPeptideEvidence contains "peptide mods delimtedtargetprots delimiteddecoyprots"
      strTmpPep = Stmp.strPeptide;
      strTmpPep.append(" ");
      strTmpPep.append(Stmp.strMods);
      strTmpPep.append(" ");
      strTmpPep.append(Stmp.strProtsTarget);
      strTmpPep.append(" ");
      strTmpPep.append(Stmp.strProtsDecoy);
      vstrPeptideEvidence.push_back(strTmpPep);
   }

   ifsTmpFile.close();

   // now generate unique lists of file offsets and peptides
   std::sort(vProteinTargets.begin(), vProteinTargets.end());
   vProteinTargets.erase(std::unique(vProteinTargets.begin(), vProteinTargets.end()), vProteinTargets.end());

   std::sort(vProteinDecoys.begin(), vProteinDecoys.end());
   vProteinDecoys.erase(std::unique(vProteinDecoys.begin(), vProteinDecoys.end()), vProteinDecoys.end());

   std::sort(vstrPeptides.begin(), vstrPeptides.end());
   vstrPeptides.erase(std::unique(vstrPeptides.begin(), vstrPeptides.end()), vstrPeptides.end());

   std::sort(vstrPeptideEvidence.begin(), vstrPeptideEvidence.end());
   vstrPeptideEvidence.erase(std::unique(vstrPeptideEvidence.begin(), vstrPeptideEvidence.end()), vstrPeptideEvidence.end());

   // print DBSequence element
   std::vector<long>::iterator it;
   char szProteinName[512];
   string strProteinName;
   string strProteinSeq;

   bool bPrintSequences = false;
   if (g_staticParams.options.bOutputMzIdentMLFile == 2) // print sequences in DBSequence
   {
      if (g_staticParams.iIndexDb)
         bPrintSequences = false;
      else
         bPrintSequences = true;
   }

   for (it = vProteinTargets.begin(); it != vProteinTargets.end(); ++it)
   {
      if (*it >= 0)
      {
         CometMassSpecUtils::GetProteinName(fpdb, *it, szProteinName);
         strProteinName = szProteinName;
         CometMassSpecUtils::EscapeString(strProteinName);
         fprintf(fpout, "  <DBSequence id=\"%s\" accession=\"%s\" searchDatabase_ref=\"DB\"", strProteinName.c_str(), strProteinName.c_str());

         if (bPrintSequences)
         {
            CometMassSpecUtils::GetProteinSequence(fpdb, *it, strProteinSeq);
            if (strProteinSeq.size() > 0)
            {
               fprintf(fpout, ">\n");
               fprintf(fpout, "   <Seq>%s</Seq>\n", strProteinSeq.c_str());
               fprintf(fpout, "   <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001344\" name=\"AA sequence\" />\n");
               fprintf(fpout, "  </DBSequence>\n");
            }
            else
               fprintf(fpout, " />\n");
         }
         else
            fprintf(fpout, " />\n");

      }
   }
   for (it = vProteinDecoys.begin(); it != vProteinDecoys.end(); ++it)
   {
      if (*it >= 0)
      {
         CometMassSpecUtils::GetProteinName(fpdb, *it, szProteinName);
         strProteinName = szProteinName;
         CometMassSpecUtils::EscapeString(strProteinName);
         fprintf(fpout, "  <DBSequence id=\"%s%s\" accession=\"%s%s\" searchDatabase_ref=\"DB\" />\n",
               g_staticParams.sDecoyPrefix.c_str(), strProteinName.c_str(), g_staticParams.sDecoyPrefix.c_str(), strProteinName.c_str());
      }
   }

   vProteinTargets.clear();
   vProteinDecoys.clear();

   // print Peptide element
   std::vector<string>::iterator it2;
   int iLen;
   string strModID;
   string strModRef;
   string strModName;
   string strTmpPeptide;
   for (it2 = vstrPeptides.begin(); it2 != vstrPeptides.end(); ++it2)
   {
      std::istringstream isString(*it2);

      fprintf(fpout, "  <Peptide id=\"%s\">\n", (*it2).c_str());  // Note: id is "peptide;mod-string"

      std::getline(isString, strLocal, ';');
      strTmpPeptide = strLocal;
      fprintf(fpout, "   <PeptideSequence>%s</PeptideSequence>\n", strTmpPeptide.c_str());
      iLen = (int)strLocal.length();

      while ( std::getline(isString, strLocal, ';') )
      {
         if (strLocal.size() > 0)
         {
            int iPosition = 0;
            double dMass = 0;
            char cResidue;

            sscanf(strLocal.c_str(), "%d:%lf", &iPosition, &dMass);

            if (iPosition == iLen)  // n-term
            {
               iPosition = 0;
               cResidue = 'n';
            }
            else if (iPosition == iLen+1)  // c-term
            {
               iPosition = iLen;
               cResidue = 'c';
            }
            else
            {
               iPosition += 1;
               cResidue = strTmpPeptide.at(iPosition-1);
            }

            fprintf(fpout, "   <Modification location=\"%d\" monoisotopicMassDelta=\"%f\">\n", iPosition, dMass);
         
            GetModificationID(cResidue, dMass, &strModID, &strModRef, &strModName);
            fprintf(fpout, "   <cvParam cvRef=\"%s\" accession=\"%s\" name=\"%s\" />\n",
                  strModRef.c_str(), strModID.c_str(), strModName.c_str());

            fprintf(fpout, "   </Modification>\n");
         }
      }

      fprintf(fpout, "  </Peptide>\n");
   }

   // Now write PeptideEvidence to map every peptide to every protein sequence.
   // Need unique set of peptide+mods and proteins
   for (it2 = vstrPeptideEvidence.begin(); it2 != vstrPeptideEvidence.end(); ++it2)
   {
      string strPeptide;
      string strMods;
      string strTargets;
      string strDecoys;

      std::istringstream isString(*it2);

      bool bDecoy;
      int iLenDecoyPrefix = (int)strlen(g_staticParams.szDecoyPrefix);

      int n=0;
      while ( std::getline(isString, strLocal, ' ') )
      {
         switch (n)
         {
            case 0:
               strPeptide = strLocal;
               break;
            case 1:
               strMods = strLocal;
               break;
            case 2:
               if (strLocal.length() > 0)
               {
                  std::istringstream isTargets(strLocal);
                  std::string strOffset;  // contains "offset:iStartResidue;" pair
                  std::string strOffset2; // parse out just the "offset"
                  int iStartResidue;
                  int iEndResidue;
                  long lOffset;

                  // Now parse out individual target entries (file offsets) delimited by ";"
                  while ( std::getline(isTargets, strOffset, ';') )
                  {
                     std::istringstream isTargets2(strOffset);    // first get offset in "offset:iStartResidue;" pair
                     std::getline(isTargets2, strOffset2, ':');
                     lOffset = stol(strOffset2);

                     if (lOffset >= 0)
                     {
                        std::getline(isTargets2, strOffset2, ':');
                        iStartResidue = stoi(strOffset2);
                        iEndResidue = iStartResidue + (int)strPeptide.length() - 1;

                        CometMassSpecUtils::GetProteinName(fpdb, lOffset, szProteinName);
                        strProteinName = szProteinName;
                        CometMassSpecUtils::EscapeString(strProteinName);

                        bDecoy = true;
                        if (strncmp(strProteinName.c_str(), g_staticParams.szDecoyPrefix, iLenDecoyPrefix))
                           bDecoy = false;

                        fprintf(fpout, "  <PeptideEvidence start=\"%d\" end=\"%d\" id=\"%s;%s;%s\" isDecoy=\"%s\" peptide_ref=\"%s;%s\" dBSequence_ref=\"%s\" />\n",
                              iStartResidue,
                              iEndResidue,
                              strPeptide.c_str(),
                              strMods.c_str(),
                              strProteinName.c_str(),
                              (bDecoy?"true":"false"),  // for regular search, the check if entry is user-supplied decoy
                              strPeptide.c_str(),
                              strMods.c_str(),
                              strProteinName.c_str());
                     }
                  }
               }

               break;
            case 3:
               if (strLocal.length() > 0)
               {
                  std::istringstream isDecoys(strLocal);
                  std::string strOffset;
                  std::string strOffset2;
                  int iStartResidue;
                  int iEndResidue;
                  long lOffset;

                  // Now parse out individual decoy entries (file offsets) delimited by ";"
                  while ( std::getline(isDecoys, strOffset, ';') )
                  {
                     std::istringstream isDecoys2(strOffset);
                     std::getline(isDecoys2, strOffset2, ':');
                     lOffset = stol(strOffset2);
                     
                     if (lOffset >= 0)
                     {
                        std::getline(isDecoys2, strOffset2, ':');
                        iStartResidue = stoi(strOffset2);
                        iEndResidue = iStartResidue + (int)strPeptide.length() - 1;

                        CometMassSpecUtils::GetProteinName(fpdb, lOffset, szProteinName);
                        strProteinName = szProteinName;
                        CometMassSpecUtils::EscapeString(strProteinName);

                        fprintf(fpout, "  <PeptideEvidence start=\"%d\" end=\"%d\" id=\"%s;%s;%s%s\" isDecoy=\"true\" peptide_ref=\"%s;%s\" dBSequence_ref=\"%s%s\" />\n",
                              iStartResidue,
                              iEndResidue,
                              strPeptide.c_str(),
                              strMods.c_str(),
                              g_staticParams.sDecoyPrefix.c_str(),
                              strProteinName.c_str(),
                              strPeptide.c_str(),  //FIX is this right??
                              strMods.c_str(),
                              g_staticParams.sDecoyPrefix.c_str(),
                              strProteinName.c_str());
                     }
                  }
               }
               break;
         }

         n++;
      }
   }

   fprintf(fpout, " </SequenceCollection>\n");

   fprintf(fpout, " <AnalysisCollection>\n");
   fprintf(fpout, "  <SpectrumIdentification spectrumIdentificationList_ref=\"SIL\" spectrumIdentificationProtocol_ref=\"SIP\" id=\"SI\">\n");
   fprintf(fpout, "   <InputSpectra spectraData_ref=\"SD\" />\n");
   fprintf(fpout, "   <SearchDatabaseRef searchDatabase_ref=\"DB\" />\n");
   fprintf(fpout, "  </SpectrumIdentification>\n");
   fprintf(fpout, " </AnalysisCollection>\n");

   fprintf(fpout, " <AnalysisProtocolCollection>\n");
   fprintf(fpout, "  <SpectrumIdentificationProtocol analysisSoftware_ref=\"Comet\" id=\"SIP\">\n");
   fprintf(fpout, "   <SearchType>\n");
   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001083\" name=\"ms-ms search\" />\n");
   fprintf(fpout, "   </SearchType>\n");

   fprintf(fpout, "   <AdditionalSearchParams>\n");
   if (g_staticParams.massUtility.bMonoMassesParent)
      fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001211\" name=\"parent mass type mono\" />\n");
   else
      fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001212\" name=\"parent mass type average\" />\n");

   if (g_staticParams.massUtility.bMonoMassesFragment)
      fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001256\" name=\"fragment mass type mono\" />\n");
   else
      fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001255\" name=\"fragment mass type average\" />\n");
   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002495\" name=\"no special processing\" />\n");

   // write out all Comet parameters
   std::map<std::string, CometParam*> mapParams = searchMgr.GetParamsMap();
   for (std::map<std::string, CometParam*>::iterator it=mapParams.begin(); it!=mapParams.end(); ++it)
   {
      if (it->first != "[COMET_ENZYME_INFO]")
      {
         fprintf(fpout, "    <userParam name=\"%s\" value=\"%s\" />\n", it->first.c_str(), it->second->GetStringValue().c_str());
      }
   }
   fprintf(fpout, "   </AdditionalSearchParams>\n");

   WriteMods(fpout, searchMgr);
   WriteEnzyme(fpout);
   WriteMassTable(fpout);
   WriteTolerance(fpout);

   fprintf(fpout, "  </SpectrumIdentificationProtocol>\n");
   fprintf(fpout, " </AnalysisProtocolCollection>\n");

   fprintf(fpout, " <DataCollection>\n");

   WriteInputs(fpout);

   fprintf(fpout, "  <AnalysisData>\n");

   WriteSpectrumIdentificationList(fpout, fpdb, &vMzidTmp);

   fprintf(fpout, "  </AnalysisData>\n");
   fprintf(fpout, " </DataCollection>\n");

   return true;
}


void CometWriteMzIdentML::WriteMods(FILE *fpout,
                                    CometSearchManager &searchMgr)
{
   string strModID;
   string strModRef;
   string strModName;

   fprintf(fpout, "   <ModificationParams>\n");

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

   double dMass = 0.0;
   if (searchMgr.GetParamValue("add_Cterm_peptide", dMass))
   {
      if (!isEqual(dMass, 0.0))
      {
         double dMassDiff = g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS - g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

         fprintf(fpout, "  <SearchModification fixedMod=\"true\" massDelta=\"%0.6f\" residues=\".\">\n", dMassDiff);
         fprintf(fpout, "   <SpecificityRules>\n");
         fprintf(fpout, "     <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001190\" name=\"modification specificity peptide C-term\" />\n");
         fprintf(fpout, "   </SpecificityRules>\n");

         GetModificationID('c', dMassDiff, &strModID, &strModRef, &strModName);
         fprintf(fpout, "   <cvParam cvRef=\"%s\" accession=\"%s\" name=\"%s\" />\n",
               strModRef.c_str(), strModID.c_str(), strModName.c_str());

         fprintf(fpout, "  </SearchModification>\n");
      }
   }

   dMass = 0.0;
   if (searchMgr.GetParamValue("add_Nterm_peptide", dMass))
   {
      if (!isEqual(dMass, 0.0))
      {
         double dMassDiff = g_staticParams.precalcMasses.dNtermProton - PROTON_MASS - g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

         fprintf(fpout, "  <SearchModification fixedMod=\"true\" massDelta=\"%0.6f\" residues=\".\">\n", dMassDiff);
         fprintf(fpout, "   <SpecificityRules>\n");
         fprintf(fpout, "     <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001189\" name=\"modification specificity peptide N-term\" />\n");
         fprintf(fpout, "   </SpecificityRules>\n");

         GetModificationID('n', dMassDiff, &strModID, &strModRef, &strModName);
         fprintf(fpout, "   <cvParam cvRef=\"%s\" accession=\"%s\" name=\"%s\" />\n",
               strModRef.c_str(), strModID.c_str(), strModName.c_str());

         fprintf(fpout, "  </SearchModification>\n");
      }
   }

   dMass = 0.0;
   if (searchMgr.GetParamValue("add_Cterm_protein", dMass))
   {
      if (!isEqual(dMass, 0.0))
      {
         double dMassDiff = g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS - g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

         fprintf(fpout, "  <SearchModification fixedMod=\"true\" massDelta=\"%0.6f\" residues=\".\">\n", dMassDiff);
         fprintf(fpout, "   <SpecificityRules>\n");
         fprintf(fpout, "     <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002058\" name=\"modification specificity protein C-term\" />\n");
         fprintf(fpout, "   </SpecificityRules>\n");

         GetModificationID('c', dMassDiff, &strModID, &strModRef, &strModName);
         fprintf(fpout, "   <cvParam cvRef=\"%s\" accession=\"%s\" name=\"%s\" />\n",
               strModRef.c_str(), strModID.c_str(), strModName.c_str());

         fprintf(fpout, "  </SearchModification>\n");
      }
   }

   if (!isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0))
   {
      fprintf(fpout, "  <SearchModification fixedMod=\"true\" massDelta=\"%0.6f\" residues=\".\">\n", g_staticParams.staticModifications.dAddNterminusProtein);
      fprintf(fpout, "   <SpecificityRules>\n");
      fprintf(fpout, "     <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002057\" name=\"modification specificity protein N-term\" />\n");
      fprintf(fpout, "   </SpecificityRules>\n");

      GetModificationID('n', g_staticParams.staticModifications.dAddNterminusProtein, &strModID, &strModRef, &strModName);
      fprintf(fpout, "   <cvParam cvRef=\"%s\" accession=\"%s\" name=\"%s\" />\n",
         strModRef.c_str(), strModID.c_str(), strModName.c_str());

      fprintf(fpout, "  </SearchModification>\n");
   }

   // Write out properly encoded mods
   for (int iWhichVariableMod = 0; iWhichVariableMod < VMODS; ++iWhichVariableMod)
   {
      WriteVariableMod(fpout, iWhichVariableMod, 0);
   }
   for (int iWhichVariableMod = 0; iWhichVariableMod < VMODS; ++iWhichVariableMod)
   {
      WriteVariableMod(fpout, iWhichVariableMod, 1);
   }

   fprintf(fpout, "   </ModificationParams>\n");
}


void CometWriteMzIdentML::WriteStaticMod(FILE *fpout,
                                         CometSearchManager &searchMgr,
                                         string paramName)
{
   double dMass = 0.0;
   if (searchMgr.GetParamValue(paramName, dMass))
   {
      if (!isEqual(dMass, 0.0))
      {
         fprintf(fpout, "    <SearchModification residues=\"%c\" massDelta=\"%0.6f\" fixedMod= \"true\" >\n",
               paramName[4], dMass);

         string strModID;
         string strModRef;
         string strModName; 

         GetModificationID(paramName[4], dMass, &strModID, &strModRef, &strModName);

         fprintf(fpout, "     <cvParam cvRef=\"%s\" accession=\"%s\" name=\"%s\" />\n",
               strModRef.c_str(), strModID.c_str(), strModName.c_str());
         fprintf(fpout, "    </SearchModification>\n");
      }
   }
}


void CometWriteMzIdentML::GetModificationID(char cResidue,
                                            double dModMass,
                                            string *strModID,
                                            string *strModRef,
                                            string *strModName)
{
   *strModID = "MS:1001460";
   *strModRef = "PSI-MS";
   *strModName = "unknown modification";

   if (fabs(dModMass - Oxygen_Mono) < 0.01 && strchr("DKNPFYRMCWHGUEILQSTV", cResidue))
   {
      *strModID = "UNIMOD:35";
      *strModRef = "UNIMOD";
      *strModName = "Oxidation";
   }
   else if (fabs(dModMass - 79.966331) < 0.01 && strchr("TSYDHCRK", cResidue))
   {
      *strModID = "UNIMOD:21";
      *strModRef = "UNIMOD";
      *strModName = "Phospho";
   }
   else if (fabs(dModMass - 42.010565) < 0.01 && strchr("nKCSTYHR", cResidue))
   {
      *strModID = "UNIMOD:1";
      *strModRef = "UNIMOD";
      *strModName = "Acetyl";
   }
   else if (fabs(dModMass - 226.077598) < 0.01 && strchr("nK", cResidue))
   {
      *strModID = "UNIMOD:3";
      *strModRef = "UNIMOD";
      *strModName = "Biotin";
   }
   else if (fabs(dModMass - 57.021464) < 0.01 && strchr("nCKHDESTYHM", cResidue))
   {
      *strModID = "UNIMOD:4";
      *strModRef = "UNIMOD";
      *strModName = "Carbamidomethyl";
   }
   else if (fabs(dModMass - 43.005814) < 0.01 && strchr("nKRCMSTY", cResidue))
   {
      *strModID = "UNIMOD:5";
      *strModRef = "UNIMOD";
      *strModName = "Carbamyl";
   }
   else if (fabs(dModMass - 58.005479) < 0.01 && strchr("nCKWU", cResidue))
   {
      *strModID = "UNIMOD:6";
      *strModRef = "UNIMOD";
      *strModName = "Carboxymethyl";
   }
   else if (fabs(dModMass - 0.984016) < 0.01 && strchr("QNRF", cResidue))
   {
      *strModID = "UNIMOD:7";
      *strModRef = "UNIMOD";
      *strModName = "Deamidated";
   }
   else if (fabs(dModMass - -18.010565) < 0.01)
   {
      if (cResidue == 'E')
      {
         *strModID = "UNIMOD:27";
         *strModRef = "UNIMOD";
         *strModName = "Glu->pyro-Glu";
      }
      else if (strchr("NQSTYDC", cResidue))
      {
         *strModID = "UNIMOD:23";
         *strModRef = "UNIMOD";
         *strModName = "Dehydrated";
      }
   }
   else if (fabs(dModMass - -17.026549) < 0.01)
   {
      if (cResidue == 'Q')
      {
         *strModID = "UNIMOD:28";
         *strModRef = "UNIMOD";
         *strModName = "Gln->pyro-Glu";
      }
      else if (strchr("TSCN", cResidue))
      {
         *strModID = "UNIMOD:385";
         *strModRef = "UNIMOD";
         *strModName = "Ammonia-loss";
      }
   }
   else if (fabs(dModMass - 14.01565) < 0.01 && strchr("ncCHKNQRILEDST", cResidue))
   {
      *strModID = "UNIMOD:34";
      *strModRef = "UNIMOD";
      *strModName = "Methyl";
   }
   else if (fabs(dModMass - 114.042927) < 0.01 && strchr("nKSTC", cResidue))
   {
      *strModID = "UNIMOD:121";
      *strModRef = "UNIMOD";
      *strModName = "GG";
   }
   else if (fabs(dModMass - 27.994915) < 0.01 && strchr("nKST", cResidue))
   {
      *strModID = "UNIMOD:122";
      *strModRef = "UNIMOD";
      *strModName = "Formyl";
   }
   else if (fabs(dModMass - 28.990164) < 0.01 && strchr("C", cResidue))
   {
      *strModID = "UNIMOD:275";
      *strModRef = "UNIMOD";
      *strModName = "Nitrosyl";
   }
   else if (fabs(dModMass - 229.162932) < 0.01 && strchr("nKHST", cResidue))
   {
      *strModID = "UNIMOD:737";
      *strModRef = "UNIMOD";
      *strModName = "TMT6plex";
   }
   else if (fabs(dModMass - 225.155833) < 0.01 && strchr("nKHST", cResidue))
   {
      *strModID = "UNIMOD:738";
      *strModRef = "UNIMOD";
      *strModName = "TMT2plex";
   }
   else if (fabs(dModMass - 224.152478) < 0.01 && strchr("nKHST", cResidue))
   {
      *strModID = "UNIMOD:739";
      *strModRef = "UNIMOD";
      *strModName = "TMT";
   }
   else if (fabs(dModMass - 304.207146) < 0.01 && strchr("nKHST", cResidue))
   {
      *strModID = "UNIMOD:2016";
      *strModRef = "UNIMOD";
      *strModName = "TMTpro";
   }
}
                                            

void CometWriteMzIdentML::WriteVariableMod(FILE *fpout,
                                           int iWhichVariableMod,
                                           bool bWriteTerminalMods)
{
   VarMods varModsParam = g_staticParams.variableModParameters.varModList[iWhichVariableMod];

   if (!isEqual(varModsParam.dVarModMass, 0.0))
   {
      int iLen = (int)strlen(varModsParam.szVarModChar);

      for (int i=0; i<iLen; ++i)
      {
         string strModID;
         string strModRef;
         string strModName;

         GetModificationID(varModsParam.szVarModChar[i], varModsParam.dVarModMass, &strModID, &strModRef, &strModName);

         if (varModsParam.szVarModChar[i]=='n' && bWriteTerminalMods)
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
                  fprintf(fpout, "    <SearchModification residues=\".\" massDelta=\"%0.6f\" fixedMod= \"false\" >\n",
                     varModsParam.dVarModMass + g_staticParams.staticModifications.dAddNterminusProtein);

                  fprintf(fpout, "     <SpecificityRules>\n");
                  fprintf(fpout, "       <cvParam accession=\"MS:1002057\" cvRef=\"PSI-MS\" name=\"modification specificity protein N-term\" />\n");
                  fprintf(fpout, "     </SpecificityRules>\n");

                  fprintf(fpout, "     <cvParam cvRef=\"%s\" accession=\"%s\" name=\"%s\" />\n",
                        strModRef.c_str(), strModID.c_str(), strModName.c_str());
                  fprintf(fpout, "    </SearchModification>\n");
               }
               // print this if non-protein N-term variable mod
               else
               {
                  fprintf(fpout, "    <SearchModification residues=\".\" massDelta=\"%0.6f\" fixedMod= \"false\" >\n", varModsParam.dVarModMass);

                  fprintf(fpout, "     <SpecificityRules>\n");
                  fprintf(fpout, "       <cvParam accession=\"MS:1001189\" cvRef=\"PSI-MS\" name=\"modification specificity peptide N-term\" />\n");
                  fprintf(fpout, "     </SpecificityRules>\n");

                  fprintf(fpout, "     <cvParam cvRef=\"%s\" accession=\"%s\" name=\"%s\" />\n",
                        strModRef.c_str(), strModID.c_str(), strModName.c_str());
                  fprintf(fpout, "    </SearchModification>\n");
               }
            }
         }
         else if (varModsParam.szVarModChar[i]=='c' && bWriteTerminalMods)
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
                  fprintf(fpout, "    <SearchModification residues=\".\" massDelta=\"%0.6f\" fixedMod= \"false\" >\n",
                     varModsParam.dVarModMass + g_staticParams.staticModifications.dAddCterminusProtein);

                  fprintf(fpout, "     <SpecificityRules>\n");
                  fprintf(fpout, "       <cvParam accession=\"MS:1002058\" cvRef=\"PSI-MS\" name=\"modification specificity protein C-term\" />\n");
                  fprintf(fpout, "     </SpecificityRules>\n");

                  fprintf(fpout, "     <cvParam cvRef=\"%s\" accession=\"%s\" name=\"%s\" />\n",
                        strModRef.c_str(), strModID.c_str(), strModName.c_str());
                  fprintf(fpout, "    </SearchModification>\n");
               }
               // print this if non-protein C-term variable mod
               else
               {
                  fprintf(fpout, "    <SearchModification residues=\".\" massDelta=\"%0.6f\" fixedMod= \"false\" >\n", varModsParam.dVarModMass);

                  fprintf(fpout, "     <SpecificityRules>\n");
                  fprintf(fpout, "       <cvParam accession=\"MS:1001190\" cvRef=\"PSI-MS\" name=\"modification specificity peptide C-term\" />\n");
                  fprintf(fpout, "     </SpecificityRules>\n");

                  fprintf(fpout, "     <cvParam cvRef=\"%s\" accession=\"%s\" name=\"%s\" />\n",
                        strModRef.c_str(), strModID.c_str(), strModName.c_str());
                  fprintf(fpout, "    </SearchModification>\n");
               }
            }
         }
         else if (!bWriteTerminalMods && varModsParam.szVarModChar[i]!='c' && varModsParam.szVarModChar[i]!='n')
         {
            fprintf(fpout, "    <SearchModification residues=\"%c\" massDelta=\"%0.6f\" fixedMod= \"false\" >\n",
                  varModsParam.szVarModChar[i], varModsParam.dVarModMass);

            fprintf(fpout, "     <cvParam cvRef=\"%s\" accession=\"%s\" name=\"%s\" />\n",
                  strModRef.c_str(), strModID.c_str(), strModName.c_str());
            fprintf(fpout, "    </SearchModification>\n");
         }
      }
   }
}


void CometWriteMzIdentML::WriteEnzyme(FILE *fpout)
{
   fprintf(fpout, "   <Enzymes>\n");

   char szSemi[8];
   char szRegExpression[128];
   if (g_staticParams.options.iEnzymeTermini == ENZYME_DOUBLE_TERMINI)
      strcpy(szSemi, "false");
   else
      strcpy(szSemi, "true");

   // regular expression examples
   // Trypsin (?<=[KR])(?!P)
   // Trypsin/P (?<=[KR])
   // Chymotrypsin (?<=[FYWL])(?!P)
   // CNBr (?<=M)
   // Asp-N (?=[BD])
   // Lys-N (?=K)

   //search enzyme 1
   if (g_staticParams.enzymeInformation.iSearchEnzymeOffSet == 1)
   {
      sprintf(szRegExpression, "(?&lt;=[%s])", g_staticParams.enzymeInformation.szSearchEnzymeBreakAA);
      if (g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA[0] != '-')
         sprintf(szRegExpression+strlen(szRegExpression), "(?!%s)", g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA);
   }
   else
   {
      szRegExpression[0] = '\0';
      if (g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA[0] != '-')
         sprintf(szRegExpression, "(?&lt;=[%s]) ", g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA);
      sprintf(szRegExpression+strlen(szRegExpression), "(?=[%s])", g_staticParams.enzymeInformation.szSearchEnzymeBreakAA);
   }

   string sAdditionalEnzymeInfo = GetAdditionalEnzymeInfo(1);

   fprintf(fpout, "     <Enzyme id=\"ENZ\"  missedCleavages=\"%d\" semiSpecific=\"%s\">\n", 
         g_staticParams.enzymeInformation.iAllowedMissedCleavage, szSemi);
   fprintf(fpout, "      <SiteRegexp>(%s)</SiteRegexp>\n", szRegExpression);
   fprintf(fpout, "%s", sAdditionalEnzymeInfo.c_str());
   fprintf(fpout, "     </Enzyme>\n");

   //search enzyme 2
   if (!g_staticParams.enzymeInformation.bNoEnzyme2Selected)
   {
      if (g_staticParams.enzymeInformation.iSearchEnzyme2OffSet == 1)
      {
         sprintf(szRegExpression, "(?&lt;=[%s])", g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA);
         if (g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA[0] != '-')
            sprintf(szRegExpression+strlen(szRegExpression), "(?!%s)", g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA);
      }
      else
      {
         szRegExpression[0] = '\0';
         if (g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA[0] != '-')
            sprintf(szRegExpression, "(?&lt;=[%s])", g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA);
         sprintf(szRegExpression, "(?=[%s])", g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA);
      }

      string sAdditionalEnzymeInfo = GetAdditionalEnzymeInfo(2);
      fprintf(fpout, "     <Enzyme id=\"ENZ2\"  missedCleavages=\"%d\" semiSpecific=\"%s\">\n", 
            g_staticParams.enzymeInformation.iAllowedMissedCleavage, szSemi);
      fprintf(fpout, "      <SiteRegexp>(%s)</SiteRegexp>\n", szRegExpression);
      fprintf(fpout, "%s", sAdditionalEnzymeInfo.c_str());
      fprintf(fpout, "     </Enzyme>\n");
   }

   fprintf(fpout, "   </Enzymes>\n");
}


void CometWriteMzIdentML::WriteMassTable(FILE *fpout)
{
   if (g_staticParams.massUtility.bMonoMassesParent == g_staticParams.massUtility.bMonoMassesFragment)
   {
      fprintf(fpout, "   <MassTable id=\"MT\" msLevel=\"1 %d\">\n", g_staticParams.options.iMSLevel);
      for (int i = 65; i <= 90; ++i)   // 64=A, 90=Z
      {
         if (g_staticParams.massUtility.pdAAMassFragment[i] < 999998.0)
         {
            fprintf(fpout, "    <Residue code=\"%c\" mass=\"%lf\" />\n",
                  i, g_staticParams.massUtility.pdAAMassFragment[i] - g_staticParams.staticModifications.pdStaticMods[i]);
         }
      }
      fprintf(fpout, "   </MassTable>\n");
   }
   else
   {
      fprintf(fpout, "   <MassTable id=\"MT\" msLevel=\"1\">\n");
      for (int i = 65; i <= 90; ++i)   // 64=A, 90=Z
      {
         if (g_staticParams.massUtility.pdAAMassParent[i] < 999998.0)
         {
            fprintf(fpout, "    <Residue code=\"%c\" mass=\"%lf\" />\n",
                  i, g_staticParams.massUtility.pdAAMassParent[i] - g_staticParams.staticModifications.pdStaticMods[i]);
         }
      }
      fprintf(fpout, "   </MassTable>\n");

      fprintf(fpout, "   <MassTable id=\"MT2\" msLevel=\"%d\">\n", g_staticParams.options.iMSLevel);
      for (int i = 65; i <= 90; ++i)   // 64=A, 90=Z
      {
         if (g_staticParams.massUtility.pdAAMassFragment[i] < 999998.0)
         {
            fprintf(fpout, "    <Residue code=\"%c\" mass=\"%lf\" />\n",
                  i, g_staticParams.massUtility.pdAAMassFragment[i] - g_staticParams.staticModifications.pdStaticMods[i]);
         }
      }
      fprintf(fpout, "   </MassTable>\n");
   }
}


void CometWriteMzIdentML::WriteTolerance(FILE *fpout)
{
   // bin size does not translate to Da or PPM so will use Da
   fprintf(fpout, "   <FragmentTolerance>\n");
   fprintf(fpout, "    <cvParam accession=\"MS:1001412\" name=\"search tolerance plus value\" value=\"%lf\" cvRef=\"PSI-MS\" unitAccession=\"UO:0000221\" unitName=\"dalton\" unitCvRef=\"UO\" />\n",
         g_staticParams.tolerances.dFragmentBinSize / 2.0);
   fprintf(fpout, "    <cvParam accession=\"MS:1001413\" name=\"search tolerance minus value\" value=\"%lf\" cvRef=\"PSI-MS\" unitAccession=\"UO:0000221\" unitName=\"dalton\" unitCvRef=\"UO\" />\n",
         g_staticParams.tolerances.dFragmentBinSize / 2.0);
   fprintf(fpout, "   </FragmentTolerance>\n");


   // not sure how isotope error is handled
   fprintf(fpout, "   <ParentTolerance>\n");

   if (g_staticParams.tolerances.iMassToleranceUnits == 0)
   {
      fprintf(fpout, "    <cvParam accession=\"MS:1001412\" name=\"search tolerance plus value\" value=\"%lf\" cvRef=\"PSI-MS\" unitAccession=\"UO:0000221\" unitName=\"dalton\" unitCvRef=\"UO\" />\n", g_staticParams.tolerances.dInputToleranceMinus);
      fprintf(fpout, "    <cvParam accession=\"MS:1001413\" name=\"search tolerance minus value\" value=\"%lf\" cvRef=\"PSI-MS\" unitAccession=\"UO:0000221\" unitName=\"dalton\" unitCvRef=\"UO\" />\n", g_staticParams.tolerances.dInputTolerancePlus);
   }
   else if (g_staticParams.tolerances.iMassToleranceUnits == 2)
   {
      fprintf(fpout, "   <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001412\" name=\"search tolerance plus value\" value=\"%lf\" unitAccession=\"UO:0000169\" unitName=\"parts per million\" unitCvRef=\"UO\"></cvParam>\n", g_staticParams.tolerances.dInputToleranceMinus);
      fprintf(fpout, "   <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001413\" name=\"search tolerance minus value\" value=\"%lf\" unitAccession=\"UO:0000169\" unitName=\"parts per million\" unitCvRef=\"UO\"></cvParam>\n", g_staticParams.tolerances.dInputTolerancePlus);
   }
   else  // invalid
   {
      fprintf(fpout, "   <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001412\" name=\"search tolerance plus value\" value=\"%lf\"></cvParam>\n", g_staticParams.tolerances.dInputToleranceMinus);
      fprintf(fpout, "   <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001413\" name=\"search tolerance minus value\" value=\"%lf\"></cvParam>\n", g_staticParams.tolerances.dInputTolerancePlus);
   }

   fprintf(fpout, "   </ParentTolerance>\n");

   fprintf(fpout, "   <Threshold>\n");
   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001494\" name=\"no threshold\" />\n");
   fprintf(fpout, "   </Threshold>\n");
}


void CometWriteMzIdentML::WriteInputs(FILE *fpout)
{
   fprintf(fpout, "  <Inputs>\n");
   fprintf(fpout, "   <SearchDatabase id=\"DB\" location=\"%s\">\n", g_staticParams.databaseInfo.szDatabase);
   fprintf(fpout, "    <FileFormat>\n");
   fprintf(fpout, "     <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001348\" name=\"FASTA format\" />\n");
   fprintf(fpout, "    </FileFormat>\n");
   fprintf(fpout, "    <DatabaseName>\n");

   char *pStr;
   char szNoPathDatabase[512];
   char cSep;
#ifdef _WIN32
   cSep = '\\';
#else
   cSep = '/';
#endif
   if ( (pStr = strrchr(g_staticParams.databaseInfo.szDatabase, cSep)) != NULL)
      strcpy(szNoPathDatabase, pStr);
   else
      strcpy(szNoPathDatabase, g_staticParams.databaseInfo.szDatabase);

   fprintf(fpout, "     <userParam type=\"string\" name=\"%s\" />\n", szNoPathDatabase);
   fprintf(fpout, "    </DatabaseName>\n");
   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001073\" name=\"database type amino acid\" />\n");
   if (g_staticParams.options.iDecoySearch == 1)
   {
      fprintf(fpout, "    <cvParam accession=\"MS:XXXXXXX\" cvRef=\"PSI-MS\" name=\"Comet internal target+decoy\" />\n");
      fprintf(fpout, "    <cvParam accession=\"MS:1001283\" cvRef=\"PSI-MS\" value=\"^%s\" name=\"decoy DB accession regexp\" />\n", g_staticParams.sDecoyPrefix.c_str());
      fprintf(fpout, "    <cvParam accession=\"MS:XXXXXXX\" cvRef=\"PSI-MS\" name=\"Comet internal pseudo reverse decoy peptide\" />\n");
   }
   else if (g_staticParams.options.iDecoySearch == 2)
   {
      fprintf(fpout, "    <cvParam accession=\"MS:XXXXXXX\" cvRef=\"PSI-MS\" name=\"Comet internal separate target-decoy\" />\n");
      fprintf(fpout, "    <cvParam accession=\"MS:1001283\" cvRef=\"PSI-MS\" value=\"^%s\" name=\"decoy DB accession regexp\" />\n", g_staticParams.sDecoyPrefix.c_str());
      fprintf(fpout, "    <cvParam accession=\"MS:XXXXXXX\" cvRef=\"PSI-MS\" name=\"Comet internal pseudo reverse decoy peptide\" />\n");
   }
   fprintf(fpout, "   </SearchDatabase>\n");

   fprintf(fpout, "   <SpectraData location=\"%s\" id=\"SD\" >\n", g_staticParams.inputFile.szFileName);
   fprintf(fpout, "    <FileFormat>\n");

   char szFormatAccession[24];
   char szFormatName[128];
   char szSpectrumAccession[24];
   char szSpectrumName[128];
   int iLen = (int)strlen(g_staticParams.inputFile.szFileName);
   char szFileNameLower[SIZE_FILE];

   for (int x = 0; x < iLen; ++x)
      szFileNameLower[x] = tolower(g_staticParams.inputFile.szFileName[x]);
   szFileNameLower[iLen] = '\0';

   if (!strcmp(szFileNameLower + iLen - 4, ".raw"))
   {
      strcpy(szFormatAccession, "MS:1000563"); // Thermo RAW
      strcpy(szFormatName, "Thermo RAW file");

      strcpy(szSpectrumAccession, "MS:1000776");
      strcpy(szSpectrumName, "scan number only nativeID format");
   }
   else if (!strcmp(szFileNameLower + iLen - 6, ".mzxml")
         || !strcmp(szFileNameLower + iLen - 9, ".mzxml.gz"))
   {
      strcpy (szFormatAccession, "MS:1000566");  // mzXML
      strcpy(szFormatName, "ISB mzXML file");

      strcpy(szSpectrumAccession, "MS:1000776");
      strcpy(szSpectrumName, "scan number only nativeID format");
   }
   else if (!strcmp(szFileNameLower + iLen - 5, ".mzml")
         || !strcmp(szFileNameLower + iLen - 8, ".mzml.gz"))
   {
      strcpy (szFormatAccession, "MS:1000584");  // mzML
      strcpy(szFormatName, "mzML file");

      strcpy(szSpectrumAccession, "MS:1001530");
      strcpy(szSpectrumName, "mzML unique identifier");
   }
   else if (!strcmp(szFileNameLower + iLen - 4, ".ms2"))
   {
      strcpy (szFormatAccession, "MS:1001466");  // ms2
      strcpy(szFormatName, "MS2 file");

      strcpy(szSpectrumAccession, "MS:1000776");
      strcpy(szSpectrumName, "scan number only nativeID format");
   }
   else if (!strcmp(szFileNameLower + iLen - 4, ".mgf"))
   {
      strcpy (szFormatAccession, "MS:1001062");  // mgf
      strcpy(szFormatName, "Mascot MGF file");

      strcpy(szSpectrumAccession, "MS:1000776");
      strcpy(szSpectrumName, "scan number only nativeID format");
   }
   else
   {
      strcpy (szFormatAccession, "error");
      strcpy(szFormatName, "error");

      strcpy(szSpectrumAccession, "error");
      strcpy(szSpectrumName, "error");
   }

   fprintf(fpout, "     <cvParam cvRef=\"PSI-MS\" accession=\"%s\" name=\"%s\" />\n", szFormatAccession, szFormatName);
   fprintf(fpout, "    </FileFormat>\n");
   fprintf(fpout, "    <SpectrumIDFormat>\n");
   fprintf(fpout, "     <cvParam cvRef=\"PSI-MS\" accession=\"%s\" name=\"%s\" />\n", szSpectrumAccession, szSpectrumName);
   fprintf(fpout, "    </SpectrumIDFormat>\n");
   fprintf(fpout, "   </SpectraData>\n");
   fprintf(fpout, "  </Inputs>\n");
}


void CometWriteMzIdentML::WriteSpectrumIdentificationList(FILE* fpout,
      FILE *fpdb,
      vector<MzidTmpStruct>* vMzid)
{
   fprintf(fpout, "   <SpectrumIdentificationList id=\"SIL\">\n");

   long lCount = 1;

   double dPrevRT = 0;
   for (std::vector<MzidTmpStruct>::iterator itMzid = (*vMzid).begin(); itMzid < (*vMzid).end(); ++itMzid)
   {
      char szProteinName[512];
      string strProteinName;
      long lOffset;

      if ((*itMzid).iWhichResult == 0)
      {
         if (itMzid != (*vMzid).begin())
         {
            if (dPrevRT > 0.0)
            {
               fprintf(fpout, "     <cvParam cvRef=\"PSI-MS\" accession=\"MS:1000894\" name=\"retention time\" value=\"%0.4f\" unitCvRef=\"UO\" unitAccession=\"UO:0000010\" unitName=\"second\"/>\n", dPrevRT);
            }
            fprintf(fpout, "    </SpectrumIdentificationResult>\n");
         }
         fprintf(fpout, "    <SpectrumIdentificationResult id=\"SIR_%d.%d\" spectrumID=\"%d\" spectraData_ref=\"SD\">\n",
               (*itMzid).iWhichQuery,
               (*itMzid).iBatchNum,
               (*itMzid).iScanNumber);
      }

      fprintf(fpout, "     <SpectrumIdentificationItem id=\"SII_%d.%d.%d\" rank=\"%d\" chargeState=\"%d\" peptide_ref=\"%s;%s\" experimentalMassToCharge=\"%f\" calculatedMassToCharge=\"%f\" passThreshold=\"false\">\n",
            (*itMzid).iWhichQuery,
            (*itMzid).iBatchNum,
            (*itMzid).iWhichResult + 1,
            (*itMzid).iWhichResult + 1,
            (*itMzid).iCharge,
            (*itMzid).strPeptide.c_str(),
            (*itMzid).strMods.c_str(),
            ((*itMzid).dExpMass + (*itMzid).iCharge * PROTON_MASS) / (*itMzid).iCharge,
            ((*itMzid).dCalcMass + (*itMzid).iCharge * PROTON_MASS) / (*itMzid).iCharge);

      std::string field;    // used below for parsing protein offset:iStartResidue pairs
      std::string field2;   // used below for parsing protein offset:iStartResidue pairs

      if ((*itMzid).strProtsTarget.size() > 0)
      {
         // walk through semi-colon delimited list to get each protein file offset
         std::istringstream isString((*itMzid).strProtsTarget);

         while ( std::getline(isString, field, ';') )
         {
            std::istringstream isString2(field);       // grab the offset in "offset:iStartResidue" string
            std::getline(isString2, field2, ':');
            lOffset = stol(field2);

            if (lOffset >= 0)
            {
               CometMassSpecUtils::GetProteinName(fpdb, lOffset, szProteinName);
               strProteinName = szProteinName;
               CometMassSpecUtils::EscapeString(strProteinName);

               fprintf(fpout, "      <PeptideEvidenceRef peptideEvidence_ref=\"%s;%s;%s\" />\n",
                     (*itMzid).strPeptide.c_str(),
                     (*itMzid).strMods.c_str(),
                     strProteinName.c_str() );
            }
         }
      }
      if ((*itMzid).strProtsDecoy.size() > 0)
      {
         // walk through semi-colon delimited list to get each protein file offset
         std::istringstream isString((*itMzid).strProtsDecoy);

         while ( std::getline(isString, field, ';') )
         {
            std::istringstream isString2(field);       // grab the offset in "offset:iStartResidue" string
            std::getline(isString2, field2, ':');
            lOffset = stol(field);

            if (lOffset >= 0)
            {
               CometMassSpecUtils::GetProteinName(fpdb, lOffset, szProteinName);
               strProteinName = szProteinName;
               CometMassSpecUtils::EscapeString(strProteinName);

               fprintf(fpout, "      <PeptideEvidenceRef peptideEvidence_ref=\"%s;%s;%s%s\" />\n",
                     (*itMzid).strPeptide.c_str(),
                     (*itMzid).strMods.c_str(),
                     g_staticParams.sDecoyPrefix.c_str(), 
                     strProteinName.c_str() );
            }
         }
      }

      fprintf(fpout, "      <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001121\" name=\"number of matched peaks\" value=\"%d\" />\n", (*itMzid).iMatchedIons);
      fprintf(fpout, "      <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001362\" name=\"number of unmatched peaks\" value=\"%d\" />\n", (*itMzid).iTotalIons - (*itMzid).iMatchedIons);
      fprintf(fpout, "      <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002252\" name=\"Comet:xcorr\" value=\"%0.4f\" />\n", (*itMzid).fXcorr);
      fprintf(fpout, "      <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002253\" name=\"Comet:deltacn\" value=\"%0.4f\" />\n", (*itMzid).fCn);
      fprintf(fpout, "      <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002255\" name=\"Comet:spscore\" value=\"%0.4f\" />\n", (*itMzid).fSp);
      fprintf(fpout, "      <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002256\" name=\"Comet:sprank\" value=\"%d\" />\n", (*itMzid).iRankSp);
      fprintf(fpout, "      <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002257\" name=\"Comet:expectation value\" value=\"%0.2E\" />\n", (*itMzid).dExpect);
//    fprintf(fpout, "      <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002500\" name=\"peptide passes threshold\" value=\"false\" />\n");
      fprintf(fpout, "     </SpectrumIdentificationItem>\n");


      dPrevRT = (*itMzid).dRTime; // grab retention time of scan to use in next iteration of for loop

      lCount++;
   }

   if (dPrevRT > 0.0)
   {
      fprintf(fpout, "     <cvParam cvRef=\"PSI-MS\" accession=\"MS:1000016\" name=\"start scan time\" value=\"%0.4f\" unitCvRef=\"UO\" unitAccession=\"UO:0000010\" unitName=\"second\"/>\n", dPrevRT);
   }

   fprintf(fpout, "    </SpectrumIdentificationResult>\n");

   time_t tTime;
   char szDate[48];
   time(&tTime);
   strftime(szDate, 46, "%Y-%m-%dT%H:%M:%S", localtime(&tTime));

   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001035\" name=\"date / time search performed\" value=\"%s\" />\n", szDate);
   fprintf(fpout, "   </SpectrumIdentificationList>\n");
}


void CometWriteMzIdentML::PrintTmpPSM(int iWhichQuery,
                                      int iPrintTargetDecoy,
                                      int iBatchNum,
                                      FILE *fpout)
{
   if ((iPrintTargetDecoy != 2 && g_pvQuery.at(iWhichQuery)->_pResults[0].fXcorr > g_staticParams.options.dMinimumXcorr)
         || (iPrintTargetDecoy == 2 && g_pvQuery.at(iWhichQuery)->_pDecoys[0].fXcorr > g_staticParams.options.dMinimumXcorr))
   {
      Query* pQuery = g_pvQuery.at(iWhichQuery);

      Results *pOutput;
      int iNumPrintLines;

      if (iPrintTargetDecoy == 2)  // decoys
      {
         pOutput = pQuery->_pDecoys;
         iNumPrintLines = pQuery->iDecoyMatchPeptideCount;
      }
      else  // combined or separate targets
      {
         pOutput = pQuery->_pResults;
         iNumPrintLines = pQuery->iMatchPeptideCount;
      }

      if (iNumPrintLines > g_staticParams.options.iNumPeptideOutputLines)
         iNumPrintLines = g_staticParams.options.iNumPeptideOutputLines;

      int iMinLength = 999;
      for (int i=0; i<iNumPrintLines; ++i)
      {
         int iLen = (int)strlen(pOutput[i].szPeptide);
         if (iLen == 0)
            break;
         if (iLen < iMinLength)
            iMinLength = iLen;
      }

      for (int iWhichResult=0; iWhichResult<iNumPrintLines; ++iWhichResult)
      {
         if (pOutput[iWhichResult].fXcorr <= g_staticParams.options.dMinimumXcorr)
            continue;

         fprintf(fpout, "%d\t", pQuery->_spectrumInfoInternal.iScanNumber);
         fprintf(fpout, "%d\t", iBatchNum);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].iRankXcorr);
         fprintf(fpout, "%d\t", pQuery->_spectrumInfoInternal.iChargeState);
         fprintf(fpout, "%0.6f\t", pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS);
         fprintf(fpout, "%0.6f\t", pOutput[iWhichResult].dPepMass - PROTON_MASS);
         fprintf(fpout, "%0.2E\t", pOutput[iWhichResult].dExpect);
         fprintf(fpout, "%0.4f\t", pOutput[iWhichResult].fXcorr);
         fprintf(fpout, "%0.4f\t", pOutput[iWhichResult].fDeltaCn);
         fprintf(fpout, "%0.1f\t", pOutput[iWhichResult].fScoreSp);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].iRankSp);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].iMatchedIons);
         fprintf(fpout, "%d\t", pOutput[iWhichResult].iTotalIons);

         // plain peptide
         fprintf(fpout, "%s\t", pOutput[iWhichResult].szPeptide);

         // prev/next AA
         fprintf(fpout, "%c%c\t", pOutput[iWhichResult].cPrevAA, pOutput[iWhichResult].cNextAA);

         // modifications:  zero-position:mass; semi-colon delimited; length=nterm, length+1=c-term

         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide] > 0)
         {
            fprintf(fpout, "%d:%0.6f;", pOutput[iWhichResult].iLenPeptide, 
                  g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide]-1].dVarModMass);
         }

         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0)
         {
            fprintf(fpout, "%d:%0.6f;", pOutput[iWhichResult].iLenPeptide + 1, 
                  g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].dVarModMass);
         }

         for (int i=0; i<pOutput[iWhichResult].iLenPeptide; ++i)
         {
            if (pOutput[iWhichResult].piVarModSites[i] != 0)
               fprintf(fpout, "%d:%0.6f;", i, pOutput[iWhichResult].pdVarModSites[i]);
         }

         fprintf(fpout, "\t");

         // semicolon separated list of fpdb pointers for target proteins
         std::vector<ProteinEntryStruct>::iterator it;
         if (pOutput[iWhichResult].pWhichProtein.size() > 0)
         {
            for (it=pOutput[iWhichResult].pWhichProtein.begin(); it!=pOutput[iWhichResult].pWhichProtein.end(); ++it)
            {
#ifdef _WIN32
               fprintf(fpout, "%I64d:%d;", (*it).lWhichProtein, (*it).iStartResidue);
#else
               fprintf(fpout, "%ld:%d;", (*it).lWhichProtein, (*it).iStartResidue);
#endif
            }
            fprintf(fpout, "\t");
         }
         else
         {
            fprintf(fpout, "-1;\t");
         }

         // semicolon separated list of fpdb pointers for decoy proteins
         if (pOutput[iWhichResult].pWhichDecoyProtein.size() > 0)
         {
            for (it=pOutput[iWhichResult].pWhichDecoyProtein.begin(); it!=pOutput[iWhichResult].pWhichDecoyProtein.end(); ++it)
            {
#ifdef _WIN32
               fprintf(fpout, "%I64d:%d;", (*it).lWhichProtein, (*it).iStartResidue);
#else
               fprintf(fpout, "%ld:%d;", (*it).lWhichProtein, (*it).iStartResidue);
#endif
            }
            fprintf(fpout, "\t");
         }
         else
         {
            fprintf(fpout, "-2;\t");
         }

         fprintf(fpout, "%d\t%d\t", iWhichQuery, iWhichResult);

         fprintf(fpout, "%0.4f", pQuery->_spectrumInfoInternal.dRTime);
 
         fprintf(fpout, "\n");
      }
   }
}


string CometWriteMzIdentML::GetAdditionalEnzymeInfo(int iWhichEnzyme)
{
   string sBreakAA;
   string sNoBreakAA;
   string sReturnString = "";
   int iOffSet;

   if (iWhichEnzyme == 1)
   {
      iOffSet = g_staticParams.enzymeInformation.iSearchEnzymeOffSet;
      sBreakAA = g_staticParams.enzymeInformation.szSearchEnzymeBreakAA;
      sNoBreakAA = g_staticParams.enzymeInformation.szSearchEnzymeNoBreakAA;
   }
   else // 2nd enzyme
   {
      iOffSet = g_staticParams.enzymeInformation.iSearchEnzyme2OffSet;
      sBreakAA = g_staticParams.enzymeInformation.szSearchEnzyme2BreakAA;
      sNoBreakAA = g_staticParams.enzymeInformation.szSearchEnzyme2NoBreakAA;
   }

   sReturnString  = "      <EnzymeName>\n";
   if (sNoBreakAA.length() == 1)
   {
      if (iOffSet == 1)
      {
         if (sBreakAA.length() == 1)
         {
            if (sBreakAA == "K" && sNoBreakAA == "P")
            {
               sReturnString += "       <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001335\" name=\"Lys-C\"/>\n";
            }
            else if (sBreakAA == "R" && sNoBreakAA == "P")
            {
               sReturnString += "       <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001272\" name=\"Arg-C\"/>\n";
            }
            else if (sBreakAA == "M" && sNoBreakAA == "-")
            {
               sReturnString += "       <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001333\" name=\"CNBr\"/>\n";
            }
         }
         else if (sBreakAA.length() == 2)
         {
            if (sBreakAA.find('K')!=std::string::npos && sBreakAA.find('R')!=std::string::npos && sNoBreakAA == "P")
            {
               sReturnString += "       <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001251\" name=\"Trypsin\"/>\n";
            }
            else if (sBreakAA.find('K')!=std::string::npos && sBreakAA.find('R')!=std::string::npos && sNoBreakAA == "-")
            {
               sReturnString += "       <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001313\" name=\"Trypsin/P\"/>\n";
            }
            else if (sBreakAA.find('D')!=std::string::npos && sBreakAA.find('E')!=std::string::npos && sNoBreakAA == "-")
            {
               sReturnString += "       <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001274\" name=\"Asp-N_ambic\"/>\n";
            }
            else if (sBreakAA.find('F')!=std::string::npos && sBreakAA.find('L')!=std::string::npos && sNoBreakAA == "-")
            {
               sReturnString += "       <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001337\" name=\"PepsinA\"/>\n";
            }
         }
         else if (sBreakAA.length() == 4)
         {
            if (sBreakAA.find('F')!=std::string::npos && sBreakAA.find('W')!=std::string::npos
                  && sBreakAA.find('Y')!=std::string::npos && sBreakAA.find('L')!=std::string::npos && sNoBreakAA == "P")
            {
               sReturnString += "       <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001332\" name=\"Chymotrypsin\"/>\n";
            }
         }
   
      }
      else if (iOffSet == 0)
      {
         if (sBreakAA.length() == 2 && sBreakAA.find('N')!=std::string::npos
               && sBreakAA.find('D')!=std::string::npos && sNoBreakAA == "-")
         {
            sReturnString += "       <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001273\" name=\"Asp-N\"/>\n";
         }
         else if (sBreakAA == "K" && sNoBreakAA == "-")
         {
            sReturnString += "       <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001335\" name=\"Lys-N\"/>\n";
         }
      }
      sReturnString += "      </EnzymeName>\n";
   }

   // if it does not contain "<cvParam" string, that means no enzyme match
   if (sReturnString.find("<cvParam") == std::string::npos)
      sReturnString = "";

   return sReturnString;
}
