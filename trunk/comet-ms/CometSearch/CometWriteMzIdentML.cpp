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
#include "CometWriteMzIdentML.h"
#include "CometSearchManager.h"
#include "CometStatus.h"

#include "limits.h"
#include "stdlib.h"

#ifdef _WIN32
#define PATH_MAX _MAX_PATH
#endif

CometWriteMzIdentML::CometWriteMzIdentML()
{
}


CometWriteMzIdentML::~CometWriteMzIdentML()
{
}


void CometWriteMzIdentML::WriteMzIdentML(FILE *fpout,
                                         FILE *fpoutd,
                                         FILE *fpdb)
{
   int i;

   WriteSequenceCollection(fpout, fpdb);

   // Print results.
   for (i=0; i<(int)g_pvQuery.size(); i++)
      PrintResults(i, 0, fpout, fpdb);

   // Print out the separate decoy hits.
   if (g_staticParams.options.iDecoySearch == 2)
   {
      for (i=0; i<(int)g_pvQuery.size(); i++)
      {
         PrintResults(i, 1, fpoutd, fpdb);
      }
   }

   fflush(fpout);
}

bool CometWriteMzIdentML::WriteMzIdentMLHeader(FILE *fpout,
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
   char szRunSummaryBaseName[PATH_MAX];          // base name of szInputFile
   char szRunSummaryResolvedPath[PATH_MAX];      // resolved path of szInputFile
   int  iLen = (int)strlen(g_staticParams.inputFile.szFileName);
   strcpy(szRunSummaryBaseName, g_staticParams.inputFile.szFileName);
   if ( (pStr = strrchr(szRunSummaryBaseName, '.')))
      *pStr = '\0';

   if (!STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 9, ".mzXML.gz")
         || !STRCMP_IGNORE_CASE(g_staticParams.inputFile.szFileName + iLen - 8, ".mzML.gz"))
   {
      if ( (pStr = strrchr(szRunSummaryBaseName, '.')))
         *pStr = '\0';
   }

   char resolvedPathBaseName[PATH_MAX];
#ifdef _WIN32
   _fullpath(resolvedPathBaseName, g_staticParams.inputFile.szBaseName, PATH_MAX);
   _fullpath(szRunSummaryResolvedPath, szRunSummaryBaseName, PATH_MAX);
#else
   realpath(g_staticParams.inputFile.szBaseName, resolvedPathBaseName);
   realpath(szRunSummaryBaseName, szRunSummaryResolvedPath);
#endif

   // Write out pepXML header.
   fprintf(fpout, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

   fprintf(fpout, "<MzIdentML id=\"Comet %s\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://psidev.info/psi/pi/mzIdentML/1.2 http://www.psidev.info/files/mzIdentML1.2.0.xsd\" xmlns=\"http://psidev.info/psi/pi/mzIdentML/1.2\" version=\"1.2.0\" creationDate=\"%s\">\n", szDate, comet_version);
   fprintf(fpout, " <cvList>\n");
   fprintf(fpout, "  <cv id=\"PSI-MS\" uri=\"https://raw.githubusercontent.com/HUPO-PSI/psi-ms-CV/master/psi-ms.obo\" fullName=\"PSI-MS\"/>\n");
   fprintf(fpout, "  <cv id=\"UNIMOD\" uri=\"http://www.unimod.org/obo/unimod.obo\" fullName=\"UNIMOD\"/>\n");
   fprintf(fpout, "  <cv id=\"UO\" uri=\"https://raw.githubusercontent.com/bio-ontology-research-group/unit-ontology/master/unit.obo\" fullName=\"UNIT-ONTOLOGY\"/>\n");
   fprintf(fpout, "  <cv id=\"PRIDE\" uri=\"https://github.com/PRIDE-Utilities/pride-ontology/blob/master/pride_cv.obo\" fullName=\"PRIDE\"/>\n");
   fprintf(fpout, " </cvList>\n");


   fprintf(fpout, " <AnalysisSoftwareList>\n");
   fprintf(fpout, "  <AnalysisSoftware id=\"AS_Comet\" name=\"Comet\" version=\"%s\">\n", comet_version);
   fprintf(fpout, "   <SoftwareName><cvParam cvRef=\"MS\" accession=\"MS:1002251\" name=\"Comet\" value=\"\"/></SoftwareName>\n");
   fprintf(fpout, "  </AnalysisSoftware>\n");
   fprintf(fpout, " </AnalysisSoftwareList>\n");

   fflush(fpout);

   return true;
}


