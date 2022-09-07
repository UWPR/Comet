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
  versionStr.clear();
  xmlns.clear();
  schema.clear();
  parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, this);
  XML_SetElementHandler(parser, CMzIdentML_startElementCallback, CMzIdentML_endElementCallback);
  XML_SetCharacterDataHandler(parser, CMzIdentML_charactersCallback);
}

CMzIdentML::~CMzIdentML(){
  XML_ParserFree(parser);
}

void CMzIdentML::characters(const XML_Char *s, int len) {
  char* st = new char[len + 1];
  switch (activeEl.back()){
  case Customizations:
    strncpy(st, s, len);
    st[len] = '\0';
    analysisSoftwareList.analysisSoftware.back().customizations.back().text = st;
    break;
  case ExternalFormatDocumentation:
    strncpy(st, s, len);
    st[len] = '\0';
    dataCollection.inputs.spectraData.back().externalFormatDocumentation.text=st;
    break;
  case PeptideSequence:
    char strbuf[1024];
    if (len>1024) {
      cout << "character buffer overrun" << endl;
      return;
    }
    strncpy(strbuf,s,len);
    strbuf[len] = '\0';
    sequenceCollection.peptide.back().peptideSequence.text+=strbuf;
    break;
  case Seq:
    strncpy(st,s,len);
    st[len]='\0';
    sequenceCollection.dbSequence.back().seq.back().text=st;
    break;
  case SiteRegexp:
    strncpy(st, s, len);
    st[len] = '\0';
    analysisProtocolCollection.spectrumIdentificationProtocol.back().enzymes.back().enzyme.back().siteRegexp.back().text+=st;
    break;
  default:
    //cout << "unprocessed characters: " << s << endl;
    break;
  }
  delete [] st;
}

