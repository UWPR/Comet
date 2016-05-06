#include "pepXMLWriter.h"

PepXMLWriter::PepXMLWriter(){
  index=0;
  iTabs=0;
  bTabs=true;
  spQueryIndex=0;
  fptr=NULL;
}

PepXMLWriter::~PepXMLWriter(){
}

void PepXMLWriter::addTab(){
  strTabs[iTabs++]=' ';
  strTabs[iTabs]='\0';
}

void PepXMLWriter::closePepXML(){
  while(vTagState.size()>0){
    deleteTab();
    writeLine(&vTagState.back()[0]);
    vTagState.pop_back();
  }
  fclose(fptr);
}

bool PepXMLWriter::createPepXML(char* fn, pxwMSMSRunSummary& run, PXWSearchSummary* search){

  string st;
  spQueryIndex=1;

  if(fptr!=NULL) fclose(fptr);
  fptr=fopen(fn,"wt");
  if(fptr==NULL) return false;

  resetTabs();
  writeLine("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

  writeLine("<msms_pipeline_analysis>\n");
  addTab();
  st="</msms_pipeline_analysis>\n";
  vTagState.push_back(st);

  st="<msms_run_summary base_name=\"";
  st+=run.base_name;
  st+="\" raw_data_type=\"";
  st+=run.raw_data_type;
  st+="\" raw_data=\"";
  st+=run.raw_data;
  st+="\">\n";
  writeLine(&st[0]);
  addTab();
  st="</msms_run_summary>\n";
  vTagState.push_back(st);

  if(search!=NULL){
    //pass to searchSummary eventually
    st="<search_summary base_name=\"";
    st+=search->base_name;
    st+="\" search_engine=\"";
    st+=search->search_engine;
    st+="\">\n";
    writeLine(&st[0]);
    st="</search_summary>\n";
    writeLine(&st[0]);
  }

  return true;

}

void PepXMLWriter::deleteTab(){
  iTabs--;
  strTabs[iTabs]='\0';
}

void PepXMLWriter::resetTabs(){
  iTabs=0;
  strTabs[iTabs]='\0';
}

void PepXMLWriter::writeAltProtein(pxwProtein& s){
  string st;

  st="<alternative_protein protein=\"";
  st+=s.protein;
  st+="\" peptide_prev_aa=\"";
  st+=s.peptide_prev_aa;
  st+="\" peptide_next_aa=\"";
  st+=s.peptide_next_aa;
  st+="\"/>\n";
  writeLine(&st[0]);
}

void PepXMLWriter::writeModAAMass(pxwModAA& s){
  string st;
  char nStr[32];

  st="<mod_aminoacid_mass position=\"";
  sprintf(nStr,"%d",s.position);
  st+=nStr;
  st+="\" mass=\"";
  sprintf(nStr,"%.6lf",s.mass);
  st+=nStr;
  st+="\"/>\n";
  writeLine(&st[0]);

}

void PepXMLWriter::writeModInfo(PXWModInfo& s){
  string st;
  char nStr[64];
  size_t i;

  st="<modification_info";
  if(s.modified_peptide.size()>0){
    st+=" modified_peptide=\"";
    st+=s.modified_peptide;
    st+="\"";
  }
  if(s.mod_nterm_mass>0){
    sprintf(nStr," mod_nterm_mass=\"%.4lf\"",s.mod_nterm_mass);
    st+=nStr;
  }
  if(s.mod_cterm_mass>0){
    sprintf(nStr," mod_cterm_mass=\"%.4lf\"",s.mod_cterm_mass);
    st+=nStr;
  }
  st+=">\n";
  writeLine(&st[0]);
  addTab();

  for(i=0;i<s.sizeMods();i++){
    writeModAAMass(s.getMod(i));
  }

  deleteTab();
  st="</modification_info>\n";
  writeLine(&st[0]);
}

void PepXMLWriter::writeLine(char* str){
  if(bTabs) fprintf(fptr,"%s",strTabs);
  fprintf(fptr,"%s",str);
}

void PepXMLWriter::writeLinkedPeptide(PXWSearchHit& s, bool alpha){
  string st;
  char nStr[32];
  size_t i;

  st="<linked_peptide peptide=\"";
  st+=s.peptide;
  st+="\" peptide_prev_aa=\"";
  if(s.sizeProteins()>0) st+=s.getProtein(0).peptide_prev_aa;
  else st+="-";
  st+="\" peptide_next_aa=\"";
  if(s.sizeProteins()>0) st+=s.getProtein(0).peptide_next_aa;
  else st+="-";
  st+="\" protein=\"";
  if(s.sizeProteins()>0) st+=s.getProtein(0).protein;
  else st+="unknown";
  st+="\" num_tot_proteins=\"";
  sprintf(nStr,"%d",s.num_tot_proteins);
  st+=nStr;
  st+="\" calc_neutral_pep_mass=\"";
  sprintf(nStr,"%.6lf",s.calc_neutral_pep_mass);
  st+=nStr;
  st+="\" complement_mass=\"";
  sprintf(nStr,"%.6lf",s.massdiff);
  st+=nStr;
  if(alpha) st+="\" designation=\"alpha\">\n";
  else st+="\" designation=\"beta\">\n";
  writeLine(&st[0]);
  addTab();

  for(i=1;i<s.sizeProteins();i++){
    writeAltProtein(s.getProtein(i));
  }

  if(s.modInfo.sizeMods()>0 || s.modInfo.mod_cterm_mass>0 || s.modInfo.mod_nterm_mass>0){
    writeModInfo(s.modInfo);
  }

  for(i=0;i<s.sizeXLScores();i++){
    st="<xlink_score name=\"";
    st+=s.getXLScore(i).name;
    st+="\" value=\"";
    st+=s.getXLScore(i).value;
    st+="\"/>\n";
    writeLine(&st[0]);
  }

  deleteTab();
  writeLine("</linked_peptide>\n");
      
}

void PepXMLWriter::writeSearchHit(pxwSearchHitPair& s) {
  string st;
  char nStr[32];
  size_t i;

  bool bCross=false;
  if(s.a->xlink_type.compare("xl")==0) bCross=true;

  st="<search_hit hit_rank=\"";
  sprintf(nStr,"%d",s.a->hit_rank);
  st+=nStr;
  st+="\" peptide=\"";
  if(bCross) st+="-";
  else st+=s.a->peptide;
  st+="\" peptide_prev_aa=\"";
  if(s.a->sizeProteins()>0 && !bCross) st+=s.a->getProtein(0).peptide_prev_aa;
  else st+="-";
  st+="\" peptide_next_aa=\"";
  if(s.a->sizeProteins()>0 && !bCross) st+=s.a->getProtein(0).peptide_next_aa;
  else st+="-";
  st+="\" protein=\"";
  if(s.a->sizeProteins()>0 && !bCross) st+=s.a->getProtein(0).protein;
  else st+="-";
  st+="\" num_tot_proteins=\"";
  sprintf(nStr,"%d",s.a->num_tot_proteins);
  st+=nStr;
  st+="\" calc_neutral_pep_mass=\"";
  if(bCross) sprintf(nStr,"%.6lf",s.a->calc_neutral_xl_mass);
  else sprintf(nStr,"%.6lf",s.a->calc_neutral_pep_mass);
  st+=nStr;
  st+="\" massdiff=\"";
  if(bCross) sprintf(nStr,"%.6lf",s.a->xl_massdiff);
  else sprintf(nStr,"%.6lf",s.a->massdiff);
  st+=nStr;
  st+="\" xlink_type=\"";
  st+=s.a->xlink_type;
  st+="\">\n";
  writeLine(&st[0]);
  addTab();

  if(!bCross){
    for(i=1;i<s.a->sizeProteins();i++){
      writeAltProtein(s.a->getProtein(i));
    }

    if(s.a->modInfo.sizeMods()>0 || s.a->modInfo.mod_cterm_mass>0 || s.a->modInfo.mod_nterm_mass>0){
      writeModInfo(s.a->modInfo);
    }
  }

  //loop or cross-link info
  if(s.a->xlink_type.compare("na")!=0){
    st="<xlink identifier=\"";
    st+=s.identifier;
    st+="\" mass=\"";
    sprintf(nStr,"%.6lf",s.mass);
    st+=nStr;
    st+="\">\n";
    writeLine(&st[0]);
    addTab();

    if(bCross) {
      writeLinkedPeptide(*s.a,true);
      writeLinkedPeptide(*s.b,false);
    } else {
      for(i=0;i<s.a->sizeXLScores();i++){
        st="<xlink_score name=\"";
        st+=s.a->getXLScore(i).name;
        st+="\" value=\"";
        st+=s.a->getXLScore(i).value;
        st+="\"/>\n";
        writeLine(&st[0]);
      }
    }

    deleteTab();
    writeLine("</xlink>\n");
  }

  for(i=0;i<s.a->sizeScores();i++){
    st="<search_score name=\"";
    st+=s.a->getScore(i).name;
    st+="\" value=\"";
    st+=s.a->getScore(i).value;
    st+="\"/>\n";
    writeLine(&st[0]);
  }
  
  deleteTab();
  writeLine("</search_hit>\n");

}

void PepXMLWriter::writeSpectrumQuery(PXWSpectrumQuery& s){
  string st;
  char nStr[32];
  
  st="<spectrum_query spectrum=\"";
  st+=s.spectrum;
  st+="\" start_scan=\"";
  sprintf(nStr,"%d",s.start_scan);
  st+=nStr;
  st+="\" end_scan=\"";
  sprintf(nStr,"%d",s.end_scan);
  st+=nStr;
  st+="\" precursor_neutral_mass=\"";
  sprintf(nStr,"%.6lf",s.precursor_neutral_mass);
  st+=nStr;
  st+="\" assumed_charge=\"";
  sprintf(nStr,"%d",s.assumed_charge);
  st+=nStr;
  st+="\" index=\"";
  sprintf(nStr,"%d",spQueryIndex++);
  st+=nStr;
  if(s.retention_time_sec>0){ //retention time is optional
    st+="\" retention_time_sec=\"";
    sprintf(nStr,"%.1lf",s.retention_time_sec);
    st+=nStr;
  }
  st+="\">\n";
  writeLine(&st[0]);
  addTab();

  st="<search_result>\n";
  writeLine(&st[0]);
  addTab();

  for(size_t i=0;i<s.sizeSearchHits();i++){
    writeSearchHit(s.getSearchHit(i));
  }

  deleteTab();
  st="</search_result>\n";
  writeLine(&st[0]);

  deleteTab();
  st="</spectrum_query>\n";
  writeLine(&st[0]);

}