void CometWriteMzIdentML::WriteSequenceCollection(FILE *fpout,
                                                  FILE *fpdb)
{
   std::vector<string> vProteinTargets;  // store vector of target protein names
   std::vector<string> vProteinDecoys;   // store vector of decoy protein names
   int iNumPrintLines;

   fprintf(fpout, " <SequenceCollection xmlns=\"http://psidev.info/psi/pi/mzIdentML/1.2\">\n");

   // get all protein names
   for (int iWhichQuery=0; iWhichQuery<(int)g_pvQuery.size(); iWhichQuery++)
   {
      iNumPrintLines = g_pvQuery.at(iWhichQuery)->iMatchPeptideCount + g_pvQuery.at(iWhichQuery)->iDecoyMatchPeptideCount;

      if (iNumPrintLines > g_staticParams.options.iNumPeptideOutputLines)
         iNumPrintLines = g_staticParams.options.iNumPeptideOutputLines;

      for (int iWhichResult=0; iWhichResult<iNumPrintLines; iWhichResult++)
      {
         CometMassSpecUtils::GetProteinNameString(fpdb, iWhichQuery, iWhichResult, 0, vProteinTargets, vProteinDecoys);
      }
   }

   // now unique sort vProteinTargets and vProteinDecoys
   std::sort(vProteinTargets.begin(), vProteinTargets.end());
   vProteinTargets.erase(std::unique(vProteinTargets.begin(), vProteinTargets.end()), vProteinTargets.end());

   std::sort(vProteinDecoys.begin(), vProteinDecoys.end());
   vProteinDecoys.erase(std::unique(vProteinDecoys.begin(), vProteinDecoys.end()), vProteinDecoys.end());

   std::vector<string>::iterator it;

   for (it = vProteinTargets.begin(); it != vProteinTargets.end(); it++)
   {
      fprintf(fpout, "1 <DBSequence id=\"%s\" accession=\"%s\" searchDatabase_ref=\"DB_1\"/>\n", (*it).c_str(), (*it).c_str());
   }
   for (it = vProteinDecoys.begin(); it != vProteinDecoys.end(); it++)
   {
      fprintf(fpout, "2 <DBSequence id=\"%s\" accession=\"%s\" searchDatabase_ref=\"DB_1\"/>\n", (*it).c_str(), (*it).c_str());
   }

   fprintf(fpout, " </SequenceCollection>\n");
}


void CometWriteMzIdentML::WriteAnalysisProtocol(FILE *fpout)
{
   fprintf(fpout, " <AnalysisCollection>\n");
   fprintf(fpout, "  <SpectrumIdentification spectrumIdentificationList_ref=\"SIL_1\" spectrumIdentificationProtocol_ref=\"SIP_1\" id=\"SpecIdent_1\">\n");
   fprintf(fpout, "   <InputSpectra spectraData_ref=\"qExactive01819.mgf\"/>\n");
   fprintf(fpout, "   <SearchDatabaseRef searchDatabase_ref=\"SearchDB_1\"/>\n");
   fprintf(fpout, "  </SpectrumIdentification>\n");
   fprintf(fpout, "  <ProteinDetection proteinDetectionProtocol_ref=\"PeptideShaker_1\" proteinDetectionList_ref=\"Protein_groups\" id=\"PD_1\">\n");
   fprintf(fpout, "   <InputSpectrumIdentifications spectrumIdentificationList_ref=\"SIL_1\"/>\n");
   fprintf(fpout, "  </ProteinDetection>\n");

   fprintf(fpout, " </AnalysisCollection>\n");

   fprintf(fpout, " <AnalysisProtocolCollection>\n");
   fprintf(fpout, "  <SpectrumIdentificationProtocol analysisSoftware_ref=\"ID_software\" id=\"SIP_1\">\n");
   fprintf(fpout, "   <SearchType>\n");
   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001083\" name=\"ms-ms search\"/>\n");
   fprintf(fpout, "   </SearchType>\n");
   fprintf(fpout, "   <AdditionalSearchParams>\n");
   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001211\" name=\"parent mass type mono\"/>\n");
   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001256\" name=\"fragment mass type mono\"/>\n");