void CMzIdentML::endElement(const XML_Char *el) {

  string s;

  if (isElement("AdditionalSearchParams", el)){
    if (activeEl.back() != AdditionalSearchParams) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Affiliation", el)){
    if (activeEl.back() != Affiliation) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("AmbiguousResidue",el)){
    if (activeEl.back() != AmbiguousResidue) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("AnalysisCollection", el)){
    if (activeEl.back() != AnalysisCollection) cout << "Error in activeEl" << endl;
    else activeEl.pop_back(); 

  } else if (isElement("AnalysisData", el)){
      if (activeEl.back() != AnalysisData) cout << "Error in activeEl" << endl;
      else activeEl.pop_back();

  } else if (isElement("AnalysisParams", el)){
    if (activeEl.back() != AnalysisParams) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();
    
  } else if (isElement("AnalysisProtocolCollection", el)){
    if (activeEl.back() != AnalysisProtocolCollection) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("AnalysisSampleCollection", el)){
    if (activeEl.back() != AnalysisSampleCollection) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("AnalysisSoftware", el)){
    if (activeEl.back() != AnalysisSoftware) cout << "Error in activeEl" << endl;
    else activeEl.pop_back(); 
    
  } else if (isElement("AnalysisSoftwareList", el)){
    if (activeEl.back() != AnalysisSoftwareList) cout << "Error in activeEl" << endl;
    else activeEl.pop_back(); 

  } else if (isElement("AuditCollection", el)){
    if (activeEl.back() != AuditCollection) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("BibliographicReference", el)){
    if (activeEl.back() != BibliographicReference) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("ContactRole", el)){
    if (activeEl.back() != ContactRole) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Customizations", el)){
    if (activeEl.back() != Customizations) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("DBSequence", el)){
    if (activeEl.back() != DBSequence) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("DataCollection", el)){
    if (activeEl.back() != DataCollection) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("DatabaseFilters", el)){
    if (activeEl.back() != DatabaseFilters) cout << "Error in activeEl" << endl;
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

  } else if (isElement("ExternalFormatDocumentation", el)){
    if (activeEl.back() != ExternalFormatDocumentation) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("FileFormat", el)){
    if (activeEl.back() != FileFormat) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Filter", el)){
    if (activeEl.back() != Filter) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("FilterType", el)){
    if (activeEl.back() != FilterType) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("FragmentArray", el)){
    if (activeEl.back() != FragmentArray) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Fragmentation", el)){
    if (activeEl.back() != Fragmentation) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("FragmentationTable", el)){
    if (activeEl.back() != FragmentationTable) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("FragmentTolerance", el)){
    if (activeEl.back() != FragmentTolerance) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Include", el)){
    if (activeEl.back() != Include) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();
    
  } else if (isElement("Inputs", el)){
    if (activeEl.back() != Inputs) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("IonType", el)){
    if (activeEl.back() != IonType) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("MassTable", el)){
    if (activeEl.back() != MassTable) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Measure", el)){
    if (activeEl.back() != Measure) cout << "Error in activeEl" << endl;
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

  } else if (isElement("Organization", el)){
    if (activeEl.back() != Organization) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Parent", el)){
    if (activeEl.back() != Parent) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("ParentTolerance", el)){
    if (activeEl.back() != ParentTolerance) cout << "Error in activeEl" << endl;
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

  } else if (isElement("Person", el)){
    if (activeEl.back() != Person) cout << "Error in activeEl" << endl;
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

  } else if (isElement("Provider", el)){
    if (activeEl.back() != Provider) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Residue", el)){
    if (activeEl.back() != Residue) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Role", el)){
    if (activeEl.back() != Role) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("Sample", el)){
    if (activeEl.back() != Sample) cout << "Error in activeEl" << endl;
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

  } else if (isElement("Seq", el)){
    if (activeEl.back() != Seq) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SequenceCollection", el)){
    if (activeEl.back() != SequenceCollection) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SiteRegexp", el)){
    if (activeEl.back() != SiteRegexp) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SoftwareName", el)){
    if (activeEl.back() != SoftwareName) cout << "Error in activeEl" << endl;
    else activeEl.pop_back();

  } else if (isElement("SourceFile", el)){
    if (activeEl.back() != SourceFile) cout << "Error in activeEl" << endl;
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

  //Nifty debugging
  //cout << el << endl;

  string s;

  if (isElement("AdditionalSearchParams", el)){
    activeEl.push_back(AdditionalSearchParams);
    CAdditionalSearchParams c;
    analysisProtocolCollection.spectrumIdentificationProtocol.back().additionalSearchParams.push_back(c);
    
  } else if (isElement("Affiliation", el)){
    activeEl.push_back(Affiliation);
    CAffiliation c;
    c.organizationRef=getAttrValue("organization_ref",attr);
    auditCollection.back().person.back().affiliation.push_back(c);

  } else if (isElement("AmbiguousResidue", el)){
    activeEl.push_back(AmbiguousResidue);
    CAmbiguousResidue m;
    m.code = getAttrValue("code", attr)[0];
    analysisProtocolCollection.spectrumIdentificationProtocol.back().massTable.back().ambiguousResidue.push_back(m);

  } else if (isElement("AnalysisCollection", el)){
    activeEl.push_back(AnalysisCollection);

  } else if (isElement("AnalysisData", el)){
    activeEl.push_back(AnalysisData);

  } else if (isElement("AnalysisParams", el)){
    activeEl.push_back(AnalysisParams);
    CAnalysisParams c;
    analysisProtocolCollection.proteinDetectionProtocol.back().analysisParams.push_back(c);

  } else if (isElement("AnalysisProtocolCollection", el)){
    activeEl.push_back(AnalysisProtocolCollection);
  
  } else if (isElement("AnalysisSampleCollection", el)){
    activeEl.push_back(AnalysisSampleCollection);
    CAnalysisSampleCollection c;
    analysisSampleCollection.push_back(c);

  } else if (isElement("AnalysisSoftware", el)){
    activeEl.push_back(AnalysisSoftware); 
    CAnalysisSoftware as;
    as.id = getAttrValue("id", attr);
    as.name = getAttrValue("name", attr);
    as.version = getAttrValue("version", attr);
    as.uri = getAttrValue("uri", attr);
    analysisSoftwareList.analysisSoftware.push_back(as);
    
  } else if (isElement("AnalysisSoftwareList", el)){
    activeEl.push_back(AnalysisSoftwareList);

  } else if (isElement("AuditCollection", el)){
    activeEl.push_back(AuditCollection);
    CAuditCollection c;
    if(auditCollection.size()>0){
      cerr << "Error: Multiple audit collections read." << endl;
      exit(71);
    }
    auditCollection.push_back(c);

  } else if (isElement("BibliographicReference", el)){
    activeEl.push_back(BibliographicReference);
    CBibliographicReference c;
    c.authors = getAttrValue("authors", attr);
    c.doi = getAttrValue("doi", attr);
    c.editor = getAttrValue("editor", attr);
    c.id = getAttrValue("id", attr);
    c.issue = getAttrValue("issue", attr);
    c.name = getAttrValue("name", attr);
    c.pages = getAttrValue("pages", attr);
    c.publication = getAttrValue("publication", attr);
    c.publisher = getAttrValue("publisher", attr);
    c.title = getAttrValue("title", attr);
    c.volume = getAttrValue("volume", attr);
    c.year = atoi(getAttrValue("year", attr));
    bibliographicReference.push_back(c);

  } else if (isElement("ContactRole", el)){
    activeEl.push_back(ContactRole);
    CContactRole c;
    c.contactRef=getAttrValue("contact_ref",attr);
    if(activeEl[activeEl.size()-2]==AnalysisSoftware){
      if (analysisSoftwareList.analysisSoftware.back().contactRole.size()>0){
        cerr << "Error: Multiple AnalysisSoftware::ContactRole read." << endl;
        exit(71);
      }
      analysisSoftwareList.analysisSoftware.back().contactRole.push_back(c);
    } else if (activeEl[activeEl.size() - 2] == Provider){
      if(provider.contactRole.size()>0){
        cerr << "Error: Multiple Provider::ContactRole read." << endl;
        exit(71);
      }
      provider.contactRole.push_back(c);
    } else {
      cerr << "Cannot find parent element of ContactRole" << endl;
      exit(71);
    }

  } else if (isElement("Customizations", el)){
    activeEl.push_back(Customizations);
    sCustomizations c;
    analysisSoftwareList.analysisSoftware.back().customizations.push_back(c);

  } else if (isElement("DBSequence", el)){
    activeEl.push_back(DBSequence);
    CDBSequence db;
    db.id = getAttrValue("id", attr);
    db.accession = getAttrValue("accession", attr);
    db.name = getAttrValue("name", attr);
    db.searchDatabaseRef = getAttrValue("searchDatabase_ref", attr);
    db.length = atoi(getAttrValue("length", attr));
    sequenceCollection.dbSequence.push_back(db);
    //sequenceCollection.sortDBSequence=true;

  } else if (isElement("DataCollection", el)){
    activeEl.push_back(DataCollection);

  } else if (isElement("DatabaseFilters", el)){
    activeEl.push_back(DatabaseFilters);
    CDatabaseFilters c;
    analysisProtocolCollection.spectrumIdentificationProtocol.back().databaseFilters.push_back(c);

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
    analysisProtocolCollection.spectrumIdentificationProtocol.back().enzymes.back().enzyme.push_back(c);

  } else if(isElement("EnzymeName",el)){
    activeEl.push_back(EnzymeName);
    CEnzymeName c;
    analysisProtocolCollection.spectrumIdentificationProtocol.back().enzymes.back().enzyme.back().enzymeName.push_back(c);

  } else if (isElement("Enzymes",el)){
    activeEl.push_back(Enzymes);
    CEnzymes c;
    s = getAttrValue("isDecoy", attr);
    if (s.compare("true") == 0) c.independent = true;
    else c.independent = false;
    analysisProtocolCollection.spectrumIdentificationProtocol.back().enzymes.push_back(c);

  } else if (isElement("ExternalFormatDocumentation", el)){
    activeEl.push_back(ExternalFormatDocumentation);

  } else if (isElement("FileFormat", el)){
    activeEl.push_back(FileFormat);

  } else if (isElement("Filter", el)){
    activeEl.push_back(Filter);
    CFilter c;
    analysisProtocolCollection.spectrumIdentificationProtocol.back().databaseFilters.back().filter.push_back(c);

  } else if (isElement("FilterType", el)){
    activeEl.push_back(FilterType);

  } else if (isElement("FragmentArray", el)){
    activeEl.push_back(FragmentArray);
    CFragmentArray c;
    c.measureRef=getAttrValue("measure_ref",attr);
    c.values=getAttrValue("values",attr);
    dataCollection.analysisData.spectrumIdentificationList.back().spectrumIdentificationResult.back().spectrumIdentificationItem.back().fragmentation.back().ionType.back().fragmentArray.push_back(c);

  } else if (isElement("Fragmentation", el)){
    activeEl.push_back(Fragmentation);
    CFragmentation c;
    if (dataCollection.analysisData.spectrumIdentificationList.back().spectrumIdentificationResult.back().spectrumIdentificationItem.back().fragmentation.size()>0){
      cerr << "Error: Multiple SpectrumIdentificationItem::Fragmentation read." << endl;
      exit(71);
    }
    dataCollection.analysisData.spectrumIdentificationList.back().spectrumIdentificationResult.back().spectrumIdentificationItem.back().fragmentation.push_back(c);

  } else if (isElement("FragmentationTable", el)){
    activeEl.push_back(FragmentationTable);
    CFragmentationTable c;
    dataCollection.analysisData.spectrumIdentificationList.back().fragmentationTable.push_back(c);

  } else if (isElement("FragmentTolerance", el)){
    activeEl.push_back(FragmentTolerance);
    CFragmentTolerance c;
    if (analysisProtocolCollection.spectrumIdentificationProtocol.back().fragmentTolerance.size()>0){
      cerr << "Error: Multiple FragmentTolerance read." << endl;
      exit(71);
    }
    analysisProtocolCollection.spectrumIdentificationProtocol.back().fragmentTolerance.push_back(c);

  } else if (isElement("IonType", el)){
    activeEl.push_back(IonType);
    CIonType c;
    c.charge=atoi(getAttrValue("charge",attr));
    c.index=getAttrValue("index",attr);
    dataCollection.analysisData.spectrumIdentificationList.back().spectrumIdentificationResult.back().spectrumIdentificationItem.back().fragmentation.back().ionType.push_back(c);

  } else if (isElement("Include", el)){
    activeEl.push_back(Include);
    CInclude c;
    analysisProtocolCollection.spectrumIdentificationProtocol.back().databaseFilters.back().filter.back().include.push_back(c);

  } else if (isElement("Inputs", el)){
    activeEl.push_back(Inputs);

  } else if (isElement("InputSpectra",el)){
    sInputSpectra is;
    is.spectraDataRef = getAttrValue("spectraData_ref", attr);
    analysisCollection.spectrumIdentification.back().inputSpectra.push_back(is);

  } else if (isElement("InputSpectrumIdentifications", el)){
    sInputSpectrumIdentifications isi;
    isi.spectrumIdentificationListRef = getAttrValue("spectrumIdentificationList_ref", attr);
    analysisCollection.proteinDetection.back().inputSpectrumidentifications.push_back(isi);

  } else if (isElement("MassTable", el)){
    activeEl.push_back(MassTable);
    CMassTable m;
    m.id=getAttrValue("id",attr);
    m.name=getAttrValue("name",attr);
    m.msLevel = getAttrValue("msLevel", attr);
    analysisProtocolCollection.spectrumIdentificationProtocol.back().massTable.push_back(m);

  } else if (isElement("Measure", el)){
    activeEl.push_back(Measure);
    CMeasure m;
    m.id = getAttrValue("id", attr);
    m.name = getAttrValue("name",attr);
    dataCollection.analysisData.spectrumIdentificationList.back().fragmentationTable.back().measure.push_back(m);

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
    sequenceCollection.peptide.back().modification.push_back(m);

  } else if (isElement("ModificationParams", el)){
    activeEl.push_back(ModificationParams);
    CModificationParams c;
    analysisProtocolCollection.spectrumIdentificationProtocol.back().modificationParams.push_back(c);

  } else if (isElement("MzIdentML", el)){
    activeEl.push_back(MzIdentML);
    s = getAttrValue("version", attr);
    if (s.size() == 0)  killRead=true;
    versionStr=s;
    xmlns=getAttrValue("xmlns",attr);
    schema=getAttrValue("xsi:schemaLocation",attr);
    id=getAttrValue("id",attr);
    creationDate.parseDateTime(getAttrValue("creationDate",attr));

  } else if (isElement("Organization", el)){
    activeEl.push_back(Organization);
    COrganization c;
    c.id = getAttrValue("id", attr);
    c.name = getAttrValue("name", attr);
    auditCollection.back().organization.push_back(c);

  } else if (isElement("Parent", el)){
    activeEl.push_back(Parent);
    CParent c;
    c.organizationRef = getAttrValue("organization_ref", attr);
    auditCollection.back().organization.back().parent=c;

  } else if (isElement("ParentTolerance", el)){
    activeEl.push_back(ParentTolerance);
    CParentTolerance c;
    if (analysisProtocolCollection.spectrumIdentificationProtocol.back().parentTolerance.size()>0){
      cerr << "Error: Multiple ParentTolerance read." << endl;
      exit(71);
    }
    analysisProtocolCollection.spectrumIdentificationProtocol.back().parentTolerance.push_back(c);

  } else if (isElement("Peptide", el)){
    activeEl.push_back(Peptide);
    CPeptide p;
    p.id = getAttrValue("id", attr);
    p.name = getAttrValue("name", attr);
    sequenceCollection.peptide.push_back(p);
    //sequenceCollection.sortPeptide=true;

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
    s = getAttrValue("end", attr);
    if (s.size()>0) pe.end = atoi(s.c_str());
    s = getAttrValue("start", attr);
    if (s.size()>0) pe.start = atoi(s.c_str());
    sequenceCollection.peptideEvidence.push_back(pe);
    //sequenceCollection.sortPeptideEvidence=true;

  } else if (isElement("PeptideEvidenceRef", el)){
    sPeptideEvidenceRef per;
    per.peptideEvidenceRef = getAttrValue("peptideEvidence_ref", attr);
    dataCollection.analysisData.spectrumIdentificationList.back().spectrumIdentificationResult.back().spectrumIdentificationItem.back().peptideEvidenceRef.push_back(per);

  } else if (isElement("PeptideHypothesis", el)){
    activeEl.push_back(PeptideHypothesis);
    CPeptideHypothesis ph;
    ph.peptideEvidenceRef = getAttrValue("peptideEvidence_ref",attr);
    dataCollection.analysisData.proteinDetectionList.back().proteinAmbiguityGroup.back().proteinDetectionHypothesis.back().peptideHypothesis.push_back(ph);

  } else if (isElement("PeptideSequence", el)){
    activeEl.push_back(PeptideSequence);

  } else if (isElement("Person", el)){
    activeEl.push_back(Person);
    CPerson c;
    c.firstName=getAttrValue("firstName", attr);
    c.id = getAttrValue("id", attr);
    c.lastName = getAttrValue("lastName", attr);
    c.midInitials = getAttrValue("midInitials", attr);
    c.name = getAttrValue("name", attr);
    auditCollection.back().person.push_back(c);

  } else if (isElement("ProteinAmbiguityGroup", el)){
    activeEl.push_back(ProteinAmbiguityGroup);
    CProteinAmbiguityGroup pag;
    pag.id = getAttrValue("id", attr);
    dataCollection.analysisData.proteinDetectionList.back().proteinAmbiguityGroup.push_back(pag);

  } else if (isElement("ProteinDetection", el)){
    activeEl.push_back(ProteinDetection);
    CProteinDetection c;
    c.id=getAttrValue("id", attr);
    c.name = getAttrValue("name", attr);
    c.proteinDetectionListRef=getAttrValue("proteinDetectionList_ref", attr);
    c.proteinDetectionProtocolRef=getAttrValue("proteinDetectionProtocol_ref", attr);
    analysisCollection.proteinDetection.push_back(c);

  } else if (isElement("ProteinDetectionHypothesis", el)){
    activeEl.push_back(ProteinDetectionHypothesis);
    CProteinDetectionHypothesis pdh;
    pdh.id = getAttrValue("id", attr);
    pdh.dbSequenceRef = getAttrValue("dBSequence_ref", attr);
    s = getAttrValue("passThreshold",attr);
    if(s.compare("true") == 0) pdh.passThreshold = true;
    else pdh.passThreshold = false;
    dataCollection.analysisData.proteinDetectionList.back().proteinAmbiguityGroup.back().proteinDetectionHypothesis.push_back(pdh);

  } else if (isElement("ProteinDetectionList", el)){
    activeEl.push_back(ProteinDetectionList);
    CProteinDetectionList c;
    c.id = getAttrValue("id", attr);
    c.name = getAttrValue("name", attr);
    dataCollection.analysisData.proteinDetectionList.push_back(c);

  } else if (isElement("ProteinDetectionProtocol", el)){
    activeEl.push_back(ProteinDetectionProtocol);
    CProteinDetectionProtocol pdp;
    pdp.id = getAttrValue("id", attr);
    pdp.analysisSoftwareRef = getAttrValue("analysisSoftware_ref", attr);
    analysisProtocolCollection.proteinDetectionProtocol.push_back(pdp);

  } else if (isElement("Provider", el)){
    activeEl.push_back(Provider);
    provider.analysisSoftwareRef = getAttrValue("analysisSoftware_ref", attr);
    provider.id = getAttrValue("id", attr);
    provider.name = getAttrValue("name", attr);
    
  } else if (isElement("Residue", el)){
    activeEl.push_back(Residue);
    CResidue m;
    m.code = getAttrValue("code", attr)[0];
    m.mass = (float)atof(getAttrValue("mass", attr));
    analysisProtocolCollection.spectrumIdentificationProtocol.back().massTable.back().residue.push_back(m);

  } else if (isElement("Role", el)){
    activeEl.push_back(Role);
    CRole c;
    if (activeEl[activeEl.size() - 3] == AnalysisSoftware){
      analysisSoftwareList.analysisSoftware.back().contactRole.back().role=c;
    } else if (activeEl[activeEl.size() - 3] == Provider){
      provider.contactRole.back().role=c;
    } else {
      cerr << "Cannot find parent element of Role" << endl;
      exit(71);
    }

  } else if (isElement("Sample", el)){
    activeEl.push_back(Sample);
    CSample c;
    c.id = getAttrValue("id", attr);
    c.name = getAttrValue("name", attr);
    analysisSampleCollection.back().sample.push_back(c);

  } else if (isElement("SearchDatabase", el)){
    activeEl.push_back(SearchDatabase);
    CSearchDatabase db;
    db.id = getAttrValue("id", attr);
    db.location = getAttrValue("location", attr);
    dataCollection.inputs.searchDatabase.push_back(db);

  } else if (isElement("SearchDatabaseRef", el)){
    sSearchDatabaseRef sdr;
    sdr.searchDatabaseRef = getAttrValue("searchDatabase_ref", attr);
    analysisCollection.spectrumIdentification.back().searchDatabaseRef.push_back(sdr);

  } else if (isElement("SearchModification", el)){
    activeEl.push_back(SearchModification);
    CSearchModification sm;
    sm.residues = getAttrValue("residues", attr);
    s = getAttrValue("massDelta", attr);
    sm.massDelta = atof(&s[0]);
    s = getAttrValue("fixedMod", attr);
    if (s.compare("true") == 0) sm.fixedMod = true;
    else sm.fixedMod = false;
    analysisProtocolCollection.spectrumIdentificationProtocol.back().modificationParams.back().searchModification.push_back(sm);

  } else if (isElement("SearchType", el)){
    activeEl.push_back(SearchType);

  } else if (isElement("Seq", el)){
    activeEl.push_back(Seq);
    sSeq c;
    sequenceCollection.dbSequence.back().seq.push_back(c);

  } else if (isElement("SequenceCollection", el)){
    activeEl.push_back(SequenceCollection);

  } else if (isElement("SiteRegexp", el)){
    activeEl.push_back(SiteRegexp);
    CSiteRegexp c;
    analysisProtocolCollection.spectrumIdentificationProtocol.back().enzymes.back().enzyme.back().siteRegexp.push_back(c);

  } else if (isElement("SoftwareName", el)){
    activeEl.push_back(SoftwareName);

  } else if (isElement("SourceFile", el)){
    activeEl.push_back(SourceFile);
    CSourceFile c;
    c.id=getAttrValue("id",attr);
    c.location=getAttrValue("location",attr);
    c.name=getAttrValue("name",attr);
    dataCollection.inputs.sourceFile.push_back(c);

  } else if (isElement("SpecificityRules", el)){
    activeEl.push_back(SpecificityRules);
    CSpecificityRules c;
    analysisProtocolCollection.spectrumIdentificationProtocol.back().modificationParams.back().searchModification.back().specificityRules.push_back(c);

  } else if (isElement("SpectraData", el)){
    activeEl.push_back(SpectraData);
    CSpectraData sd;
    sd.id = getAttrValue("id", attr);
    sd.location = getAttrValue("location", attr);
    dataCollection.inputs.spectraData.push_back(sd);

  } else if (isElement("SpectrumIDFormat", el)){
    activeEl.push_back(SpectrumIDFormat);

  } else if (isElement("SpectrumIdentification", el)){
    activeEl.push_back(SpectrumIdentification);
    CSpectrumIdentification si;
    si.id = getAttrValue("id", attr);
    si.spectrumIdentificationListRef = getAttrValue("spectrumIdentificationList_ref", attr);
    si.spectrumIdentificationProtocolRef = getAttrValue("spectrumIdentificationProtocol_ref", attr);
    analysisCollection.spectrumIdentification.push_back(si);

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
    dataCollection.analysisData.spectrumIdentificationList.back().spectrumIdentificationResult.back().spectrumIdentificationItem.push_back(sii);
    
  } else if (isElement("SpectrumIdentificationItemRef", el)){
    sSpectrumIdentificationItemRef siir; 
    siir.text = getAttrValue("spectrumIdentificationItem_ref",attr);
    dataCollection.analysisData.proteinDetectionList.back().proteinAmbiguityGroup.back().proteinDetectionHypothesis.back().peptideHypothesis.back().spectrumIdentificationItemRef.push_back(siir);

  } else if (isElement("SpectrumIdentificationList", el)){
    activeEl.push_back(SpectrumIdentificationList);
    CSpectrumIdentificationList sil;
    sil.id = getAttrValue("id", attr);
    sil.numSequencesSearched = atoi(getAttrValue("numSequencesSearched",attr));
    dataCollection.analysisData.spectrumIdentificationList.push_back(sil);

  } else if (isElement("SpectrumIdentificationProtocol", el)){
    activeEl.push_back(SpectrumIdentificationProtocol);
    CSpectrumIdentificationProtocol sip;
    sip.id = getAttrValue("id", attr);
    sip.analysisSoftwareRef = getAttrValue("analysisSoftware_ref", attr);
    sip.name = getAttrValue("name", attr);
    analysisProtocolCollection.spectrumIdentificationProtocol.push_back(sip);

  } else if (isElement("SpectrumIdentificationResult", el)){
    activeEl.push_back(SpectrumIdentificationResult);
    CSpectrumIdentificationResult sir;
    sir.id = getAttrValue("id", attr);
    sir.spectrumID = getAttrValue("spectrumID", attr);
    sir.spectraDataRef = getAttrValue("spectraData_ref", attr);
    sir.name=getAttrValue("name",attr);
    dataCollection.analysisData.spectrumIdentificationList.back().spectrumIdentificationResult.push_back(sir);

  } else if (isElement("SubSample", el)){
    activeEl.push_back(SubSample);
    CSubSample c;
    c.sampleRef = getAttrValue("sample_ref", attr);
    analysisSampleCollection.back().sample.back().subSample.push_back(c);

  } else if (isElement("Threshold", el)){
    activeEl.push_back(Threshold);

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

  } else {
    cout << "Unprocessed element: " << el << endl;
  }
}


//Adds the analysis software information and returns a reference id.
string CMzIdentML::addAnalysisSoftware(string software, string version){
  CAnalysisSoftware as;
  as.name=software;
  as.version=version;

  size_t i;
  for (i = 0; i < analysisSoftwareList.analysisSoftware.size(); i++){
    if (analysisSoftwareList.analysisSoftware[i] == as) return analysisSoftwareList.analysisSoftware[i].id;
  }

  char cID[32];
  sprintf(cID, "AS%d", (int)analysisSoftwareList.analysisSoftware.size());
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
  } else if (software.compare("PTMProphet") == 0 || software.compare("ptmprophet") == 0){
    as.softwareName.cvParam.accession = "MS:1002292";
    as.softwareName.cvParam.cvRef = "PSI-MS";
    as.softwareName.cvParam.name = "PTMProphet";
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

  analysisSoftwareList.analysisSoftware.push_back(as);
  return as.id;
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
    dbs.cvParam.push_back(cv);
  }

  string idRef = sequenceCollection.addDBSequence(dbs);
  return idRef;
}

