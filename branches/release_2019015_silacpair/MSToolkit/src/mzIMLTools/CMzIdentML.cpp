/*
Copyright 2017, Michael R. Hoopmann, Institute for Systems Biology
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

#include "mzIMLTools.h"

using namespace std;

// Static callback handlers
static void CMzIdentML_startElementCallback(void *data, const XML_Char *el, const XML_Char **attr) {
  ((CMzIdentML*)data)->startElement(el, attr);
}

static void CMzIdentML_endElementCallback(void *data, const XML_Char *el){
  ((CMzIdentML*)data)->endElement(el);
}

static void CMzIdentML_charactersCallback(void *data, const XML_Char *s, int len){
  ((CMzIdentML*)data)->characters(s, len);
}

CMzIdentML::CMzIdentML(){
  version=2;
  id = "mzIMLTools_mzid";
  name.clear();
  fileBase.clear();
  fileFull.clear();
  filePath.clear();
  parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, this);
  XML_SetElementHandler(parser, CMzIdentML_startElementCallback, CMzIdentML_endElementCallback);
  XML_SetCharacterDataHandler(parser, CMzIdentML_charactersCallback);
}

CMzIdentML::~CMzIdentML(){
  XML_ParserFree(parser);
}

void CMzIdentML::characters(const XML_Char *s, int len) {
  switch (activeEl.back()){
  case PeptideSequence:
    char strbuf[1024];
    if (len>1024) {
      cout << "character buffer overrun" << endl;
      return;
    }
    strncpy(strbuf,s,len);
    strbuf[len] = '\0';
    sequenceCollection.peptide->back().peptideSequence.text+=strbuf;
    break;
  default:
    //cout << "unprocessed characters: " << s << endl;
    break;
  }
}

void CMzIdentML::endElement(const XML_Char *el) {

  string s;

  if (isElement("AdditionalSearchParams", el)){
    if (activeEl.back() != AdditionalSearchParams) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("AnalysisCollection", el)){
    if (activeEl.back() != AnalysisCollection) cout << "Error in activeEl" << endl;
    else activeEl.pop_back(); 

  } else if (isElement("AnalysisData", el)){
      if (activeEl.back() != AnalysisData) cout << "Error in activeEl" << endl;
      else activeEl.pop_back();
    
  } else if (isElement("AnalysisProtocolCollection", el)){
    if (activeEl.back() != AnalysisProtocolCollection) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("AnalysisSoftware", el)){
    if (activeEl.back() != AnalysisSoftware) cout << "Error in activeEl" << endl;
    else activeEl.pop_back(); 
    
  } else if (isElement("AnalysisSoftwareList", el)){
    if (activeEl.back() != AnalysisSoftwareList) cout << "Error in activeEl" << endl;
    else activeEl.pop_back(); 

  } else if (isElement("DBSequence", el)){
    if (activeEl.back() != DBSequence) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("DataCollection", el)){
    if (activeEl.back() != DataCollection) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("DatabaseName", el)){
    if (activeEl.back() != DatabaseName) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Enzyme", el)){
    if (activeEl.back() != Enzyme) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("EnzymeName", el)){
    if (activeEl.back() != EnzymeName) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Enzymes", el)){
    if (activeEl.back() != Enzymes) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("FileFormat", el)){
    if (activeEl.back() != FileFormat) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();
    
  } else if (isElement("Inputs", el)){
    if (activeEl.back() != Inputs) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("MassTable", el)){
    if (activeEl.back() != MassTable) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Modification", el)){
    if (activeEl.back() != Modification) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("ModificationParams", el)){
    if (activeEl.back() != ModificationParams) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("MzIdentML", el)){
    if (activeEl.back() != MzIdentML) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Peptide", el)){
    if (activeEl.back() != Peptide) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("PeptideEvidence", el)){
    if (activeEl.back() != PeptideEvidence) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("PeptideHypothesis", el)){
    if (activeEl.back() != PeptideHypothesis) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("PeptideSequence", el)){
    if (activeEl.back() != PeptideSequence) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("ProteinAmbiguityGroup", el)){
    if (activeEl.back() != ProteinAmbiguityGroup) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("ProteinDetection", el)){
    if (activeEl.back() != ProteinDetection) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("ProteinDetectionHypothesis", el)){
    if (activeEl.back() != ProteinDetectionHypothesis) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("ProteinDetectionList", el)){
    if (activeEl.back() != ProteinDetectionList) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("ProteinDetectionProtocol", el)){
    if (activeEl.back() != ProteinDetectionProtocol) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Residue", el)){
    if (activeEl.back() != Residue) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SearchDatabase", el)){
    if (activeEl.back() != SearchDatabase) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();
  
  } else if (isElement("SearchModification", el)){
    if (activeEl.back() != SearchModification) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();
  
  } else if (isElement("SearchType",el)){
    if (activeEl.back() != SearchType) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SequenceCollection", el)){
    if (activeEl.back() != SequenceCollection) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SoftwareName", el)){
    if (activeEl.back() != SoftwareName) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SpecificityRules", el)){
    if (activeEl.back() != SpecificityRules) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SpectraData", el)){
    if (activeEl.back() != SpectraData) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SpectrumIDFormat", el)){
    if (activeEl.back() != SpectrumIDFormat) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SpectrumIdentification", el)){
    if (activeEl.back() != SpectrumIdentification) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SpectrumIdentificationItem", el)){
    if (activeEl.back() != SpectrumIdentificationItem) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SpectrumIdentificationList", el)){
    if (activeEl.back() != SpectrumIdentificationList) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SpectrumIdentificationProtocol", el)){
    if (activeEl.back() != SpectrumIdentificationProtocol) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SpectrumIdentificationResult", el)){
    if (activeEl.back() != SpectrumIdentificationResult) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Threshold", el)){
    if (activeEl.back() != Threshold) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("cvList", el)){
    if (activeEl.back() != CvList) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  }
}

void CMzIdentML::startElement(const XML_Char *el, const XML_Char **attr){

  string s;

  if (isElement("AdditionalSearchParams", el)){
    activeEl.push_back(AdditionalSearchParams);
    
  } else if (isElement("AnalysisCollection", el)){
    activeEl.push_back(AnalysisCollection);

  } else if (isElement("AnalysisData", el)){
    activeEl.push_back(AnalysisData);

  } else if (isElement("AnalysisProtocolCollection", el)){
    activeEl.push_back(AnalysisProtocolCollection);
  
  } else if (isElement("AnalysisSoftware", el)){
    activeEl.push_back(AnalysisSoftware); 
    CAnalysisSoftware as;
    as.id = getAttrValue("id", attr);
    as.name = getAttrValue("name", attr);
    as.version = getAttrValue("version", attr);
    as.uri = getAttrValue("uri", attr);
    analysisSoftwareList.addAnalysisSoftware(as);
    
  } else if (isElement("AnalysisSoftwareList", el)){
    activeEl.push_back(AnalysisSoftwareList);

  } else if (isElement("DBSequence", el)){
    activeEl.push_back(DBSequence);
    CDBSequence db;
    db.id = getAttrValue("id", attr);
    db.accession = getAttrValue("accession", attr);
    db.name = getAttrValue("name", attr);
    db.searchDatabaseRef = getAttrValue("searchDatabase_ref", attr);
    sequenceCollection.addDBSequence(db);

  } else if (isElement("DataCollection", el)){
    activeEl.push_back(DataCollection);

  } else if (isElement("DatabaseName", el)){
    activeEl.push_back(DatabaseName);

  } else if(isElement("Enzyme",el)){
    activeEl.push_back(Enzyme);
    CEnzyme c;
    c.cTermGain=getAttrValue("cTermGain",attr);
    c.id=getAttrValue("id",attr);
    c.minDistance=atoi(getAttrValue("minDistance",attr));
    s = getAttrValue("missedCleavages", attr);
    if(s.size()>0) c.missedCleavages=atoi(s.c_str());
    c.nTermGain=getAttrValue("nTermGain",attr);
    c.name=getAttrValue("name",attr);
    s = getAttrValue("semiSpecific", attr);
    if (s.compare("true") == 0) c.semiSpecific = true;
    else c.semiSpecific = false;
    if (analysisProtocolCollection.spectrumIdentificationProtocol->back().enzymes.enzyme->at(0).id.compare("null") == 0){
      analysisProtocolCollection.spectrumIdentificationProtocol->back().enzymes.enzyme->clear();
    }
    analysisProtocolCollection.spectrumIdentificationProtocol->back().enzymes.enzyme->push_back(c);

  } else if(isElement("EnzymeName",el)){
    activeEl.push_back(EnzymeName);
    CEnzymeName c;
    analysisProtocolCollection.spectrumIdentificationProtocol->back().enzymes.enzyme->back().enzymeName=c;

  } else if (isElement("Enzymes",el)){
    activeEl.push_back(Enzymes);
    s = getAttrValue("isDecoy", attr);
    if (s.compare("true") == 0) analysisProtocolCollection.spectrumIdentificationProtocol->back().enzymes.independent = true;
    else analysisProtocolCollection.spectrumIdentificationProtocol->back().enzymes.independent = false;

  } else if (isElement("FileFormat", el)){
    activeEl.push_back(FileFormat);

  } else if (isElement("Inputs", el)){
    activeEl.push_back(Inputs);

  } else if (isElement("InputSpectra",el)){
    sInputSpectra is;
    is.spectraDataRef = getAttrValue("spectraData_ref", attr);
    if (analysisCollection.spectrumIdentification->back().inputSpectra->at(0).spectraDataRef.compare("null") == 0){
      analysisCollection.spectrumIdentification->back().inputSpectra->clear();
    }
    analysisCollection.spectrumIdentification->back().inputSpectra->push_back(is);

  } else if (isElement("InputSpectrumIdentifications", el)){
    sInputSpectrumIdentifications isi;
    isi.spectrumIdentificationListRef = getAttrValue("spectrumIdentificationList_ref", attr);
    analysisCollection.proteinDetection.inputSpectrumidentifications->push_back(isi);

  } else if (isElement("MassTable", el)){
    activeEl.push_back(MassTable);
    CMassTable m;
    m.id=getAttrValue("id",attr);
    m.name=getAttrValue("name",attr);
    s = getAttrValue("msLevel", attr);
    for(size_t i=0;i<s.size();i++){
      if(s[i]==' ') continue;
      string tmp;
      tmp+=s[i];
      m.msLevel->push_back(atoi(tmp.c_str()));
    }
    analysisProtocolCollection.spectrumIdentificationProtocol->back().massTable->push_back(m);

  } else if (isElement("Modification", el)){
    activeEl.push_back(Modification);
    CModification m;
    m.residues = getAttrValue("residues", attr);
    s = getAttrValue("location", attr);
    if (s.size() > 0)  m.location = atoi(&s[0]);
    s = getAttrValue("monoisotopicMassDelta", attr);
    if (s.size() > 0)  m.monoisotopicMassDelta = atof(&s[0]);
    s = getAttrValue("avgMassDelta", attr);
    if (s.size() > 0)  m.avgMassDelta = atof(&s[0]);
    sequenceCollection.peptide->back().modification->push_back(m);

  } else if (isElement("ModificationParams", el)){
    activeEl.push_back(ModificationParams);

  } else if (isElement("MzIdentML", el)){
    activeEl.push_back(MzIdentML);
    s = getAttrValue("version", attr);
    if (s.size() == 0)  killRead=true;
    creationDate.parseDateTime(getAttrValue("creationDate",attr));

  } else if (isElement("Peptide", el)){
    activeEl.push_back(Peptide);
    CPeptide p;
    p.id = getAttrValue("id", attr);
    p.name = getAttrValue("name", attr);
    sequenceCollection.addPeptide(p);

  } else if (isElement("PeptideEvidence", el)){
    activeEl.push_back(PeptideEvidence);
    CPeptideEvidence pe;
    pe.id = getAttrValue("id", attr);
    pe.name = getAttrValue("name", attr);
    pe.dbSequenceRef = getAttrValue("dBSequence_ref", attr);
    pe.peptideRef = getAttrValue("peptide_ref", attr);
    s = getAttrValue("isDecoy",attr);
    if (s.compare("true")==0) pe.isDecoy=true;
    else pe.isDecoy=false;
    s = getAttrValue("pre", attr);
    if (s.size()>0) pe.pre = s[0];
    s = getAttrValue("post", attr);
    if (s.size()>0) pe.post = s[0];
    sequenceCollection.addPeptideEvidence(pe);

  } else if (isElement("PeptideEvidenceRef", el)){
    sPeptideEvidenceRef per;
    per.peptideEvidenceRef = getAttrValue("peptideEvidence_ref", attr);
    dataCollection.analysisData.spectrumIdentificationList->back().spectrumIdentificationResult->back().spectrumIdentificationItem->back().addPeptideEvidenceRef(per);

  } else if (isElement("PeptideHypothesis", el)){
    activeEl.push_back(PeptideHypothesis);
    CPeptideHypothesis ph;
    ph.peptideEvidenceRef = getAttrValue("peptideEvidence_ref",attr);
    if (dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().proteinDetectionHypothesis->back().peptideHypothesis->at(0).peptideEvidenceRef.compare("null") == 0){
      dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().proteinDetectionHypothesis->back().peptideHypothesis->clear();
    }
    dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().proteinDetectionHypothesis->back().peptideHypothesis->push_back(ph);

  } else if (isElement("PeptideSequence", el)){
    activeEl.push_back(PeptideSequence);

  } else if (isElement("ProteinAmbiguityGroup", el)){
    activeEl.push_back(ProteinAmbiguityGroup);
    CProteinAmbiguityGroup pag;
    pag.id = getAttrValue("id", attr);
    dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->push_back(pag);

  } else if (isElement("ProteinDetection", el)){
    activeEl.push_back(ProteinDetection);
    analysisCollection.proteinDetection.id = getAttrValue("id", attr);
    analysisCollection.proteinDetection.proteinDetectionListRef = getAttrValue("proteinDetctionList_ref", attr);;
    analysisCollection.proteinDetection.proteinDetectionProtocolRef = getAttrValue("proteinDetectionProtocol_ref", attr);
    analysisCollection.proteinDetection.inputSpectrumidentifications->clear();

  } else if (isElement("ProteinDetectionHypothesis", el)){
    activeEl.push_back(ProteinDetectionHypothesis);
    CProteinDetectionHypothesis pdh;
    pdh.id = getAttrValue("id", attr);
    pdh.dbSequenceRef = getAttrValue("dBSequence_ref", attr);
    s = getAttrValue("passThreshold",attr);
    if(s.compare("true") == 0) pdh.passThreshold = true;
    else pdh.passThreshold = false;
    if (dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().proteinDetectionHypothesis->at(0).id.compare("null") == 0){
      dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().proteinDetectionHypothesis->clear();
    }
    dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().proteinDetectionHypothesis->push_back(pdh);

  } else if (isElement("ProteinDetectionList", el)){
    activeEl.push_back(ProteinDetectionList);
    dataCollection.analysisData.proteinDetectionList.id = getAttrValue("id", attr);
    dataCollection.analysisData.proteinDetectionList.name = getAttrValue("name", attr);

  } else if (isElement("ProteinDetectionProtocol", el)){
    activeEl.push_back(ProteinDetectionProtocol);
    CProteinDetectionProtocol pdp;
    pdp.id = getAttrValue("id", attr);
    pdp.analysisSoftwareRef = getAttrValue("analysisSoftware_ref", attr);
    analysisProtocolCollection.proteinDetectionProtocol = pdp;

  } else if (isElement("Residue", el)){
    activeEl.push_back(Residue);
    CResidue m;
    m.code = getAttrValue("code", attr)[0];
    m.mass = (float)atof(getAttrValue("mass", attr));
    analysisProtocolCollection.spectrumIdentificationProtocol->back().massTable->back().residue->push_back(m);

  } else if (isElement("SearchDatabase", el)){
    activeEl.push_back(SearchDatabase);
    CSearchDatabase db;
    db.id = getAttrValue("id", attr);
    db.location = getAttrValue("location", attr);
    dataCollection.inputs.searchDatabase->push_back(db);

  } else if (isElement("SearchDatabaseRef", el)){
    sSearchDatabaseRef sdr;
    sdr.searchDatabaseRef = getAttrValue("searchDatabase_ref", attr);
    if (analysisCollection.spectrumIdentification->back().searchDatabaseRef->at(0).searchDatabaseRef.compare("null")==0){
      analysisCollection.spectrumIdentification->back().searchDatabaseRef->clear();
    }
    analysisCollection.spectrumIdentification->back().searchDatabaseRef->push_back(sdr);

  } else if (isElement("SearchModification", el)){
    activeEl.push_back(SearchModification);
    CSearchModification sm;
    sm.residues = getAttrValue("residues", attr);
    s = getAttrValue("massDelta", attr);
    sm.massDelta = atof(&s[0]);
    s = getAttrValue("fixedMod", attr);
    if (s.compare("true") == 0) sm.fixedMod = true;
    else sm.fixedMod = false;
    analysisProtocolCollection.spectrumIdentificationProtocol->back().modificationParams.addSearchModification(sm);

  } else if (isElement("SearchType", el)){
    activeEl.push_back(SearchType);

  } else if (isElement("SequenceCollection", el)){
    activeEl.push_back(SequenceCollection);

  } else if (isElement("SoftwareName", el)){
    activeEl.push_back(SoftwareName);

  } else if (isElement("SpecificityRules", el)){
    activeEl.push_back(SpecificityRules);

  } else if (isElement("SpectraData", el)){
    activeEl.push_back(SpectraData);
    CSpectraData sd;
    sd.id = getAttrValue("id", attr);
    sd.location = getAttrValue("location", attr);
    dataCollection.inputs.addSpectraData(sd);

  } else if (isElement("SpectrumIDFormat", el)){
    activeEl.push_back(SpectrumIDFormat);

  } else if (isElement("SpectrumIdentification", el)){
    activeEl.push_back(SpectrumIdentification);
    CSpectrumIdentification si;
    si.id = getAttrValue("id", attr);
    si.spectrumIdentificationListRef = getAttrValue("spectrumIdentificationList_ref", attr);
    si.spectrumIdentificationProtocolRef = getAttrValue("spectrumIdentificationProtocol_ref", attr);
    analysisCollection.addSpectrumIdentification(si);

  } else if (isElement("SpectrumIdentificationItem", el)){
    activeEl.push_back(SpectrumIdentificationItem);
    CSpectrumIdentificationItem sii;
    sii.id = getAttrValue("id", attr);
    sii.peptideRef = getAttrValue("peptide_ref", attr);
    s = getAttrValue("chargeState", attr);
    sii.chargeState = atoi(&s[0]);
    s = getAttrValue("experimentalMassToCharge", attr);
    sii.experimentalMassToCharge = atof(&s[0]);
    s = getAttrValue("rank", attr);
    sii.rank = atoi(&s[0]);
    s = getAttrValue("calculatedMassToCharge", attr);
    if(s.size()>0) sii.calculatedMassToCharge = atof(&s[0]);
    s = getAttrValue("passThreshold", attr);
    if (s.compare("true") == 0) sii.passThreshold = true;
    else sii.passThreshold = false;
    dataCollection.analysisData.spectrumIdentificationList->back().spectrumIdentificationResult->back().addSpectrumIdentificationItem(sii);

  } else if (isElement("SpectrumIdentificationItemRef", el)){
    sSpectrumIdentificationItemRef siir;
    siir.text = getAttrValue("spectrumIdentificationItem_ref",attr);
    if (dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().proteinDetectionHypothesis->back().peptideHypothesis->back().spectrumIdentificationItemRef->at(0).text.compare("null") == 0){
      dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().proteinDetectionHypothesis->back().peptideHypothesis->back().spectrumIdentificationItemRef->clear();
    }
    dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().proteinDetectionHypothesis->back().peptideHypothesis->back().spectrumIdentificationItemRef->push_back(siir);

  } else if (isElement("SpectrumIdentificationList", el)){
    activeEl.push_back(SpectrumIdentificationList);
    CSpectrumIdentificationList sil;
    sil.id = getAttrValue("id", attr);
    dataCollection.analysisData.addSpectrumIdentificationList(sil);

  } else if (isElement("SpectrumIdentificationProtocol", el)){
    activeEl.push_back(SpectrumIdentificationProtocol);
    CSpectrumIdentificationProtocol sip;
    sip.id = getAttrValue("id", attr);
    sip.analysisSoftwareRef = getAttrValue("analysisSoftware_ref", attr);
    analysisProtocolCollection.addSpectrumIdentificationProtocol(sip);

  } else if (isElement("SpectrumIdentificationResult", el)){
    activeEl.push_back(SpectrumIdentificationResult);
    CSpectrumIdentificationResult sir;
    sir.id = getAttrValue("id", attr);
    sir.spectrumID = getAttrValue("spectrumID", attr);
    sir.spectraDataRef = getAttrValue("spectraData_ref", attr);
    dataCollection.analysisData.spectrumIdentificationList->back().addSpectrumIdentificationResult(sir);

  } else if (isElement("Threshold", el)){
    activeEl.push_back(Threshold);
    //clear defaults
    if (activeEl.back() == SpectrumIdentificationProtocol){
      analysisProtocolCollection.spectrumIdentificationProtocol->back().threshold.cvParam->clear();
      analysisProtocolCollection.spectrumIdentificationProtocol->back().threshold.userParam->clear();
    } else if (activeEl.back() == ProteinDetectionProtocol){
      analysisProtocolCollection.proteinDetectionProtocol.threshold.cvParam->clear();
      analysisProtocolCollection.proteinDetectionProtocol.threshold.userParam->clear();
    }

  } else if (isElement("cv", el)){
    sCV cv;
    cv.fullName = getAttrValue("fullName", attr);
    cv.id = getAttrValue("id", attr);
    cv.uri = getAttrValue("uri", attr);
    cv.version = getAttrValue("version", attr);
    cvList.addCV(cv);

  } else if (isElement("cvList", el)){
    activeEl.push_back(CvList);

  } else if (isElement("cvParam", el)){
    sCvParam cv;
    cv.accession = getAttrValue("accession", attr);
    cv.name = getAttrValue("name", attr);
    cv.cvRef = getAttrValue("cvRef", attr);
    cv.value = getAttrValue("value", attr);
    cv.unitAccession = getAttrValue("unitAccession", attr);
    cv.unitCvRef = getAttrValue("unitCvRef", attr);
    cv.unitName = getAttrValue("unitName", attr);
    processCvParam(cv);

  } else if (isElement("userParam", el)){
    sUserParam u;
    u.type = getAttrValue("type", attr);
    u.name = getAttrValue("name", attr);
    u.value = getAttrValue("value", attr);
    processUserParam(u);

  }
}


//Adds the analysis software information and returns a reference id.
string CMzIdentML::addAnalysisSoftware(string software, string version){
  //clear any placeholders
  if (analysisSoftwareList[0].id.compare("null") == 0) analysisSoftwareList.analysisSoftware->clear();

  CAnalysisSoftware as;
  as.name=software;
  as.version=version;

  size_t i;
  for (i = 0; i < analysisSoftwareList.analysisSoftware->size(); i++){
    if (analysisSoftwareList.analysisSoftware->at(i) == as) return analysisSoftwareList.analysisSoftware->at(i).id;
  }

  char cID[32];
  sprintf(cID, "AS%zu", analysisSoftwareList.analysisSoftware->size());
  as.id = cID;

  if (software.compare("Comet") == 0){
    as.softwareName.cvParam.accession = "MS:1002251";
    as.softwareName.cvParam.cvRef = "PSI-MS";
    as.softwareName.cvParam.name = "Comet";
  } else if (software.compare("database_refresh") == 0){
    as.softwareName.cvParam.accession = "MS:1002286";
    as.softwareName.cvParam.cvRef = "PSI-MS";
    as.softwareName.cvParam.name = "Trans-Proteomic Pipeline software";
  } else if (software.compare("decoy") == 0){
    as.softwareName.cvParam.accession = "MS:1002286";
    as.softwareName.cvParam.cvRef = "PSI-MS";
    as.softwareName.cvParam.name = "Trans-Proteomic Pipeline software";
  } else if (software.compare("interact") == 0){
    as.softwareName.cvParam.accession = "MS:1002286";
    as.softwareName.cvParam.cvRef = "PSI-MS";
    as.softwareName.cvParam.name = "Trans-Proteomic Pipeline software";
  } else if (software.compare("interprophet") == 0){
    as.softwareName.cvParam.accession = "MS:1002288";
    as.softwareName.cvParam.cvRef = "PSI-MS";
    as.softwareName.cvParam.name = "iProphet";
  } else if (software.compare("libra") == 0){
    as.softwareName.cvParam.accession = "MS:1002291";
    as.softwareName.cvParam.cvRef = "PSI-MS";
    as.softwareName.cvParam.name = "Libra";
  } else if (software.compare("peptideprophet") == 0){
    as.softwareName.cvParam.accession = "MS:1002287";
    as.softwareName.cvParam.cvRef = "PSI-MS";
    as.softwareName.cvParam.name = "PeptideProphet";
  } else if (software.compare("proteinprophet") == 0){
    as.softwareName.cvParam.accession = "MS:1002289";
    as.softwareName.cvParam.cvRef = "PSI-MS";
    as.softwareName.cvParam.name = "ProteinProphet";
  } else if (software.compare("X! Tandem (k-score)") == 0){
    as.softwareName.cvParam.accession = "MS:1001476";
    as.softwareName.cvParam.cvRef = "PSI-MS";
    as.softwareName.cvParam.name = "X!Tandem";
  } else {
    as.softwareName.cvParam.accession = "MS:1001456";
    as.softwareName.cvParam.cvRef = "PSI-MS";
    as.softwareName.cvParam.name = "analysis software";
  }

  //TODO: add all sorts of optional information

  analysisSoftwareList.analysisSoftware->push_back(as);
  return as.id;
}

//Adds the database file information and returns a reference id.
string CMzIdentML::addDatabase(string s){

  //TODO: add all sorts of optional information

  return dataCollection.inputs.addSearchDatabase(s);
}

string CMzIdentML::addDBSequence(string acc, string sdbRef, string desc){

  CDBSequence dbs;
  dbs.accession=acc;
  dbs.searchDatabaseRef=sdbRef;

  //TODO: add optional information
  if (desc.size() > 0){
    sCvParam cv;
    cv.cvRef = "PSI-MS";
    cv.accession = "MS:1001088";
    cv.name = "protein description";
    cv.value = desc;
    dbs.cvParam->push_back(cv);
  }

  string idRef = sequenceCollection.addDBSequence(dbs);
  return idRef;
}

string CMzIdentML::addPeptide(string seq, vector<CModification>& mods){
  CPeptide p;
  p.peptideSequence.text=seq;
  
  size_t i;
  for (i = 0; i < mods.size(); i++){
    p.modification->push_back(mods[i]);
  }

  string idRef = sequenceCollection.addPeptide(p);
  return idRef;
}

sPeptideEvidenceRef CMzIdentML::addPeptideEvidence(string dbRef, string pepRef, int start, int end, char pre, char post, bool isDecoy){
  CPeptideEvidence p;
  p.dbSequenceRef = dbRef;
  p.peptideRef = pepRef;

  //TODO: add optional information
  p.start=start;
  p.end=end;
  p.pre=pre;
  p.post=post;
  p.isDecoy=isDecoy;

  //a common disallowed character in pepXML. if there are more of these characters,
  //a more elegant elegant solution should be found.
  if(p.pre=='*') p.pre='?';
  if(p.post=='*') p.post='?';

  sPeptideEvidenceRef peRef = sequenceCollection.addPeptideEvidence(p);
  return peRef;
}

CProteinAmbiguityGroup* CMzIdentML::addProteinAmbiguityGroup(){
  CProteinAmbiguityGroup p;
  char str[32];
  
  sprintf(str, "PAG_%zu", dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->size());
  p.id=str;

  dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->push_back(p);
  return &dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back();
}

//Adds the spectrum data file information and returns a reference id.
//FileFormat is determined by evaluating the extension
string CMzIdentML::addSpectraData(string fn){
  return dataCollection.inputs.addSpectraData(fn);
}

CSpectrumIdentification* CMzIdentML::addSpectrumIdentification(string& spectraDataRef, string& searchDatabaseRef){
  //remove any placeholder
  if (analysisCollection.spectrumIdentification->at(0).id.compare("null") == 0) analysisCollection.spectrumIdentification->clear();

  CSpectrumIdentification si;
  si.inputSpectra->at(0).spectraDataRef=spectraDataRef;
  si.searchDatabaseRef->at(0).searchDatabaseRef=searchDatabaseRef;

  size_t i;
  for (i = 0; i < analysisCollection.spectrumIdentification->size(); i++){
    if (analysisCollection.spectrumIdentification->at(i).compare(si)) return &analysisCollection.spectrumIdentification->at(i);
  }

  char cID[32];
  sprintf(cID, "SI%zu", analysisCollection.spectrumIdentification->size());
  si.id=cID;
  si.spectrumIdentificationListRef = dataCollection.analysisData.addSpectrumIdentificationList();
  si.spectrumIdentificationProtocolRef = analysisProtocolCollection.addSpectrumIdentificationProtocol(string("null"));

  analysisCollection.spectrumIdentification->push_back(si);
  return &analysisCollection.spectrumIdentification->at(analysisCollection.spectrumIdentification->size()-1);

}

bool CMzIdentML::addXLPeptides(string seq1, vector<CModification>& mods1, string& ref1, string seq2, vector<CModification>& mods2, string& ref2, string& value){
  size_t i;
  CPeptide p1,p2;
  string ID1;
  string ID2;
  string xID;
  char str[32];

  //make the first peptide object
  p1.peptideSequence.text = seq1; 
  ID1=seq1;
  for (i = 0; i < mods1.size(); i++) {
    p1.modification->push_back(mods1[i]);
    sprintf(str,"[%d,%.2lf]",mods1[i].location, mods1[i].monoisotopicMassDelta);
    ID1+=str;
  }
  ID2 = seq2;
  p2.peptideSequence.text = seq2;
  for (i = 0; i < mods2.size(); i++) {
    p2.modification->push_back(mods2[i]);
    sprintf(str, "[%d,%.2lf]", mods2[i].location, mods2[i].monoisotopicMassDelta);
    ID2 += str;
  }

  //put the objects in the right order
  if(seq1.size()>seq2.size()){
    xID=ID1+ID2;
    return sequenceCollection.addXLPeptides(xID, p1, p2, ref1, ref2, value);
  } else if(seq1.size()==seq2.size() && seq1.compare(seq2)<=0){
    xID = ID1 + ID2;
    return sequenceCollection.addXLPeptides(xID, p1, p2, ref1, ref2, value);
  } else {
    xID = ID2 + ID1;
    return sequenceCollection.addXLPeptides(xID, p2, p1, ref2, ref1, value);
  }

}

void CMzIdentML::consolidateSpectrumIdentificationProtocol(){
  size_t i,j,k;
  string oldRef;
  string newRef;
  for (i = 0; i < analysisProtocolCollection.spectrumIdentificationProtocol->size(); i++){
    for (j = i + 1; j < analysisProtocolCollection.spectrumIdentificationProtocol->size(); j++){
      if (analysisProtocolCollection.spectrumIdentificationProtocol->at(i) == analysisProtocolCollection.spectrumIdentificationProtocol->at(j)){
        newRef = analysisProtocolCollection.spectrumIdentificationProtocol->at(i).id;
        oldRef = analysisProtocolCollection.spectrumIdentificationProtocol->at(j).id;
        for (k = 0; k < analysisCollection.spectrumIdentification->size(); k++){
          if (analysisCollection.spectrumIdentification->at(k).spectrumIdentificationProtocolRef.compare(oldRef) == 0) {
            analysisCollection.spectrumIdentification->at(k).spectrumIdentificationProtocolRef = newRef;
          }
        }
        analysisProtocolCollection.spectrumIdentificationProtocol->erase(analysisProtocolCollection.spectrumIdentificationProtocol->begin()+j);
        j--;
      }
    }
  }
}

CDBSequence CMzIdentML::getDBSequence(string& dBSequence_ref){
  return sequenceCollection.getDBSequence(dBSequence_ref);
}

CDBSequence CMzIdentML::getDBSequenceByAcc(string acc){
  return *sequenceCollection.getDBSequenceByAcc(acc);
}

CPeptide CMzIdentML::getPeptide(string peptide_ref){
  return *sequenceCollection.getPeptide(peptide_ref);
}

CPeptideEvidence CMzIdentML::getPeptideEvidence(string& peptideEvidence_ref){
  return sequenceCollection.getPeptideEvidence(peptideEvidence_ref);
}

CPSM CMzIdentML::getPSM(int index, int rank){
  CPSM psm;

  //immediately reduce rank to zero-based
  rank--;

  size_t i,j;

  //get the index local to the spectrumIdentification List
  for (i = 0; i < dataCollection.analysisData.spectrumIdentificationList->size(); i++){
    if(index<(int)dataCollection.analysisData.spectrumIdentificationList->at(i).spectrumIdentificationResult->size()) break;
    else index -= (int)dataCollection.analysisData.spectrumIdentificationList->at(i).spectrumIdentificationResult->size();
  }
  if (i == dataCollection.analysisData.spectrumIdentificationList->size()) return psm; //return the empty psm because index is out of range
  
  //return empty psm if rank is out of range
  if (rank >= (int)dataCollection.analysisData.spectrumIdentificationList->at(i).spectrumIdentificationResult->at(index).spectrumIdentificationItem->size()) return psm;

  //start gathering data
  CSpectrumIdentificationResult* sir;
  CSpectrumIdentificationItem* sii;
  sir = &dataCollection.analysisData.spectrumIdentificationList->at(i).spectrumIdentificationResult->at(index);
  sii = &sir->spectrumIdentificationItem->at(rank);

  psm.charge=sii->chargeState;
  psm.mzObs=sii->experimentalMassToCharge;

  psm.scanInfo.scanID=sir->spectrumID;
  for (i = 0; i < dataCollection.inputs.spectraData->size(); i++){
    if (dataCollection.inputs.spectraData->at(i).id.compare(sir->spectraDataRef) == 0){
      psm.scanInfo.fileName = dataCollection.inputs.spectraData->at(i).location;
      break;
    }
  }

  for (i = 0; i < sir->cvParam->size(); i++){
    if (sir->cvParam->at(i).accession.compare("MS:1000894") == 0){
      if (sir->cvParam->at(i).unitName.compare("second") == 0){
        psm.scanInfo.rTimeSec = atof(&sir->cvParam->at(i).value[0]);
        psm.scanInfo.rTimeMin = psm.scanInfo.rTimeSec/60;
        break;
      } else if (sir->cvParam->at(i).unitName.compare("minute") == 0){
        psm.scanInfo.rTimeMin = atof(&sir->cvParam->at(i).value[0]);
        psm.scanInfo.rTimeSec = psm.scanInfo.rTimeMin * 60;
        break;
      }
    }
  }

  CPeptide* pep=sequenceCollection.getPeptide(sii->peptideRef);
  psm.sequence=pep->peptideSequence.text;

  char str[32];
  sPSMMod mod;
  vector<sPSMMod> mods;
  for (i = 0; i < pep->modification->size(); i++){
    mod.mass = pep->modification->at(i).monoisotopicMassDelta;
    mod.pos = pep->modification->at(i).location;
    mod.residue = pep->modification->at(i).residues[0]; //potential for error here
    mods.push_back(mod);
  }
  psm.addMods(mods);
  for (i = 0; i < mods.size(); i++){
    if (mods[i].pos==0) {
      sprintf(str, "n[%.2lf]",mods[i].mass);
      psm.sequenceMod+=str;
      break;
    }
  }
  for (i = 0; i < psm.sequence.size(); i++){
    psm.sequenceMod += psm.sequence[i];
    for (j = 0; j < mods.size(); j++){
      if (mods[j].pos - 1 == i){
        sprintf(str, "[%.2lf]", mods[j].mass);
        psm.sequenceMod+=str;
        break;
      }
    }
  }
  for (i = 0; i < mods.size(); i++){
    if (mods[i].pos > psm.sequence.size()) {
      sprintf(str, "c[%.2lf]", mods[i].mass);
      psm.sequenceMod += str;
      break;
    }
  }

  vector<string> proteins;
  if (sii->peptideEvidenceRef->size()>20) {
    cout << "wtf: " << sii->peptideEvidenceRef->size() << " " << sii->id << " " << index << endl;
    cout << "\t" << psm.sequence << "\t" << psm.sequenceMod << "\t" << sii->peptideRef << endl;
  }
  for (i = 0; i < sii->peptideEvidenceRef->size(); i++){
    proteins.push_back(sequenceCollection.getProtein(sii->peptideEvidenceRef->at(i)));
    //if (index == 4642) cout << sii->peptideEvidenceRef->at(i).peptideEvidenceRef << " " << sequenceCollection.getProtein(sii->peptideEvidenceRef->at(i)) << endl;
  }
  psm.addProteins(proteins);

  sPSMScore score;
  vector<sPSMScore> scores;
  for (i = 0; i < sii->cvParam->size(); i++){
    score.name = sii->cvParam->at(i).name;
    score.value = atof(&sii->cvParam->at(i).value[0]);
    scores.push_back(score);
  }
  psm.addScores(scores);

  sir=NULL;
  sii=NULL;
  pep=NULL; 

  return psm;

}

int CMzIdentML::getPSMCount(){
  size_t count=0;
  for (size_t i = 0; i < dataCollection.analysisData.spectrumIdentificationList->size(); i++){
    count += dataCollection.analysisData.spectrumIdentificationList->at(i).spectrumIdentificationResult->size();
  }
  return (int)count;
}

CSpectraData CMzIdentML::getSpectraData(string& spectraData_ref){
  size_t i;
  for (i = 0; i < dataCollection.inputs.spectraData->size(); i++){
    if (dataCollection.inputs.spectraData->at(i).id.compare(spectraData_ref) == 0){
      return dataCollection.inputs.spectraData->at(i);
    }
  }
  CSpectraData blank;
  return blank;
}

//Gets the pointer to the requested list, or returns null if bad reference is requested.
CSpectrumIdentificationList* CMzIdentML::getSpectrumIdentificationList(string& spectrumIdentificationList_ref){
  size_t i;
  for (i = 0; i < dataCollection.analysisData.spectrumIdentificationList->size(); i++){
    if (dataCollection.analysisData.spectrumIdentificationList->at(i).id.compare(spectrumIdentificationList_ref) == 0){
      return &dataCollection.analysisData.spectrumIdentificationList->at(i);
    }
  }
  return NULL;
}

//Gets the pointer to the requested protocol, or returns null if bad reference is requested.
CSpectrumIdentificationProtocol* CMzIdentML::getSpectrumIdentificationProtocol(string& spectrumIdentificationProtocol_ref){
  size_t i;
  for (i = 0; i < analysisProtocolCollection.spectrumIdentificationProtocol->size(); i++){
    if (analysisProtocolCollection.spectrumIdentificationProtocol->at(i).id.compare(spectrumIdentificationProtocol_ref) == 0){
      return &analysisProtocolCollection.spectrumIdentificationProtocol->at(i);
    }
  }
  return NULL;
}

int CMzIdentML::getVersion(){
  return version;
}

void CMzIdentML::processCvParam(sCvParam& cv){
  mzidElement e = AnalysisCollection;
  if(activeEl.size()>1) e=activeEl.at(activeEl.size()-2);
  switch (activeEl.back()){
  case AdditionalSearchParams:
    if (analysisProtocolCollection.spectrumIdentificationProtocol->back().additionalSearchParams.cvParam->at(0).accession.compare("null")==0) {
      analysisProtocolCollection.spectrumIdentificationProtocol->back().additionalSearchParams.cvParam->clear();
    }
    analysisProtocolCollection.spectrumIdentificationProtocol->back().additionalSearchParams.cvParam->push_back(cv);
    break;
  case DBSequence:
    sequenceCollection.dbSequence->back().cvParam->push_back(cv);
    break;
  case EnzymeName:
    if (analysisProtocolCollection.spectrumIdentificationProtocol->back().enzymes.enzyme->back().enzymeName.cvParam->at(0).accession.compare("null")==0){
      analysisProtocolCollection.spectrumIdentificationProtocol->back().enzymes.enzyme->back().enzymeName.cvParam->clear();
    }
    analysisProtocolCollection.spectrumIdentificationProtocol->back().enzymes.enzyme->back().enzymeName.cvParam->push_back(cv);
    break;
  case FileFormat:
    if (e == SpectraData){
      dataCollection.inputs.spectraData->back().fileFormat.cvParam=cv;
    } else if (e == SearchDatabase){
      dataCollection.inputs.searchDatabase->back().fileFormat.cvParam=cv;
    }
    break;
  case MassTable:
    analysisProtocolCollection.spectrumIdentificationProtocol->back().massTable->back().cvParam->push_back(cv);
    break;
  case Modification:
    sequenceCollection.peptide->back().modification->back().cvParam->push_back(cv);
    break;
  case ProteinAmbiguityGroup:
    dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().cvParam->push_back(cv);
    break;
  case ProteinDetectionHypothesis:
    dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().proteinDetectionHypothesis->back().cvParam->push_back(cv);
    break;
  case ProteinDetectionList:
    dataCollection.analysisData.proteinDetectionList.cvParam->push_back(cv);
    break;
  case SearchDatabase:
    dataCollection.inputs.searchDatabase->back().cvParam->push_back(cv);
    break;
  case SearchModification:
    if (analysisProtocolCollection.spectrumIdentificationProtocol->back().modificationParams.searchModification->back().cvParam->at(0).accession.compare("null") == 0){
      analysisProtocolCollection.spectrumIdentificationProtocol->back().modificationParams.searchModification->back().cvParam->clear();
    }
    analysisProtocolCollection.spectrumIdentificationProtocol->back().modificationParams.searchModification->back().cvParam->push_back(cv);
    break;
  case SearchType:
    analysisProtocolCollection.spectrumIdentificationProtocol->back().searchType.cvParam=cv;
    break;
  case SoftwareName:
    analysisSoftwareList.analysisSoftware->back().softwareName.cvParam=cv;
    break;
  case SpecificityRules:
    analysisProtocolCollection.spectrumIdentificationProtocol->back().modificationParams.searchModification->back().specificityRules.addCvParam(cv);
    break;
  case SpectrumIDFormat:
    dataCollection.inputs.spectraData->back().spectrumIDFormat.cvParam=cv;
    break;
  case SpectrumIdentificationItem:
    dataCollection.analysisData.spectrumIdentificationList->back().spectrumIdentificationResult->back().spectrumIdentificationItem->back().addCvParam(cv);
    break;
  case SpectrumIdentificationResult:
    dataCollection.analysisData.spectrumIdentificationList->back().spectrumIdentificationResult->back().cvParam->push_back(cv);
    break;
  case Threshold:
    if (e == SpectrumIdentificationProtocol){
      if (analysisProtocolCollection.spectrumIdentificationProtocol->back().threshold.cvParam->at(0).name.compare("null") == 0){
        analysisProtocolCollection.spectrumIdentificationProtocol->back().threshold.cvParam->clear();
      }
      analysisProtocolCollection.spectrumIdentificationProtocol->back().threshold.cvParam->push_back(cv);
    } else if (e == ProteinDetectionProtocol){
      if (analysisProtocolCollection.proteinDetectionProtocol.threshold.cvParam->at(0).name.compare("null") == 0){
        analysisProtocolCollection.proteinDetectionProtocol.threshold.cvParam->clear();
      }
      analysisProtocolCollection.proteinDetectionProtocol.threshold.cvParam->push_back(cv);
    }
    break;
  default:
    cout << "unprocessed cvParam: " << cv.accession << endl;
    break;
  }
}

void CMzIdentML::processUserParam(sUserParam& u){
  switch (activeEl.back()){
  case AdditionalSearchParams:
    if (analysisProtocolCollection.spectrumIdentificationProtocol->back().additionalSearchParams.userParam->at(0).name.compare("null") == 0) {
      analysisProtocolCollection.spectrumIdentificationProtocol->back().additionalSearchParams.userParam->clear();
    }
    analysisProtocolCollection.spectrumIdentificationProtocol->back().additionalSearchParams.userParam->push_back(u);
    break;
  case DatabaseName:
    dataCollection.inputs.searchDatabase->back().databaseName.userParam=u;
    break;
  case EnzymeName:
    if (analysisProtocolCollection.spectrumIdentificationProtocol->back().enzymes.enzyme->back().enzymeName.userParam->at(0).name.compare("null") == 0){
      analysisProtocolCollection.spectrumIdentificationProtocol->back().enzymes.enzyme->back().enzymeName.userParam->clear();
    }
    analysisProtocolCollection.spectrumIdentificationProtocol->back().enzymes.enzyme->back().enzymeName.userParam->push_back(u);
    break;
  case MassTable:
    analysisProtocolCollection.spectrumIdentificationProtocol->back().massTable->back().userParam->push_back(u);
    break;
  case ProteinAmbiguityGroup:
    dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().userParam->push_back(u);
    break;
  case ProteinDetectionHypothesis:
    dataCollection.analysisData.proteinDetectionList.proteinAmbiguityGroup->back().proteinDetectionHypothesis->back().userParam->push_back(u);
    break;
  case SpectrumIdentificationItem:
    dataCollection.analysisData.spectrumIdentificationList->back().spectrumIdentificationResult->back().spectrumIdentificationItem->back().userParam->push_back(u);
    break;
  default:
    cout << "unprocessed userParam" << endl;
    break;
  }
}

bool CMzIdentML::readFile(const char* fn) {

  XML_ParserFree(parser);
  parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, this);
  XML_SetElementHandler(parser, CMzIdentML_startElementCallback, CMzIdentML_endElementCallback);
  XML_SetCharacterDataHandler(parser, CMzIdentML_charactersCallback);

  // clear data
  cvList.clear();
  analysisSoftwareList.clear();

  FILE* fptr = fopen(fn, "rt");
  if (fptr == NULL){
    cerr << "Error parse(): No open file." << endl;
    return false;
  }

  char buffer[16384];
  int readBytes = 0;
  bool success = true;
  int chunk = 0;
  killRead = false;

  while (success && (readBytes = (int)fread(buffer, 1, sizeof(buffer), fptr)) != 0){
    success = (XML_Parse(parser, buffer, readBytes, false) != 0);
    if (killRead){
      fclose(fptr);
      return false;
    }
  }
  success = success && (XML_Parse(parser, buffer, 0, true) != 0);

  if (!success) {
    XML_Error error = XML_GetErrorCode(parser);

    cerr << fn << "(" << XML_GetCurrentLineNumber(parser) << ") : error " << (int)error << ": ";
    switch (error) {
    case XML_ERROR_SYNTAX:
    case XML_ERROR_INVALID_TOKEN:
    case XML_ERROR_UNCLOSED_TOKEN:
      cerr << "Syntax error parsing XML.";
      break;
    case XML_ERROR_TAG_MISMATCH:
      cerr << "XML tag mismatch.";
      break;
    case XML_ERROR_DUPLICATE_ATTRIBUTE:
      cerr << "XML duplicate attribute.";
      break;
    case XML_ERROR_JUNK_AFTER_DOC_ELEMENT:
      cerr << "XML junk after doc element.";
      break;
    default:
      cerr << "XML Parsing error.";
      break;
    }
    cerr << "\n";
    fclose(fptr);
    return false;
  }

  fclose(fptr);

  fileFull=fn;
  filePath=fileFull;
  if(filePath.find_last_of("\\")!=string::npos) filePath=filePath.substr(0,filePath.find_last_of("\\"));
  else if (filePath.find_last_of("/") != string::npos) filePath = filePath.substr(0, filePath.find_last_of("/"));
  else filePath.clear();
  fileBase=fileFull;
  if (fileBase.find_last_of("\\") != string::npos) fileBase = fileBase.substr(fileBase.find_last_of("\\")+1,fileBase.size());
  else if (fileBase.find_last_of("/") != string::npos) fileBase = fileBase.substr(fileBase.find_last_of("/")+1,fileBase.size());
  if (fileBase.find_last_of(".")!=string::npos) fileBase = fileBase.substr(0,fileBase.find_last_of("."));

  return true;
}

void CMzIdentML::setVersion(int ver){
  if(ver<1 || ver>2){
    cerr << "CMzIdentML::setVersion(): invalid version number. Defaulting to 2 (1.2.0)" << endl;
    version=2;
  } else {
    version=ver;
  }
}

bool CMzIdentML::writeFile(const char* fn){
  FILE* f = fopen(fn,"wt");
  if (f==NULL) return false;

  char timebuf[80];
  time_t timeNow;
  time(&timeNow);
  strftime(timebuf, 80, "%Y-%m-%dT%H:%M:%S", localtime(&timeNow));

  fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(f, "<MzIdentML id=\"%s\"",id.c_str());
  if (version == 1) fprintf(f, " version=\"%s\"  xsi:schemaLocation=\"%s\" xmlns = \"%s\"", mzIdentMLv1, mzIdentMLv1schema, mzIdentMLv1xmlns);
  else fprintf(f, " version=\"%s\"  xsi:schemaLocation=\"%s\" xmlns = \"%s\"", mzIdentMLv2, mzIdentMLv2schema, mzIdentMLv2xmlns);
  fprintf(f, " xmlns:xsi = \"http://www.w3.org/2001/XMLSchema-instance\" creationDate = \"%s\">\n",timebuf);

  cvList.writeOut(f,1);
  analysisSoftwareList.writeOut(f,1);
  sequenceCollection.writeOut(f,1);
  analysisCollection.writeOut(f,1);
  analysisProtocolCollection.writeOut(f,1);
  dataCollection.writeOut(f,1);

  fprintf(f, "</MzIdentML>\n");
  fclose(f);
  return true;

}