/*
      <userParam name="NumTolerableTermini" value="2"/>
      <userParam name="NumMatchesPerSpec" value="1"/>
      <userParam name="MaxNumModifications" value="2"/>
      <userParam name="MinPepLength" value="6"/>
      <userParam name="MaxPepLength" value="40"/>
      <userParam name="MinCharge" value="2"/>
        <userParam name="digest_mass_range" value="600.0000 35000.0000"/>
        <userParam name="add_Cterm_peptide" value="0.0000"/>
        <userParam name="add_Cterm_protein" value="0.0000"/>
        <userParam name="add_Nterm_peptide" value="229.162932"/>
        <userParam name="add_Nterm_protein" value="0.0000"/>
        <userParam name="add_G_Glycine" value="0.0000"/>
        <userParam name="add_A_Alanine" value="0.0000"/>
        <userParam name="add_S_Serine" value="0.0000"/>
        <userParam name="add_P_Proline" value="0.0000"/>
        <userParam name="add_V_Valine" value="0.0000"/>
        <userParam name="add_T_Threonine" value="0.0000"/>
      </AdditionalSearchParams>
      <ModificationParams>
        <SearchModification fixedMod="false" massDelta="15.9949146221" residues="M">
          <cvParam cvRef="UNIMOD" accession="UNIMOD:35" name="Oxidation" value=""/>
        </SearchModification>
        <SearchModification fixedMod="true" massDelta="229.162932" residues=".">
          <SpecificityRules>
            <cvParam cvRef="MS" accession="MS:1001189" name="modification specificity peptide N-term" value=""/>
          </SpecificityRules>
          <cvParam cvRef="UNIMOD" accession="UNIMOD:737" name="TMT6plex" value=""/>
        </SearchModification>
        <SearchModification fixedMod="true" massDelta="57.02146374" residues="C">
          <cvParam cvRef="UNIMOD" accession="UNIMOD:4" name="Carbamidomethyl" value=""/>
        </SearchModification>
        <SearchModification fixedMod="true" massDelta="229.162932" residues="K">
          <cvParam cvRef="UNIMOD" accession="UNIMOD:737" name="TMT6plex" value=""/>
        </SearchModification>
      </ModificationParams>
      <Enzymes independent="false">
        <Enzyme id="ENZ_1" cTermGain="OH" nTermGain="H" minDistance="1" semiSpecific="false">
          <SiteRegexp>(?&lt;=[KR])</SiteRegexp>
        </Enzyme>
      </Enzymes>
      <Threshold>
        <cvParam cvRef="MS" accession="MS:1001494" name="no threshold" value=""/>
      </Threshold>
    </SpectrumIdentificationProtocol>
  </AnalysisProtocolCollection>

  <DataCollection>
    <Inputs>
      <SourceFile location="file:///c06306_qy_RTS_3cell_2_A1.mzXML" id="SF">
        <FileFormat>
          <cvParam cvRef="MS" accession="MS:1000566" name="ISB mzXML format"/>
        </FileFormat>
      </SourceFile>
      <SearchDatabase id="2018-12-21_REVuniprot_HUMAN_contam_sorted.fasta" location="2018-12-21_REVuniprot_HUMAN_contam_sorted.fasta">
        <FileFormat>
          <cvParam cvRef="MS" accession="MS:1001348" name="FASTA format"/>
        </FileFormat>
        <DatabaseName>
          <userParam name="2018-12-21_REVuniprot_HUMAN_contam_sorted.fasta"/>
        </DatabaseName>
        <cvParam cvRef="MS" accession="MS:1001073" name="database type amino acid" value=""/>
      </SearchDatabase>
      <SpectraData location="file:///c06306_qy_RTS_3cell_2_A1.mzXML" id="SD">
        <FileFormat>
          <cvParam cvRef="MS" accession="MS:1000566" name="ISB mzXML format"/>
        </FileFormat>
        <SpectrumIDFormat>
          <cvParam cvRef="MS" accession="MS:1000776" name="scan number only nativeID format"/>
        </SpectrumIDFormat>
      </SpectraData>
    </Inputs>
    <AnalysisData>
      <SpectrumIdentificationList id="SIL" numSequencesSearched="0">
        <SpectrumIdentificationResult id="SIR_1" name="c06306_qy_RTS_3cell_2_A1.3.3" spectrumID="index=2 scan=3" spectraData_ref="SD">
          <SpectrumIdentificationItem id="SII_1" rank="1" chargeState="2" peptide_ref="PEP_1" experimentalMassToCharge="508.212461" calculatedMassToCharge="508.22495" passThreshold="false">
            <PeptideEvidenceRef peptideEvidence_ref="sp|Q8N3J3-3|CQ053_HUMAN_PEP_1"/>
            <cvParam cvRef="MS" accession="MS:1001121" name="number of matched peaks" value="5"/>
            <cvParam accession="MS:1001155" cvRef="MS" name="SEQUEST:xcorr" value="0.4764"/>
            <cvParam accession="MS:1001156" cvRef="MS" name="SEQUEST:deltacn" value="0.3059"/>
            <cvParam accession="MS:1002250" cvRef="MS" name="SEQUEST:deltacnstar" value="0.3059"/>
            <cvParam accession="MS:1002248" cvRef="MS" name="SEQUEST:spscore" value="119"/>
            <cvParam accession="MS:1002249" cvRef="MS" name="SEQUEST:sprank" value="1"/>
          </SpectrumIdentificationItem>
          <userParam name="search_id" value="1"/>
          <cvParam cvRef="MS" accession="MS:1001115" name="scan number(s)" value="3"/>
        </SpectrumIdentificationResult>
        <SpectrumIdentificationResult id="SIR_2" name="c06306_qy_RTS_3cell_2_A1.5.5" spectrumID="index=4 scan=5" spectraData_ref="SD">
          <SpectrumIdentificationItem id="SII_2" rank="1" chargeState="2" peptide_ref="PEP_2" experimentalMassToCharge="1266.85547" calculatedMassToCharge="1266.80967" passThreshold="false">
            <PeptideEvidenceRef peptideEvidence_ref="##sp|Q9H1C3|GL8D2_HUMAN_PEP_2"/>
            <cvParam cvRef="MS" accession="MS:1001121" name="number of matched peaks" value="5"/>
            <cvParam accession="MS:1001155" cvRef="MS" name="SEQUEST:xcorr" value="0.539"/>
            <cvParam accession="MS:1001156" cvRef="MS" name="SEQUEST:deltacn" value="0.379"/>
            <cvParam accession="MS:1002250" cvRef="MS" name="SEQUEST:deltacnstar" value="0.379"/>
            <cvParam accession="MS:1002248" cvRef="MS" name="SEQUEST:spscore" value="46"/>
            <cvParam accession="MS:1002249" cvRef="MS" name="SEQUEST:sprank" value="1"/>
          </SpectrumIdentificationItem>
          <userParam name="search_id" value="1"/>
          <cvParam cvRef="MS" accession="MS:1001115" name="scan number(s)" value="5"/>
        </SpectrumIdentificationResult>
      </SpectrumIdentificationList>
    </AnalysisData>
  </DataCollection>
</MzIdentML>
*/

   fprintf(fpout, "   </AdditionalSearchParams>\n");
   fprintf(fpout, "   <ModificationParams>\n");
   fprintf(fpout, "    <SearchModification residues=\"C\" massDelta=\"57.021464\" fixedMod= \"true\" >\n");
   fprintf(fpout, "     <cvParam cvRef=\"UNIMOD\" accession=\"UNIMOD:4\" name=\"Carbamidomethyl\"/>\n");
   fprintf(fpout, "     <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002504\" name=\"modification index\" value=\"0\"/>\n");
   fprintf(fpout, "    </SearchModification>\n");
   fprintf(fpout, "    <SearchModification residues=\"M\" massDelta=\"15.994915\" fixedMod= \"false\" >\n");
   fprintf(fpout, "     <cvParam cvRef=\"UNIMOD\" accession=\"UNIMOD:35\" name=\"Oxidation\"/>\n");
   fprintf(fpout, "     <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002504\" name=\"modification index\" value=\"1\"/>\n");
   fprintf(fpout, "    </SearchModification>\n");
   fprintf(fpout, "    <SearchModification residues=\".\" massDelta=\"42.010565\" fixedMod= \"false\" >\n");
   fprintf(fpout, "     <SpecificityRules>\n");
   fprintf(fpout, "      <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002057\" name=\"modification specificity protein N-term\"/>\n");
   fprintf(fpout, "     </SpecificityRules>\n");
   fprintf(fpout, "     <cvParam cvRef=\"UNIMOD\" accession=\"UNIMOD:1\" name=\"Acetyl\"/>\n");
   fprintf(fpout, "     <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002504\" name=\"modification index\" value=\"2\"/>\n");
   fprintf(fpout, "    </SearchModification>\n");
   fprintf(fpout, "   </ModificationParams>\n");
   fprintf(fpout, "   <Enzymes independent=\"false\">\n");
   fprintf(fpout, "    <Enzyme missedCleavages=\"2\" semiSpecific=\"false\" id=\"Enz1\" name=\"Trypsin\">\n");
   fprintf(fpout, "     <EnzymeName>\n");
   fprintf(fpout, "      <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001251\" name=\"Trypsin\"/>\n");
   fprintf(fpout, "     </EnzymeName>\n");
   fprintf(fpout, "    </Enzyme>\n");
   fprintf(fpout, "   </Enzymes>\n");
   fprintf(fpout, "   <FragmentTolerance>\n");
   fprintf(fpout, "    <cvParam accession=\"MS:1001412\" cvRef=\"PSI-MS\" unitCvRef=\"UO\" unitName=\"dalton\" unitAccession=\"UO:0000221\" value=\"0.02\" name=\"search tolerance plus value\" />\n");
   fprintf(fpout, "    <cvParam accession=\"MS:1001413\" cvRef=\"PSI-MS\" unitCvRef=\"UO\" unitName=\"dalton\" unitAccession=\"UO:0000221\" value=\"0.02\" name=\"search tolerance minus value\" />\n");
   fprintf(fpout, "   </FragmentTolerance>\n");
   fprintf(fpout, "   <ParentTolerance>\n");
   fprintf(fpout, "    <cvParam accession=\"MS:1001412\" cvRef=\"PSI-MS\" unitCvRef=\"UO\" unitName=\"parts per million\" unitAccession=\"UO:0000169\" value=\"10.0\" name=\"search tolerance plus value\" />\n");
   fprintf(fpout, "    <cvParam accession=\"MS:1001413\" cvRef=\"PSI-MS\" unitCvRef=\"UO\" unitName=\"parts per million\" unitAccession=\"UO:0000169\" value=\"10.0\" name=\"search tolerance minus value\" />\n");
   fprintf(fpout, "   </ParentTolerance>\n");
   fprintf(fpout, "   <Threshold>\n");
   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1001364\" name=\"peptide sequence-level global FDR\" value=\"1.0\"/>\n");
   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002350\" name=\"PSM-level global FDR\" value=\"1.0\"/>\n");
   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002567\" name=\"phosphoRS score threshold\" value=\"95.0\"/>\n");
   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002557\" name=\"D-Score threshold\" value=\"95.0\"/>\n");
   fprintf(fpout, "   </Threshold>\n");
   fprintf(fpout, "  </SpectrumIdentificationProtocol>\n");
   fprintf(fpout, "  <ProteinDetectionProtocol analysisSoftware_ref=\"ID_software\" id=\"PeptideShaker_1\">\n");
   fprintf(fpout, "   <Threshold>\n");
   fprintf(fpout, "    <cvParam cvRef=\"PSI-MS\" accession=\"MS:1002369\" name=\"protein group-level global FDR\" value=\"0.01\"/>\n");
   fprintf(fpout, "   </Threshold>\n");
   fprintf(fpout, "  </ProteinDetectionProtocol>\n");
   fprintf(fpout, " </AnalysisProtocolCollection>\n");


   fprintf(fpout, " <ModificationParams>\n");