string CMzIdentML::addPeptide(string seq, vector<CModification>& mods){
  CPeptide p;
  p.peptideSequence.text=seq;
  
  size_t i;
  for (i = 0; i < mods.size(); i++){
    p.modification.push_back(mods[i]);
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
  
  sprintf(str, "PAG_%zu", dataCollection.analysisData.proteinDetectionList.back().proteinAmbiguityGroup.size());
  p.id=str;

  dataCollection.analysisData.proteinDetectionList.back().proteinAmbiguityGroup.push_back(p);
  return &dataCollection.analysisData.proteinDetectionList.back().proteinAmbiguityGroup.back();
}

CProteinDetection* CMzIdentML::addProteinDetection(vector<string>& specIdentListRef, string protDetectProtRef, CProteinDetectionList*& pdl){

  CProteinDetection pd;
  sInputSpectrumIdentifications isi;

  for(size_t i=0;i<specIdentListRef.size();i++){
    pd.addInputSpectrumIdentification(specIdentListRef[i]);
  }

  char cID[32];
  sprintf(cID, "PD%d", (int)analysisCollection.proteinDetection.size());
  pd.id = cID;

  pd.proteinDetectionProtocolRef = protDetectProtRef;
  pd.proteinDetectionListRef = dataCollection.analysisData.addProteinDetectionList();
  pdl = &dataCollection.analysisData.proteinDetectionList.back();

  analysisCollection.proteinDetection.push_back(pd);
  return &analysisCollection.proteinDetection.back();

}

CSpectrumIdentification* CMzIdentML::addSpectrumIdentification(string spectraDataRef, string searchDatabaseRef, string specIdentProtRef, CSpectrumIdentificationList*& sil){

  CSpectrumIdentification si;
  sInputSpectra is;
  is.spectraDataRef=spectraDataRef;
  sSearchDatabaseRef sdr;
  sdr.searchDatabaseRef= searchDatabaseRef;
  si.inputSpectra.push_back(is);
  si.searchDatabaseRef.push_back(sdr);

  char cID[32];
  sprintf(cID, "SI%d", (int)analysisCollection.spectrumIdentification.size());
  si.id=cID;
  si.spectrumIdentificationProtocolRef = specIdentProtRef;
  si.spectrumIdentificationListRef = dataCollection.analysisData.addSpectrumIdentificationList();
  sil=&dataCollection.analysisData.spectrumIdentificationList.back();

  analysisCollection.spectrumIdentification.push_back(si);
  return &analysisCollection.spectrumIdentification.back();

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
    p1.modification.push_back(mods1[i]);
    sprintf(str,"[%d,%.2lf]",mods1[i].location, mods1[i].monoisotopicMassDelta);
    ID1+=str;
  }
  ID2 = seq2;
  p2.peptideSequence.text = seq2;
  for (i = 0; i < mods2.size(); i++) {
    p2.modification.push_back(mods2[i]);
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

//void CMzIdentML::consolidateSpectrumIdentificationProtocol(){
//  size_t i,j,k;
//  string oldRef;
//  string newRef;
//  for (i = 0; i < analysisProtocolCollection.spectrumIdentificationProtocol->size(); i++){
//    for (j = i + 1; j < analysisProtocolCollection.spectrumIdentificationProtocol->size(); j++){
//      if (analysisProtocolCollection.spectrumIdentificationProtocol->at(i) == analysisProtocolCollection.spectrumIdentificationProtocol->at(j)){
//        newRef = analysisProtocolCollection.spectrumIdentificationProtocol->at(i).id;
//        oldRef = analysisProtocolCollection.spectrumIdentificationProtocol->at(j).id;
//        for (k = 0; k < analysisCollection.spectrumIdentification->size(); k++){
//          if (analysisCollection.spectrumIdentification->at(k).spectrumIdentificationProtocolRef.compare(oldRef) == 0) {
//            analysisCollection.spectrumIdentification->at(k).spectrumIdentificationProtocolRef = newRef;
//          }
//        }
//        analysisProtocolCollection.spectrumIdentificationProtocol->erase(analysisProtocolCollection.spectrumIdentificationProtocol->begin()+j);
//        j--;
//      }
//    }
//  }
//}

CDBSequence CMzIdentML::getDBSequence(string& dBSequence_ref){
  return sequenceCollection.getDBSequence(dBSequence_ref);
}

CDBSequence CMzIdentML::getDBSequenceByAcc(string acc){
  return *sequenceCollection.getDBSequenceByAcc(acc);
}

void CMzIdentML::getDBSequenceByAcc(string acc, vector<CDBSequence>& v){
  sequenceCollection.getDBSequenceByAcc(acc,v);
}

string CMzIdentML::getMzIMLToolsVersion(){
  string st;
  st += mziVersion;
  st += "\t";
  st += mziDate;
  return st;
}

CPeptide CMzIdentML::getPeptide(string peptide_ref){
  CPeptide* p=sequenceCollection.getPeptide(peptide_ref);
  if(p==NULL) {
    cerr << "CMzIdentML::getPeptide failed:: cannot find " << peptide_ref << endl;
    exit(86);
  }
  return *p;
}

bool CMzIdentML::getPeptide(string peptide_ref, CPeptide& p){
  CPeptide* pep = sequenceCollection.getPeptide(peptide_ref);
  if (pep == NULL) {
    cerr << "CMzIdentML::getPeptide failed:: cannot find " << peptide_ref << endl;
    return false;
  }
  p=*pep;
  return true;
}

bool CMzIdentML::getPeptide(string peptide_ref, CPeptide*& p){
  p = sequenceCollection.getPeptide(peptide_ref);
  if (p == NULL) {
    cerr << "CMzIdentML::getPeptide failed:: cannot find " << peptide_ref << endl;
    return false;
  }
  return true;
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
  for (i = 0; i < dataCollection.analysisData.spectrumIdentificationList.size(); i++){
    if(index<(int)dataCollection.analysisData.spectrumIdentificationList[i].spectrumIdentificationResult.size()) break;
    else index -= (int)dataCollection.analysisData.spectrumIdentificationList[i].spectrumIdentificationResult.size();
  }
  if (i == dataCollection.analysisData.spectrumIdentificationList.size()) return psm; //return the empty psm because index is out of range
  
  //return empty psm if rank is out of range
  if (rank >= (int)dataCollection.analysisData.spectrumIdentificationList[i].spectrumIdentificationResult[index].spectrumIdentificationItem.size()) return psm;

  //start gathering data
  CSpectrumIdentificationResult* sir;
  CSpectrumIdentificationItem* sii;
  sir = &dataCollection.analysisData.spectrumIdentificationList[i].spectrumIdentificationResult[index];
  sii = &sir->spectrumIdentificationItem[rank];

  psm.charge=sii->chargeState;
  psm.mzObs=sii->experimentalMassToCharge;

  psm.scanInfo.scanID=sir->spectrumID;
  for (i = 0; i < dataCollection.inputs.spectraData.size(); i++){
    if (dataCollection.inputs.spectraData[i].id.compare(sir->spectraDataRef) == 0){
      psm.scanInfo.fileName = dataCollection.inputs.spectraData[i].location;
      break;
    }
  }

  for (i = 0; i < sir->cvParam.size(); i++){
    if (sir->cvParam[i].accession.compare("MS:1000894") == 0){
      if (sir->cvParam[i].unitName.compare("second") == 0){
        psm.scanInfo.rTimeSec = atof(&sir->cvParam[i].value[0]);
        psm.scanInfo.rTimeMin = psm.scanInfo.rTimeSec/60;
        break;
      } else if (sir->cvParam[i].unitName.compare("minute") == 0){
        psm.scanInfo.rTimeMin = atof(&sir->cvParam[i].value[0]);
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
  for (i = 0; i < pep->modification.size(); i++){
    mod.mass = pep->modification[i].monoisotopicMassDelta;
    mod.pos = pep->modification[i].location;
    mod.residue = pep->modification[i].residues[0]; //potential for error here
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
  //if (sii->peptideEvidenceRef.size()>20) {
  //  cout << "wtf: " << sii->peptideEvidenceRef.size() << " " << sii->id << " " << index << endl;
  //  cout << "\t" << psm.sequence << "\t" << psm.sequenceMod << "\t" << sii->peptideRef << endl;
  //}
  for (i = 0; i < sii->peptideEvidenceRef.size(); i++){
    proteins.push_back(sequenceCollection.getProtein(sii->peptideEvidenceRef[i]));
    //if (index == 4642) cout << sii->peptideEvidenceRef->at(i).peptideEvidenceRef << " " << sequenceCollection.getProtein(sii->peptideEvidenceRef->at(i)) << endl;
  }
  psm.addProteins(proteins);

  sPSMScore score;
  vector<sPSMScore> scores;
  for (i = 0; i < sii->cvParam.size(); i++){
    score.name = sii->cvParam.at(i).name;
    score.value = atof(&sii->cvParam[i].value[0]);
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
  for (size_t i = 0; i < dataCollection.analysisData.spectrumIdentificationList.size(); i++){
    count += dataCollection.analysisData.spectrumIdentificationList[i].spectrumIdentificationResult.size();
  }
  return (int)count;
}

CSpectraData CMzIdentML::getSpectraData(string& spectraData_ref){
  size_t i;
  for (i = 0; i < dataCollection.inputs.spectraData.size(); i++){
    if (dataCollection.inputs.spectraData[i].id.compare(spectraData_ref) == 0){
      return dataCollection.inputs.spectraData[i];
    }
  }
  CSpectraData blank;
  return blank;
}

//Gets the pointer to the requested list, or returns null if bad reference is requested.
CSpectrumIdentificationList* CMzIdentML::getSpectrumIdentificationList(string& spectrumIdentificationList_ref){
  size_t i;
  for (i = 0; i < dataCollection.analysisData.spectrumIdentificationList.size(); i++){
    if (dataCollection.analysisData.spectrumIdentificationList[i].id.compare(spectrumIdentificationList_ref) == 0){
      return &dataCollection.analysisData.spectrumIdentificationList[i];
    }
  }
  return NULL;
}

//Gets the pointer to the requested protocol, or returns null if bad reference is requested.
CSpectrumIdentificationProtocol* CMzIdentML::getSpectrumIdentificationProtocol(string& spectrumIdentificationProtocol_ref){
  size_t i;
  for (i = 0; i < analysisProtocolCollection.spectrumIdentificationProtocol.size(); i++){
    if (analysisProtocolCollection.spectrumIdentificationProtocol[i].id.compare(spectrumIdentificationProtocol_ref) == 0){
      return &analysisProtocolCollection.spectrumIdentificationProtocol[i];
    }
  }
  return NULL;
}

//This is too slow to be practical.
CSpectrumIdentificationResult* CMzIdentML::getSpectrumIdentificationResultBySpectrumID(std::string& spectrumIdentificationList_ref, std::string& spectrumIdentificationResult_spectrumID){
  CSpectrumIdentificationList* m_sil=getSpectrumIdentificationList(spectrumIdentificationList_ref);
  if(m_sil==NULL) return NULL;

  size_t i;
  for (i = 0; i < m_sil->spectrumIdentificationResult.size(); i++){
    if (m_sil->spectrumIdentificationResult[i].spectrumID.compare(spectrumIdentificationResult_spectrumID) == 0){
      return &m_sil->spectrumIdentificationResult[i];
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
    analysisProtocolCollection.spectrumIdentificationProtocol.back().additionalSearchParams.back().cvParam.push_back(cv);
    break;
  case AmbiguousResidue:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().massTable.back().ambiguousResidue.back().cvParam.push_back(cv);
    break;
  case AnalysisParams:
    analysisProtocolCollection.proteinDetectionProtocol.back().analysisParams.back().cvParam.push_back(cv);
    break;
  case DBSequence:
    sequenceCollection.dbSequence.back().cvParam.push_back(cv);
    break;
  case EnzymeName:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().enzymes.back().enzyme.back().enzymeName.back().cvParam.push_back(cv);
    break;
  case FileFormat:
    if (e == SpectraData){
      dataCollection.inputs.spectraData.back().fileFormat.cvParam=cv;
    } else if (e == SearchDatabase){
      dataCollection.inputs.searchDatabase.back().fileFormat.cvParam=cv;
    } else if (e == SourceFile){
      dataCollection.inputs.sourceFile.back().fileFormat.cvParam = cv;
    } else {
      cout << "FileFormat::cvParam not recorded." << endl;
    }
    break;
  case FilterType:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().databaseFilters.back().filter.back().filterType.cvParam=cv;
    break;
  case FragmentTolerance:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().fragmentTolerance.back().cvParam.push_back(cv);
    break;
  case Include:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().databaseFilters.back().filter.back().include.back().cvParam.push_back(cv);
    break;
  case IonType:
    dataCollection.analysisData.spectrumIdentificationList.back().spectrumIdentificationResult.back().spectrumIdentificationItem.back().fragmentation.back().ionType.back().cvParam.push_back(cv);
    break;
  case MassTable:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().massTable.back().cvParam.push_back(cv);
    break;
  case Measure:
    dataCollection.analysisData.spectrumIdentificationList.back().fragmentationTable.back().measure.back().cvParam.push_back(cv);
    break;
  case Modification:
    sequenceCollection.peptide.back().modification.back().cvParam.push_back(cv);
    break;
  case Organization:
    auditCollection.back().organization.back().cvParam.push_back(cv);
    break;
  case ParentTolerance:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().parentTolerance.back().cvParam.push_back(cv);
    break;
  case Person:
    auditCollection.back().person.back().cvParam.push_back(cv);
    break;
  case ProteinAmbiguityGroup:
    dataCollection.analysisData.proteinDetectionList.back().proteinAmbiguityGroup.back().cvParam.push_back(cv);
    break;
  case ProteinDetectionHypothesis:
    dataCollection.analysisData.proteinDetectionList.back().proteinAmbiguityGroup.back().proteinDetectionHypothesis.back().cvParam.push_back(cv);
    break;
  case ProteinDetectionList:
    dataCollection.analysisData.proteinDetectionList.back().cvParam.push_back(cv);
    break;
  case Role:
    if(activeEl.size()>2 && activeEl[activeEl.size()-3]==AnalysisSoftware){
      analysisSoftwareList.analysisSoftware.back().contactRole.back().role.cvParam=cv;
    } else if (activeEl.size()>2 && activeEl[activeEl.size() - 3] == Provider){
      provider.contactRole.back().role.cvParam=cv;
    } else {
      cout << "Role::cvParam not recorded." << endl;
    }
    break;
  case Sample:
    analysisSampleCollection.back().sample.back().cvParam.push_back(cv);
    break;
  case SearchDatabase:
    dataCollection.inputs.searchDatabase.back().cvParam.push_back(cv);
    break;
  case SearchModification:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().modificationParams.back().searchModification.back().cvParam.push_back(cv);
    break;
  case SearchType:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().searchType.cvParam=cv;
    break;
  case SoftwareName:
    analysisSoftwareList.analysisSoftware.back().softwareName.cvParam=cv;
    break;
  case SpecificityRules:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().modificationParams.back().searchModification.back().specificityRules.back().cvParam.push_back(cv);
    break;
  case SpectrumIDFormat:
    dataCollection.inputs.spectraData.back().spectrumIDFormat.cvParam=cv;
    break;
  case SpectrumIdentificationItem:
    dataCollection.analysisData.spectrumIdentificationList.back().spectrumIdentificationResult.back().spectrumIdentificationItem.back().addCvParam(cv);
    break;
  case SpectrumIdentificationResult:
    dataCollection.analysisData.spectrumIdentificationList.back().spectrumIdentificationResult.back().cvParam.push_back(cv);
    break;
  case Threshold:
    if (e == SpectrumIdentificationProtocol){
      analysisProtocolCollection.spectrumIdentificationProtocol.back().threshold.cvParam.push_back(cv);
    } else if (e == ProteinDetectionProtocol){
      analysisProtocolCollection.proteinDetectionProtocol.back().threshold.cvParam.push_back(cv);
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
    analysisProtocolCollection.spectrumIdentificationProtocol.back().additionalSearchParams.back().userParam.push_back(u);
    break;
  case AmbiguousResidue:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().massTable.back().ambiguousResidue.back().userParam.push_back(u);
    break;
  case DatabaseName:
    dataCollection.inputs.searchDatabase.back().databaseName.userParam=u;
    break;
  case EnzymeName:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().enzymes.back().enzyme.back().enzymeName.back().userParam.push_back(u);
    break;
  case IonType:
    dataCollection.analysisData.spectrumIdentificationList.back().spectrumIdentificationResult.back().spectrumIdentificationItem.back().fragmentation.back().ionType.back().userParam.push_back(u);
    break;
  case MassTable:
    analysisProtocolCollection.spectrumIdentificationProtocol.back().massTable.back().userParam.push_back(u);
    break;
  case Organization:
    auditCollection.back().organization.back().userParam.push_back(u);
    break;
  case Person:
    auditCollection.back().person.back().userParam.push_back(u);
    break;
  case ProteinAmbiguityGroup:
    dataCollection.analysisData.proteinDetectionList.back().proteinAmbiguityGroup.back().userParam.push_back(u);
    break;
  case ProteinDetectionHypothesis:
    dataCollection.analysisData.proteinDetectionList.back().proteinAmbiguityGroup.back().proteinDetectionHypothesis.back().userParam.push_back(u);
    break;
  case SoftwareName:
    analysisSoftwareList.analysisSoftware.back().softwareName.userParam=u;
    break;
  case SpectrumIdentificationItem:
    dataCollection.analysisData.spectrumIdentificationList.back().spectrumIdentificationResult.back().spectrumIdentificationItem.back().userParam.push_back(u);
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
  analysisSoftwareList.analysisSoftware.clear();

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

  sequenceCollection.rebuildDBTable();
  sequenceCollection.rebuildPepEvTable();
  sequenceCollection.rebuildPepTable();

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
  if(versionStr.size()==0){
    if (version == 1) fprintf(f, " version=\"%s\"  xsi:schemaLocation=\"%s\" xmlns = \"%s\"", mzIdentMLv1, mzIdentMLv1schema, mzIdentMLv1xmlns);
    else fprintf(f, " version=\"%s\"  xsi:schemaLocation=\"%s\" xmlns = \"%s\"", mzIdentMLv2, mzIdentMLv2schema, mzIdentMLv2xmlns);
  } else {
    fprintf(f, " version=\"%s\"",versionStr.c_str());
    fprintf(f," xmlns=\"%s\"",xmlns.c_str());
    if (schema.size() == 0)  fprintf(f, " xsi:schemaLocation=\"http://psidev.info/psi/pi/mzIdentML/1.1 http://www.psidev.info/files/mzIdentML1.1.0.xsd\"");
    else fprintf(f, " xsi:schemaLocation=\"%s\"",schema.c_str());
  }
  fprintf(f, " xmlns:xsi = \"http://www.w3.org/2001/XMLSchema-instance\" creationDate = \"%s\">\n",timebuf);

  cvList.writeOut(f,1);
  analysisSoftwareList.writeOut(f,1);
  if(provider.id.size()>0) provider.writeOut(f,1);
  if(auditCollection.size()>0) auditCollection[0].writeOut(f,1);
  if (analysisSampleCollection.size()>0) analysisSampleCollection[0].writeOut(f, 1);
  sequenceCollection.writeOut(f,1);
  analysisCollection.writeOut(f,1);
  analysisProtocolCollection.writeOut(f,1);
  dataCollection.writeOut(f,1);

  fprintf(f, "</MzIdentML>\n");
  fclose(f);
  return true;

}