/*
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

   WriteVariableMod(fpout, searchMgr, "variable_mod01", 0); // this writes aminoacid_modification
   WriteVariableMod(fpout, searchMgr, "variable_mod02", 0);
   WriteVariableMod(fpout, searchMgr, "variable_mod03", 0);
   WriteVariableMod(fpout, searchMgr, "variable_mod04", 0);
   WriteVariableMod(fpout, searchMgr, "variable_mod05", 0);
   WriteVariableMod(fpout, searchMgr, "variable_mod06", 0);
   WriteVariableMod(fpout, searchMgr, "variable_mod07", 0);
   WriteVariableMod(fpout, searchMgr, "variable_mod08", 0);
   WriteVariableMod(fpout, searchMgr, "variable_mod09", 0);
   WriteVariableMod(fpout, searchMgr, "variable_mod01", 1);  // this writes terminal_modification
   WriteVariableMod(fpout, searchMgr, "variable_mod02", 1);  // which has to come after aminoaicd_modification
   WriteVariableMod(fpout, searchMgr, "variable_mod03", 1);
   WriteVariableMod(fpout, searchMgr, "variable_mod04", 1);
   WriteVariableMod(fpout, searchMgr, "variable_mod05", 1);
   WriteVariableMod(fpout, searchMgr, "variable_mod06", 1);
   WriteVariableMod(fpout, searchMgr, "variable_mod07", 1);
   WriteVariableMod(fpout, searchMgr, "variable_mod08", 1);
   WriteVariableMod(fpout, searchMgr, "variable_mod09", 1);
*/
   fprintf(fpout, " </ModificationParams>\n");
}

void CometWriteMzIdentML::WriteVariableMod(FILE *fpout,
                                           CometSearchManager &searchMgr,
                                           string varModName,
                                           bool bWriteTerminalMods)
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
            if (varModsParam.szVarModChar[i]=='n' && bWriteTerminalMods)
            {
               if (varModsParam.iVarModTermDistance == 0 && (varModsParam.iWhichTerm == 1 || varModsParam.iWhichTerm == 3))
               {
                  // ignore if N-term mod on C-term
               }
               else
               {
                  double dMass = 0.0;
                  searchMgr.GetParamValue("add_Nterm_protein", dMass);

                  // print this if N-term protein variable mod or a generic N-term mod there's also N-term protein static mod
                  if (varModsParam.iWhichTerm == 0 && varModsParam.iVarModTermDistance == 0)
                  {
                     // massdiff = mod mass + h
                     fprintf(fpout, "  <terminal_modification terminus=\"N\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\" protein_terminus=\"Y\" symbol=\"%c\"/>\n",
                           varModsParam.dVarModMass,
                           varModsParam.dVarModMass
                              + dMass
                              + g_staticParams.precalcMasses.dNtermProton
                              - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment[(int)'h'],
                           cSymbol);
                  }
                  // print this if non-protein N-term variable mod
                  else
                  {
                     fprintf(fpout, "  <terminal_modification terminus=\"N\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\" protein_terminus=\"N\" symbol=\"%c\"/>\n",
                           varModsParam.dVarModMass,
                           varModsParam.dVarModMass
                              + g_staticParams.precalcMasses.dNtermProton
                              - PROTON_MASS + g_staticParams.massUtility.pdAAMassFragment[(int)'h'],
                           cSymbol);
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
                  double dMass = 0.0;
                  searchMgr.GetParamValue("add_Cterm_protein", dMass);

                  // print this if C-term protein variable mod or a generic C-term mod there's also C-term protein static mod
                  if (varModsParam.iWhichTerm == 1 && varModsParam.iVarModTermDistance == 0)
                  {
                     // massdiff = mod mass + oh
                     fprintf(fpout, "  <terminal_modification terminus=\"C\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\" protein_terminus=\"Y\" symbol=\"%c\"/>\n",
                           varModsParam.dVarModMass,
                           varModsParam.dVarModMass
                              + dMass
                              + g_staticParams.precalcMasses.dCtermOH2Proton
                              - PROTON_MASS
                              - g_staticParams.massUtility.pdAAMassFragment[(int)'h'],
                           cSymbol);
                  }
                  // print this if non-protein C-term variable mod
                  else
                  {
                     fprintf(fpout, "  <terminal_modification terminus=\"C\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\" protein_terminus=\"N\" symbol=\"%c\"/>\n",
                           varModsParam.dVarModMass,
                           varModsParam.dVarModMass
                              + g_staticParams.precalcMasses.dCtermOH2Proton
                              - PROTON_MASS
                              - g_staticParams.massUtility.pdAAMassFragment[(int)'h'],
                           cSymbol);
                  }
               }
            }
            else if (!bWriteTerminalMods && varModsParam.szVarModChar[i]!='c' && varModsParam.szVarModChar[i]!='n')
            {
               fprintf(fpout, "  <aminoacid_modification aminoacid=\"%c\" massdiff=\"%0.6f\" mass=\"%0.6f\" variable=\"Y\" %ssymbol=\"%c\"/>\n",
                     varModsParam.szVarModChar[i],
                     varModsParam.dVarModMass,
                     g_staticParams.massUtility.pdAAMassParent[(int)varModsParam.szVarModChar[i]] + varModsParam.dVarModMass,
                     (varModsParam.iBinaryMod?"binary=\"Y\" ":""),
                     cSymbol);
            }
         }
      }
   }
}


void CometWriteMzIdentML::WriteStaticMod(FILE *fpout,
                                         CometSearchManager &searchMgr,
                                         string paramName)
{
/*
   double dMass = 0.0;
   if (searchMgr.GetParamValue(paramName, dMass))
   {
      if (!isEqual(dMass, 0.0))
      {
         fprintf(fpout, "<SearchModification residues=\"%c\" massDelta=\"%0.6f\" fixedMod= \"true\" >\n", paramName[4], dMass,);
         fprintf(fpout, "  <cvParam cvRef=\"UNIMOD\" accession=\"UNIMOD:4\" name=\"Carbamidomethyl\"/>\n");
         fprintf(fpout, "</SearchModification>\n");
      }
   }
*/
}

void CometWriteMzIdentML::WriteMzIdentMLEndTags(FILE *fpout)
{
   fprintf(fpout, "</MzIdentML>\n");
   fflush(fpout);
}

void CometWriteMzIdentML::PrintResults(int iWhichQuery,
                                       bool bDecoy,
                                       FILE *fpout,
                                       FILE *fpdb)
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
      if (     strchr(pQuery->_spectrumInfoInternal.szNativeID, '&')
            || strchr(pQuery->_spectrumInfoInternal.szNativeID, '\"')
            || strchr(pQuery->_spectrumInfoInternal.szNativeID, '\'')
            || strchr(pQuery->_spectrumInfoInternal.szNativeID, '<')
            || strchr(pQuery->_spectrumInfoInternal.szNativeID, '>'))
      {
         fprintf(fpout, " spectrumNativeID=\"");
         for (i=0; i<(int)strlen(pQuery->_spectrumInfoInternal.szNativeID); i++)
         {
            switch(pQuery->_spectrumInfoInternal.szNativeID[i])
            {
               case '&':  fprintf(fpout, "&amp;");       break;
               case '\"': fprintf(fpout, "&quot;");      break;
               case '\'': fprintf(fpout, "&apos;");      break;
               case '<':  fprintf(fpout, "&lt;");        break;
               case '>':  fprintf(fpout, "&gt;");        break;
               default:   fprintf(fpout, "%c", pQuery->_spectrumInfoInternal.szNativeID[i]); break;
            }
         }
         fprintf(fpout, "\"");
      }
      else
         fprintf(fpout, " spectrumNativeID=\"%s\"", pQuery->_spectrumInfoInternal.szNativeID);
   }

   fprintf(fpout, " start_scan=\"%d\"", pQuery->_spectrumInfoInternal.iScanNumber);
   fprintf(fpout, " end_scan=\"%d\"", pQuery->_spectrumInfoInternal.iScanNumber);
   fprintf(fpout, " precursor_neutral_mass=\"%0.6f\"", pQuery->_pepMassInfo.dExpPepMass - PROTON_MASS);
   fprintf(fpout, " assumed_charge=\"%d\"", pQuery->_spectrumInfoInternal.iChargeState);
   fprintf(fpout, " index=\"%d\"", iWhichQuery+1);

   if (pQuery->_spectrumInfoInternal.dRTime > 0.0)
      fprintf(fpout, " retention_time_sec=\"%0.1f\">\n", pQuery->_spectrumInfoInternal.dRTime);
   else
      fprintf(fpout, ">\n");

   fprintf(fpout, "  <search_result>\n");

   Results *pOutput;

   if (bDecoy)
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
   for (i=0; i<iNumPrintLines; i++)
   {
      int iLen = (int)strlen(pOutput[i].szPeptide);
      if (iLen == 0)
         break;
      if (iLen < iMinLength)
         iMinLength = iLen;
   }

   iRankXcorr = 1;

   for (i=0; i<iNumPrintLines; i++)
   {
      int j;
      bool bNoDeltaCnYet = true;
      bool bDeltaCnStar = false;
      double dDeltaCn = 0.0;       // this is deltaCn between top hit and peptide in list (or next dissimilar peptide)
      double dDeltaCnStar = 0.0;   // if reported deltaCn is for dissimilar peptide, the value stored here is the
                                   // explicit deltaCn between top hit and peptide in list

      for (j=i+1; j<iNumPrintLines; j++)
      {
         if (j<g_staticParams.options.iNumStored)
         {
            // very poor way of calculating peptide similarity but it's what we have for now
            int iDiffCt = 0;

            for (int k=0; k<iMinLength; k++)
            {
               // I-L and Q-K are same for purposes here
               if (pOutput[i].szPeptide[k] != pOutput[j].szPeptide[k])
               {
                  if (!((pOutput[0].szPeptide[k] == 'K' || pOutput[0].szPeptide[k] == 'Q')
                          && (pOutput[j].szPeptide[k] == 'K' || pOutput[j].szPeptide[k] == 'Q'))
                        && !((pOutput[0].szPeptide[k] == 'I' || pOutput[0].szPeptide[k] == 'L')
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
            {
               dDeltaCnStar = 1.0 - pOutput[i+1].fXcorr/pOutput[i].fXcorr;
               if (isEqual(dDeltaCnStar, 0.0)) // note top two xcorrs could be identical so this gives a
                  dDeltaCnStar = 0.001;        // non-zero deltacnstar value to denote deltaCn is not explicit
            }
            else if (pOutput[i].fXcorr > 0.0 && pOutput[i+1].fXcorr < 0.0)
               dDeltaCnStar = 1.0;
            else
               dDeltaCnStar = 0.0;
         }
         else
            dDeltaCnStar = 0.0;

         PrintMzIdentMLSearchHit(iWhichQuery, i, iRankXcorr, bDecoy, pOutput, fpout, fpdb, dDeltaCn, dDeltaCnStar);
      }
   }

   fprintf(fpout, "  </search_result>\n");
   fprintf(fpout, " </spectrum_query>\n");
}


void CometWriteMzIdentML::PrintMzIdentMLSearchHit(int iWhichQuery,
                                                  int iWhichResult,
                                                  int iRankXcorr,
                                                  bool bDecoy,
                                                  Results *pOutput,
                                                  FILE *fpout,
                                                  FILE *fpdb,
                                                  double dDeltaCn,
                                                  double dDeltaCnStar)
{
   int  i;
   int iNTT;
   int iNMC;
   bool bPrintDecoyPrefix = false;

   Query* pQuery = g_pvQuery.at(iWhichQuery);

   CalcNTTNMC(pOutput, iWhichResult, &iNTT, &iNMC);

   char szProteinName[100];
   std::vector<ProteinEntryStruct>::iterator it;
   
   int iNumTotProteins = 0;

   if (bDecoy)
   {
      it=pOutput[iWhichResult].pWhichDecoyProtein.begin();
      iNumTotProteins = (int)pOutput[iWhichResult].pWhichDecoyProtein.size();
      bPrintDecoyPrefix = true;
   }
   else
   {
      // if not reporting separate decoys, it's possible only matches
      // in combined search are decoy entries
      if (pOutput[iWhichResult].pWhichProtein.size() > 0)
      {
         it=pOutput[iWhichResult].pWhichProtein.begin();
         iNumTotProteins = (int)(pOutput[iWhichResult].pWhichProtein.size() + pOutput[iWhichResult].pWhichDecoyProtein.size());
      }
      else  // only decoy matches in this search
      {
         it=pOutput[iWhichResult].pWhichDecoyProtein.begin();
         iNumTotProteins = (int)(pOutput[iWhichResult].pWhichDecoyProtein.size());
         bPrintDecoyPrefix = true;
      }
   }

   CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);
   ++it;

   fprintf(fpout, "   <search_hit hit_rank=\"%d\"", iRankXcorr);
   fprintf(fpout, " peptide=\"%s\"", pOutput[iWhichResult].szPeptide);
   fprintf(fpout, " peptide_prev_aa=\"%c\"", pOutput[iWhichResult].szPrevNextAA[0]);
   fprintf(fpout, " peptide_next_aa=\"%c\"", pOutput[iWhichResult].szPrevNextAA[1]);
   if (bPrintDecoyPrefix)
      fprintf(fpout, " protein=\"%s%s\"", g_staticParams.szDecoyPrefix, szProteinName);
   else
      fprintf(fpout, " protein=\"%s\"", szProteinName);
   fprintf(fpout, " num_tot_proteins=\"%d\"", iNumTotProteins);
   fprintf(fpout, " num_matched_ions=\"%d\"", pOutput[iWhichResult].iMatchedIons);
   fprintf(fpout, " tot_num_ions=\"%d\"", pOutput[iWhichResult].iTotalIons);
   fprintf(fpout, " calc_neutral_pep_mass=\"%0.6f\"", pOutput[iWhichResult].dPepMass - PROTON_MASS);
   fprintf(fpout, " massdiff=\"%0.6f\"", pQuery->_pepMassInfo.dExpPepMass - pOutput[iWhichResult].dPepMass);
   fprintf(fpout, " num_tol_term=\"%d\"", iNTT);
   fprintf(fpout, " num_missed_cleavages=\"%d\"", iNMC);
   fprintf(fpout, " num_matched_peptides=\"%lu\"", bDecoy?(pQuery->_uliNumMatchedDecoyPeptides):(pQuery->_uliNumMatchedPeptides));
   fprintf(fpout, ">\n");

   int iPrintDuplicateProteinCt = 0;

   // Print protein reference/accession.
   for (; it!=(bPrintDecoyPrefix?pOutput[iWhichResult].pWhichDecoyProtein.end():pOutput[iWhichResult].pWhichProtein.end()); ++it)
   {
      szProteinName[0]='\0';
      CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);
      if (bPrintDecoyPrefix)
         fprintf(fpout, "    <alternative_protein protein=\"%s%s\"/>\n", g_staticParams.szDecoyPrefix, szProteinName);
      else
         fprintf(fpout, "    <alternative_protein protein=\"%s\"/>\n", szProteinName);

      iPrintDuplicateProteinCt++;
      if (iPrintDuplicateProteinCt == g_staticParams.options.iMaxDuplicateProteins)
         break;
   }

   // If combined search printed out target proteins above, now print out decoy proteins if necessary
   if (!bDecoy && pOutput[iWhichResult].pWhichProtein.size() > 0 && pOutput[iWhichResult].pWhichDecoyProtein.size() > 0
         && iPrintDuplicateProteinCt < g_staticParams.options.iMaxDuplicateProteins)
   {
      for (it=pOutput[iWhichResult].pWhichDecoyProtein.begin(); it!=pOutput[iWhichResult].pWhichDecoyProtein.end(); ++it)
      {
         CometMassSpecUtils::GetProteinName(fpdb, (*it).lWhichProtein, szProteinName);
         fprintf(fpout, "    <alternative_protein protein=\"%s%s\"/>\n", g_staticParams.szDecoyPrefix, szProteinName);

         iPrintDuplicateProteinCt++;
         if (iPrintDuplicateProteinCt == g_staticParams.options.iMaxDuplicateProteins)
            break;
      }
   }

   // check if peptide is modified
   bool bModified = 0;

   if (!isEqual(g_staticParams.staticModifications.dAddNterminusPeptide, 0.0)
         || !isEqual(g_staticParams.staticModifications.dAddCterminusPeptide, 0.0))
      bModified = 1;

   if (pOutput[iWhichResult].szPrevNextAA[0]=='-' && !isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0))
      bModified = 1;
   if (pOutput[iWhichResult].szPrevNextAA[1]=='-' && !isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0))
      bModified = 1;

   if (pOutput[iWhichResult].cPeffOrigResidue != '\0' && pOutput[iWhichResult].iPeffOrigResiduePosition != -9)
      bModified = 1;

   if (!bModified)
   {
      for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
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
            || (pOutput[iWhichResult].szPrevNextAA[0]=='-'
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

         if (pOutput[iWhichResult].szPrevNextAA[0]=='-' && !isEqual(g_staticParams.staticModifications.dAddNterminusProtein, 0.0))
            dNterm += g_staticParams.staticModifications.dAddNterminusProtein;
      }

      // See if c-term mod (static and/or variable) needs to be reported
      if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0
            || !isEqual(g_staticParams.staticModifications.dAddCterminusPeptide, 0.0)
            || (pOutput[iWhichResult].szPrevNextAA[1]=='-'
               && !isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0)) )
      {
         bCterm = true;

         dCterm = g_staticParams.precalcMasses.dCtermOH2Proton - PROTON_MASS - g_staticParams.massUtility.pdAAMassFragment[(int)'h'];

         if (pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide+1] > 0)
         {
            dCterm += g_staticParams.variableModParameters.varModList[(int)pOutput[iWhichResult].piVarModSites[pOutput[iWhichResult].iLenPeptide+1]-1].dVarModMass;
            bCtermVariable = true;
         }

         if (pOutput[iWhichResult].szPrevNextAA[1]=='-' && !isEqual(g_staticParams.staticModifications.dAddCterminusProtein, 0.0))
            dCterm += g_staticParams.staticModifications.dAddCterminusProtein;
      }

      // generate modified_peptide string
      if (bNtermVariable)
         sprintf(szModPep+strlen(szModPep), "n[%0.0f]", dNterm);
      for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
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

      for (i=0; i<pOutput[iWhichResult].iLenPeptide; i++)
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
      if (pOutput[iWhichResult].cPeffOrigResidue != '\0' && pOutput[iWhichResult].iPeffOrigResiduePosition != -9)
      {
         if (pOutput[iWhichResult].iPeffOrigResiduePosition == -1)
         {
            fprintf(fpout, "     <aminoacid_substitution peptide_prev_aa=\"%c\" orig_aa=\"%c\"/>\n",
                  pOutput[iWhichResult].szPrevNextAA[0], pOutput[iWhichResult].cPeffOrigResidue);
         }
         else if (pOutput[iWhichResult].iPeffOrigResiduePosition == pOutput[iWhichResult].iLenPeptide)
         {
            fprintf(fpout, "     <aminoacid_substitution peptide_next_aa=\"%c\" orig_aa=\"%c\"/>\n",
                  pOutput[iWhichResult].szPrevNextAA[1], pOutput[iWhichResult].cPeffOrigResidue);
         }
         else
         {
            fprintf(fpout, "     <aminoacid_substitution position=\"%d\" orig_aa=\"%c\"/>\n",
                  pOutput[iWhichResult].iPeffOrigResiduePosition+1, pOutput[iWhichResult].cPeffOrigResidue);
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


void CometWriteMzIdentML::ReadInstrument(char *szManufacturer,
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


void CometWriteMzIdentML::GetVal(char *szElement,
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


void CometWriteMzIdentML::CalcNTTNMC(Results *pOutput,
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
