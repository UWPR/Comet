#include "MSReader.h"
#include <iostream>
using namespace std;

MSReader::MSReader(){
  fileIn=NULL;
  rampFileIn=NULL;
  iIntensityPrecision=1;
  iMZPrecision=4;
  rampFileOpen=false;
  compressMe=false;
  rawFileOpen=false;
  exportMGF=false;
  highResMGF=false;
  iFType=0;
  iVersion=0;
  for(int i=0;i<16;i++)	strcpy(header.header[i],"\0");
  headerIndex=0;

  #ifndef _NOSQLITE
  db = NULL;
  lastIndex=-1;
  lastScanNumber=-1;
  #endif
}

MSReader::~MSReader(){
  closeFile();
  if(rampFileOpen) {
    rampCloseFile(rampFileIn);
    free(pScanIndex);
  }

  #ifndef _NOSQLITE
  if(db != NULL)  sqlite3_close(db);
  #endif
}

void MSReader::addFilter(MSSpectrumType m){
	filter.push_back(m);
}

void MSReader::closeFile(){
	if(fileIn!=NULL) fclose(fileIn);
	if(rampFileOpen) {
		rampCloseFile(rampFileIn);
		rampFileIn=NULL;
		rampFileOpen=false;
		free(pScanIndex);
	}
}

MSHeader& MSReader::getHeader(){
  return header;
}

/* 0 = File opened correctly
   1 = Could not open file
*/
int MSReader::openFile(const char *c,bool text){
	int i;

	if(text) fileIn=fopen(c,"rt");
	else fileIn=fopen(c,"rb");

  if(fileIn==NULL) {
		for(i=0;i<16;i++) strcpy(header.header[i],"\0");
    headerIndex=0;
    fileType=Unspecified;
    return 1;
  } else {
    fileType=Unspecified;

		//if we don't have the eof position, get it here.
		fseek(fileIn,0,2);
		lEnd=ftell(fileIn);

		lPivot = 0;
    lFWidth = lEnd/2;

		fseek(fileIn,0,0);

		if(text){
			for(i=0;i<16;i++) strcpy(header.header[i],"\0");
			headerIndex=0;
		} else {
      fread(&iFType,4,1,fileIn);
      fread(&iVersion,4,1,fileIn);
			fread(&header,sizeof(MSHeader),1,fileIn);
		}

	  return 0;
  }
}

MSSpectrumType MSReader::getFileType(){
  return fileType;
}


bool MSReader::readMSTFile(const char *c, bool text, Spectrum& s, int scNum){
  MSScanInfo ms;
  Peak_T p;
  ZState z;
  EZState ez;
  int i;

  //variables for text reading only
  bool firstScan = false;
  bool bScan = true;
  bool bDoneHeader = false;
  char tstr[256];
  char ch;
  char *tok;

  //variables for compressed files
  uLong mzLen, intensityLen;

  //clear any spectrum data
  s.clear();

  s.setCentroidStatus(2); //unknown if centroided with these formats.

  //check for valid file and if we can access it
  if(c!=NULL){
    closeFile();
    if(openFile(c,text)==1) return false;
    lastFileFormat = checkFileFormat(c);
  } else if(fileIn==NULL) {
    return false;
  }

  //set the filetype
  switch(lastFileFormat){
  case ms2:
  case cms2:
  case bms2:
    s.setFileType(MS2);
    break;
  case zs:
    s.setFileType(ZS);
    break;
  case uzs:
    s.setFileType(UZS);
    break;
  case ms1:
  case cms1:
  case bms1:
    s.setFileType(MS1);
    break;
  default:
    s.setFileType(Unspecified);
    break;
  }

	//Handle binary and text files differently
  if(!text){

    //if binary file, read scan info sequentially, skipping to next scan if requested
    //fread(&ms,sizeof(MSScanInfo),1,fileIn);
    readSpecHeader(fileIn,ms);

    if(scNum!=0){

      fseek(fileIn,sizeof(MSHeader)+8,0);

      //fread(&ms,sizeof(MSScanInfo),1,fileIn);
      readSpecHeader(fileIn,ms);

      while(ms.scanNumber[0]!=scNum){

        fseek(fileIn,ms.numZStates*12,1);
        fseek(fileIn,ms.numEZStates*20,1);

	      if(compressMe){
	        fread(&i,4,1,fileIn);
	        mzLen = (uLong)i;
	        fread(&i,4,1,fileIn);
	        intensityLen = (uLong)i;
	        fseek(fileIn,mzLen+intensityLen,1);
	      } else {
	        fseek(fileIn,ms.numDataPoints*12,1);
	      }

	      //fread(&ms,sizeof(MSScanInfo),1,fileIn);
        readSpecHeader(fileIn,ms);
	      if(feof(fileIn)) return false;
      }
    }
    if(feof(fileIn)) return false;

		//read any charge states (for MS2 files)
    for(i=0;i<ms.numZStates;i++){
      fread(&z.z,4,1,fileIn);
      fread(&z.mz,8,1,fileIn);
      s.addZState(z);
    }

    for(i=0;i<ms.numEZStates;i++){
      fread(&ez.z,4,1,fileIn);
      fread(&ez.mh,8,1,fileIn);
      fread(&ez.pRTime,4,1,fileIn);
      fread(&ez.pArea,4,1,fileIn);
      s.addEZState(ez);
    }

    s.setScanNumber(ms.scanNumber[0]);
    s.setScanNumber(ms.scanNumber[1],true);
    s.setRTime(ms.rTime);
		if(ms.mzCount==0) s.setMZ(0);
		for(i=0;i<ms.mzCount;i++){
			if(i==0) s.setMZ(ms.mz[i]);
			else s.addMZ(ms.mz[i]);
		}
    s.setBPI(ms.BPI);
    s.setBPM(ms.BPM);
    s.setConversionA(ms.convA);
    s.setConversionB(ms.convB);
		s.setConversionA(ms.convC);
    s.setConversionB(ms.convD);
		s.setConversionA(ms.convE);
    s.setConversionB(ms.convI);
    s.setIonInjectionTime(ms.IIT);
    s.setTIC(ms.TIC);

    //read compressed data to the spectrum object
    if(compressMe) {

      readCompressSpec(fileIn,ms,s);

      //or read binary data to the spectrum object
    } else {
      for(i=0;i<ms.numDataPoints;i++){
	      fread(&p.mz,8,1,fileIn);
	      fread(&p.intensity,4,1,fileIn);
	      //cout << p.mz << " " << p.intensity << endl;
	      s.add(p);
      }
    }

    //return success
    return true;

  } else {

    //if reading text files, some parsing is required.
    while(true){

      //stop when you reach the end of the file
      if(feof(fileIn)) {

        //Special case: when doing binary search, end of file might mean to search
        //the othere end of the file.
        if(scNum != 0){
	        if(s.getScanNumber() != scNum) {
	          bScan=findSpectrum(-1);
            s.clear();
	          s.setScanNumber(0);
	          if(bScan==false) return false;
          } else {
            break;
          }
	      } else {
          break;
        }
      }

      //scan next character in the file
      ch=fgetc(fileIn);
      ungetc(ch,fileIn);

      switch(ch){
      case 'D':
	      //D lines are ignored
	      fgets(tstr,256,fileIn);
	      break;

      case 'H':
	      //Header lines are recorded as strings up to 16 lines at 256 characters each
	      fgets(tstr,256,fileIn);
	      if(!bDoneHeader) {
	        tok=strtok(tstr," \t\n\r");
	        tok=strtok(NULL,"\n\r");
	        strcat(tok,"\n");
	        if(headerIndex<16) strcpy(header.header[headerIndex++],tok);
	        else cout << "Header too big!!" << endl;
	      }
	      break;

      case 'I':
	      //I lines are recorded only if they contain retention times
        fgets(tstr,256,fileIn);
        tok=strtok(tstr," \t\n\r");
        tok=strtok(NULL," \t\n\r");
        if(strcmp(tok,"RTime")==0) {
          tok=strtok(NULL," \t\n\r,");
          s.setRTime((float)atof(tok));
        }	else if(strcmp(tok,"TIC")==0) {
          tok=strtok(NULL," \t\n\r,");
          s.setTIC((float)atof(tok));
        }	else if(strcmp(tok,"IIT")==0) {
          tok=strtok(NULL," \t\n\r,");
          s.setIonInjectionTime((float)atof(tok));
        }	else if(strcmp(tok,"BPI")==0) {
          tok=strtok(NULL," \t\n\r,");
          s.setBPI((float)atof(tok));
        }	else if(strcmp(tok,"BPM")==0) {
          tok=strtok(NULL," \t\n\r,");
          s.setBPM((float)atof(tok));
        }	else if(strcmp(tok,"ConvA")==0) {
          tok=strtok(NULL," \t\n\r,");
          s.setConversionA(atof(tok));
        }	else if(strcmp(tok,"ConvB")==0) {
          tok=strtok(NULL," \t\n\r,");
          s.setConversionB(atof(tok));
        }	else if(strcmp(tok,"ConvC")==0) {
          tok=strtok(NULL," \t\n\r,");
          s.setConversionC(atof(tok));
        }	else if(strcmp(tok,"ConvD")==0) {
          tok=strtok(NULL," \t\n\r,");
          s.setConversionD(atof(tok));
        }	else if(strcmp(tok,"ConvE")==0) {
          tok=strtok(NULL," \t\n\r,");
          s.setConversionE(atof(tok));
        }	else if(strcmp(tok,"ConvI")==0) {
          tok=strtok(NULL," \t\n\r,");
          s.setConversionI(atof(tok));
        } else if(strcmp(tok,"EZ")==0) {
          tok=strtok(NULL," \t\n\r,");
          ez.z=atoi(tok);
          tok=strtok(NULL," \t\n\r,");
          ez.mh=atof(tok);
          tok=strtok(NULL," \t\n\r,");
          ez.pRTime=(float)atof(tok);
          tok=strtok(NULL," \t\n\r,");
          ez.pArea=(float)atof(tok);
          s.addEZState(ez);
        }
        break;

      case 'S':
	      //Scan numbers are recorded and mark all following data is spectrum data
	      //until the next tag

	      //Reaching an S tag also indicates there are no more header lines
	      bDoneHeader=true;

	      if(firstScan) {
	        //if we are here, a desired scan was read and we just reached the next scan tag
	        //therefore, stop reading further.
	        return true;

	      } else {
	        fgets(tstr,256,fileIn);
	        tok=strtok(tstr," \t\n\r");
	        tok=strtok(NULL," \t\n\r");
          s.setScanNumber(atoi(tok));
	        tok=strtok(NULL," \t\n\r");
	        s.setScanNumber(atoi(tok),true);
	        tok=strtok(NULL," \t\n\r");
	        if(tok!=NULL)	s.setMZ(atof(tok));
					else s.setMZ(0);
					tok=strtok(NULL," \t\n\r");
					while(tok!=NULL) {
						s.addMZ(atof(tok));
						tok=strtok(NULL," \t\n\r");
					}
	        if(scNum != 0){
	          if(s.getScanNumber() != scNum) {
	            if(s.getScanNumber()<scNum) bScan=findSpectrum(1);
	            else bScan=findSpectrum(-1);
              s.clear();
	            s.setScanNumber(0);
	            if(bScan==false) return false;
	            break;
	          }
	        }
	        firstScan=true;
	      }
	      break;

      case 'Z':
	      //Z lines are recorded for MS2 files

        //don't record z-lines unless this is a scan number that is wanted
        if(!firstScan){
	  fgets(tstr,256,fileIn);
 	  break;
	}
	      fgets(tstr,256,fileIn);
	      tok=strtok(tstr," \t\n\r");
	      tok=strtok(NULL," \t\n\r");
	      z.z=atoi(tok);
	      tok=strtok(NULL," \t\n\r");
	      z.mz=atof(tok);
	      s.addZState(z);
	      break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
	      //lines beginning with numbers are data; if they belong to a scan we are not
	      //interested in, we ignore them.
	      if(scNum != 0){
	        if(s.getScanNumber()!=scNum) {
	          fgets(tstr,256,fileIn);
	          break;
	        }
	      }
	      //otherwise, read in the line
	      fscanf(fileIn,"%lf %f\n",&p.mz,&p.intensity);
	      s.add(p);
	      break;

      default:
	      //if the character is not recognized, ignore the entire line.
        fgets(tstr,256,fileIn);
	      //fscanf(fileIn,"%s\n",tstr);
	      break;
      }
    }

  }

  return true;

}


bool MSReader::findSpectrum(int i){

  if(i==0){
    lPivot = lEnd/2;
    lFWidth = lPivot/2;
  } else if(i==-1){
    lPivot -= lFWidth;
    lFWidth /= 2;
  } else {
    lPivot += lFWidth;
    lFWidth /= 2;
  }

  fseek(fileIn,lPivot,0);
  return (lFWidth>0 && lPivot>0 && lPivot<lEnd);

}

int MSReader::getLastScan(){
  if(rampFileIn!=NULL){
    return (rampLastScan);
  }
  #ifndef _NOSQLITE
  if(db != 0){
    return lastScanNumber;
  }
  #endif
  return -1;
}

int MSReader::getPercent(){
	switch(lastFileFormat){
		case ms1:
		case ms2:
		case  zs:
		case uzs:
		case bms1:
		case bms2:
		case cms1:
		case cms2:
			if(fileIn!=NULL) {
				return (int)((double)ftell(fileIn)/lEnd*100);
			}
			break;
		case mzXML:
		case mz5:
		case mzML:
		case mzXMLgz:
		case mzMLgz:
			if(rampFileIn!=NULL){
				return (int)((double)rampIndex/rampLastScan*100);
			}
			break;
		case raw:
			#ifdef _MSC_VER
			if(cRAW.getStatus()){
				return (int)((double)cRAW.getLastScanNumber()/cRAW.getScanCount()*100);
			}
			#endif
			break;
		default:
			break;
  }
  return -1;
}

void MSReader::writeFile(const char* c, bool text, MSObject& m){

  FILE* fileOut;
  int i;

  //if a filename isn't specified, check to see if the
  //MSObject has a filename.
  if(c == NULL) {
    return;
  } else {
    if(text) fileOut=fopen(c,"wt");
    else fileOut=fopen(c,"wb");
  }

  //output file header lines;
  if(text){
    if(exportMGF){
      //MGF file header is here
      fprintf(fileOut,"COM=Generated in the MSToolkit\n");
      if(!highResMGF) fprintf(fileOut,"CHARGE=2+ and 3+\n");
    } else {
      //MSx/SQT file header is here
      for(i=0;i<16;i++){
        if(m.getHeader().header[i][0]!='\0') {
          fputs("H\t",fileOut);
          fputs(m.getHeader().header[i],fileOut);
        }
      }
    }
  } else {
    //version 0 or 1 has basic stats
    //version 2 adds BPI,BPM,TIC,IIT,ConvA,ConvB
    //version 3 adds EZ lines
		//version 4 adds ConvC,ConvD,ConvE,ConvI
		//version 5 adds MSX support (multiple mz values per spectrum)
    fwrite(&iFType,4,1,fileOut); //file type
    i=5;
    fwrite(&i,4,1,fileOut); //version number - in case we change formats
		fwrite(&m.getHeader(),sizeof(MSHeader),1,fileOut);
	}

	//output spectra;
  for(i=0;i<m.size();i++){

		//output spectrum header
		writeSpecHeader(fileOut,text,m.at(i));

		//output scan
		if(text){
			writeTextSpec(fileOut,m.at(i));
		} else if(compressMe){
			writeCompressSpec(fileOut,m.at(i));
		} else {
			writeBinarySpec(fileOut,m.at(i));
		}

  }

	fclose(fileOut);
}

void MSReader::writeFile(const char* c, MSFileFormat ff, MSObject& m, char* sha1Report){

  switch(ff){
  case mgf:
    exportMGF=true;
    setCompression(false);
    writeFile(c,true,m);
    exportMGF=false;
    break;
  case ms1:
  case ms2:
  case  zs:
  case uzs:
    exportMGF=false;
    setCompression(false);
    writeFile(c,true,m);
    break;
  case psm:
    #ifndef _NOSQLITE
    writeSqlite(c,m, sha1Report);
    #endif
    break;
  case mzXML:
  case mz5:
	case mzML:
	case mzXMLgz:
	case mzMLgz:
    cout << "Cannot write mzXML or mz5 or mzML formats. Nothing written." << endl;
    break;
  case bms1:
    exportMGF=false;
    setCompression(false);
    iFType=1;
    writeFile(c,false,m);
    break;
  case bms2:
    exportMGF=false;
    setCompression(false);
    iFType=3;
    writeFile(c,false,m);
    break;
  case cms1:
    exportMGF=false;
    setCompression(true);
    iFType=2;
    writeFile(c,false,m);
    break;
  case cms2:
    exportMGF=false;
    setCompression(true);
    iFType=4;
    writeFile(c,false,m);
    break;
  default:
    cout << "Unknown file format. Nothing written." << endl;
    break;
  }

}

#ifndef _NOSQLITE
void MSReader::writeSqlite(const char* c, MSObject& m, char* sha1Report)
{

  //open the database for write
  sqlite3_open(c, &db);
  if(db == 0)
    {
      cout<<"Error open database "<<c<<endl;
      return;
    }

  sql_stmt("PRAGMA synchronous=OFF");
  sql_stmt("PRAGMA cache_size=750000");
  sql_stmt("PRAGMA temp_store=MEMORY");

  //create two tables (msRun and msScan)
  char zSql[8192];

  strcpy(zSql, "create table msRun(id INTEGER primary key autoincrement not null,"
	 "filename VARCHAR(150), sha1Sum VARCHAR(100), creationTime VARCHAR(255), extractor VARCHAR(255),"
	 "extractorVersion VARCHAR(100), instrumentType VARCHAR(100), instrumentVendor TEXT, instrumentSN TEXT,"
	 "acquisitionMethod TEXT, originalFileType TEXT, separateDigestion CHAR(1), uploadDate TEXT, comment TEXT)");
  sql_stmt(zSql);
  zSql[0]='\0';

  strcpy(zSql,"create table msScan(id INTEGER primary key autoincrement not null,"
	 "runID INTEGER, startScanNumber INTEGER, endScanNumber INTEGER, level INTEGER, precursorMZ REAL, precursorCharge INTEGER,"
	 "preScanID INTEGER, preScanNumber INTEGER, retentionTime REAL, fragmentationType VARCHAR(100),isCentroid CHAR(1), peakCount INTEGER)");
  sql_stmt(zSql);
  zSql[0]='\0';

  strcpy(zSql, "create table msScanData(scanID INTEGER, peakMZ BLOB, peakIntensity BLOB)");
  sql_stmt(zSql);
  zSql[0]='\0';

  strcpy(zSql,"create table MS2FileScanCharge(id INTEGER primary key autoincrement not null,"
	 "scanID INTEGER, charge INTEGER, mass REAL)");
  sql_stmt(zSql);
  zSql[0]='\0';

  //get the creationTime, sha1Sum etc.
  string fileCreateTime="=";
  string instrumentType="=";
  for(int i=0; i<16; i++)
    {
      if(m.getHeader().header[i] != '\0')
	{
	  string headerLine = m.getHeader().header[i];
	  if(headerLine.find("CreationDate") != string::npos)
	    {
	      size_t pos = headerLine.find('\t');
	      fileCreateTime = headerLine.substr(pos+1);
	    }
	  if(headerLine.find("InstrumentType") != string::npos)
	    {
	      size_t pos = headerLine.find('\t');
	      instrumentType = headerLine.substr(pos+1);
	    }
	}
    }


  //insert into table msRun
  sprintf(zSql,"insert into msRun(filename, sha1Sum, creationTime, extractor,extractorVersion, instrumentType) values('%s','%s','%s','%s','%s','%s')",
	  c,
	  sha1Report,
	  fileCreateTime.c_str(),
	  "MakeMS2",
	  "1.0",
	  instrumentType.c_str());

  sql_stmt(zSql);

}
#endif

void MSReader::appendFile(char* c, bool text, Spectrum& s){
  FILE* fileOut;

  if(c == NULL) return;

  if(text)fileOut=fopen(c,"at");
  else fileOut=fopen(c,"ab");

  //output spectrum header
  writeSpecHeader(fileOut,text,s);

  //output spectrum
  if(text){
    writeTextSpec(fileOut,s);
  } else if(compressMe){
    writeCompressSpec(fileOut,s);
  } else {
    writeBinarySpec(fileOut,s);
  }

  fclose(fileOut);

}

void MSReader::appendFile(char* c, Spectrum& s){
  MSFileFormat ff;
  FILE* fileOut;

  if(c == NULL) return;
  ff=checkFileFormat(c);

  switch(ff){
  case mgf:
    exportMGF=true;
    fileOut=fopen(c,"at");
    writeTextSpec(fileOut,s);
    fclose(fileOut);
    exportMGF=false;
    break;
  case ms1:
  case ms2:
  case  zs:
  case uzs:
    fileOut=fopen(c,"at");
    writeSpecHeader(fileOut,true,s);
    writeTextSpec(fileOut,s);
    fclose(fileOut);
    break;
  case bms1:
  case bms2:
    fileOut=fopen(c,"ab");
    writeSpecHeader(fileOut,false,s);
    writeBinarySpec(fileOut,s);
    fclose(fileOut);
    break;
  case cms1:
  case cms2:
    fileOut=fopen(c,"ab");
    writeSpecHeader(fileOut,false,s);
    writeCompressSpec(fileOut,s);
    fclose(fileOut);
    break;
  case psm:
    #ifndef _NOSQLITE
    appendFile(s);
    #endif
    break;
  default:
    cout << "Cannot append file: unknown or unsupported file type." << endl;
    break;
  }

}

//private function for insert a scan into msScan table
#ifndef _NOSQLITE
void MSReader::appendFile(Spectrum& s)
{

  if(db == 0)
    return;

  int j;

  //file compression
  int err;
  uLong len;
  unsigned char *comprM, *comprI;
  uLong comprLenM, comprLenI;
  double *pD;
  float *pF;
  uLong sizeM;
  uLong sizeI;

  //Build arrays to hold scan prior to compression

  pD = new double[s.size()];
  pF = new float[s.size()];
  for(j=0;j<s.size();j++){
    pD[j]=s.at(j).mz;
    pF[j]=s.at(j).intensity;
  }

  //compress mz
  len = (uLong)s.size()*sizeof(double);
  sizeM = len;
  comprLenM = compressBound(len);
  comprM = (unsigned char*)calloc((uInt)comprLenM, 1);
  err = compress(comprM, &comprLenM, (const Bytef*)pD, len);

  //compress intensity
  len = (uLong)s.size()*sizeof(float);
  sizeI = len;
  comprLenI = compressBound(len);
  comprI = (unsigned char*)calloc((uInt)comprLenI, 1);
  err = compress(comprI, &comprLenI, (const Bytef*)pF, len);

   //insert into table
  char zSql[8192];
  int charge;

  vector<int> chgs;
  if(s.getMsLevel() >1)
    {
      chgs = estimateCharge(s);
    }

  if(s.sizeZ() == 1)
    charge = s.atZ(0).z;
  else if(s.sizeZ() > 1)
    charge = 0;
  else
    {
      if(chgs.size() == 1)
	charge = chgs.at(0);
      else
	charge = 0;
    }


  MSActivation act = s.getActivationMethod();
  string actMethod;
  switch(act){
  case mstETD:
    actMethod="ETD";
    break;
  case mstCID:
    actMethod="CID";
    break;
  case mstECD:
    actMethod="ECD";
    break;
  case mstPQD:
    actMethod = "PQD";
    break;
  case mstHCD:
    actMethod = "HCD";
    break;
  case mstNA:
  default:
    actMethod="UNKNOWN";
    break;
  }


  sprintf(zSql, "insert into msScan(runID,startScanNumber,endScanNumber,level,precursorMZ, precursorCharge,retentionTime,fragmentationType,peakCount) "
	  "values (1,%d, %d,%d, %f, %d, %f,'%s', %d)",
          s.getScanNumber(),
	  s.getScanNumber(true),
	  s.getMsLevel(),
          s.getMZ(),
          charge,
          s.getRTime(),
	  actMethod.c_str(),
          s.size());

  sql_stmt(zSql);
  zSql[0]='\0';

  //get scanID
  strcpy(zSql, "select MAX(id) from msScan");
  int rc,iRow, iCol;
  char** result;

  int lastScanID;
   rc = sqlite3_get_table(db, zSql, &result, &iRow, &iCol, 0);

   if(rc == SQLITE_OK)
     {
       lastScanID=atoi(result[1]);

     }
   else
     {
       cout<<"Can't execute the SQL statement"<<zSql<<endl;
     }

   zSql[0]='\0';

   //insert into msScanData
   sprintf(zSql, "insert into msScanData values(%d, ?, ?)",
	   lastScanID);

  sqlite3_stmt *pStmt;


  rc = sqlite3_prepare(db, zSql, -1, &pStmt, 0);
  if( rc!=SQLITE_OK ){
    cout<<"can't prepare SQL statement!"<<rc<<endl;
    exit(1);
  }


  sqlite3_bind_blob(pStmt, 1, comprM, (int)comprLenM, SQLITE_STATIC);
  sqlite3_bind_blob(pStmt, 2, comprI, (int)comprLenI, SQLITE_STATIC);

  rc = sqlite3_step(pStmt);
  rc = sqlite3_finalize(pStmt);

  free(comprM);
  free(comprI);
  delete [] pD;
  delete [] pF;

  zSql[0]='\0';
  //insert into MS2FileScanCharge
  int chg;
  double MH;
  if(s.getMsLevel() > 1)
    {
      if(s.sizeZ() > 0)
	{
	  for(int i=0; i<s.sizeZ(); i++)
	    {
	      chg = s.atZ(i).z;
	      MH = s.getMZ()*chg-(chg-1)*1.008;
	      sprintf(zSql, "insert into MS2FileScanCharge(scanID, charge, mass) values(%d, %d, %f)",
		     lastScanID,
		     chg,
		     MH);
	      sql_stmt(zSql);
	    }

	}
      else
	{

	  for(unsigned int i=0; i<chgs.size(); i++)
	    {
	      chg=chgs.at(i);
	      MH = s.getMZ()*chg -(chg-1)*1.008;
	      sprintf(zSql, "insert into MS2FileScanCharge(scanID, charge, mass) values (%d, %d, %f)",
		     lastScanID,
		     chg,
		     MH);
	      sql_stmt(zSql);
	    }
	}
    }
}

vector<int> MSReader::estimateCharge(Spectrum& s)
{
  vector<int> chgs;
  float totalIntensity = s.getTotalIntensity();
  float beforeMZIntensity=0;

  double preMZ = s.getMZ();

  for(int i=0; i<s.size(); i++)
    {
      if(s.at(i).mz <= preMZ)
	beforeMZIntensity += s.at(i).intensity;
      else
	break;
    }

  if(beforeMZIntensity/totalIntensity >= 0.95)
    {
      chgs.push_back(1);
    }
  else
    {
      chgs.push_back(2);
      chgs.push_back(3);
    }
  return chgs;
}


void MSReader::createIndex()
{
  //create index for msScan table
  char* stmt1 = "create index idxScanNumber on msScan(startScanNumber)";
  sql_stmt(stmt1);

}
#endif

void MSReader::appendFile(char* c, bool text, MSObject& m){

  FILE* fileOut;
  int i;

  //if a filename isn't specified, check to see if the
  //MSObject has a filename.
  if(c == NULL) {
		return;
  } else {
		if(text) fileOut=fopen(c,"at");
		else fileOut=fopen(c,"ab");
	}

  //output spectra;
  for(i=0;i<m.size();i++){

		//output spectrum header
		writeSpecHeader(fileOut,text,m.at(i));

		//output spectrum
		if(text){
			writeTextSpec(fileOut,m.at(i));
		} else if(compressMe){
			writeCompressSpec(fileOut,m.at(i));
		} else {
			writeBinarySpec(fileOut,m.at(i));
		}

	}

	fclose(fileOut);
}

void MSReader::appendFile(char* c, MSObject& m){

  MSFileFormat ff;
  FILE* fileOut;
  int i;

  if(c == NULL) return;
  ff=checkFileFormat(c);

  switch(ff){
    case mgf:
      exportMGF=true;
      fileOut=fopen(c,"at");
      for(i=0;i<m.size();i++) writeTextSpec(fileOut,m.at(i));
      fclose(fileOut);
      exportMGF=false;
      break;
    case ms1:
    case ms2:
    case  zs:
    case uzs:
	    fileOut=fopen(c,"at");
      for(i=0;i<m.size();i++){
        writeSpecHeader(fileOut,true,m.at(i));
        writeTextSpec(fileOut,m.at(i));
      }
      fclose(fileOut);
      break;
    case bms1:
    case bms2:
      fileOut=fopen(c,"ab");
      for(i=0;i<m.size();i++){
        writeSpecHeader(fileOut,false,m.at(i));
        writeBinarySpec(fileOut,m.at(i));
      }
      fclose(fileOut);
      break;
    case cms1:
    case cms2:
      fileOut=fopen(c,"ab");
      for(i=0;i<m.size();i++){
        writeSpecHeader(fileOut,false,m.at(i));
        writeCompressSpec(fileOut,m.at(i));
      }
      fclose(fileOut);
      break;
    default:
      cout << "Cannot append file: unknown or unsupported file type." << endl;
      break;
  }

}

void MSReader::setPrecision(int i, int j){
  iIntensityPrecision=i;
  iMZPrecision=j;
}

void MSReader::setPrecisionInt(int i){
  iIntensityPrecision=i;
}

void MSReader::setPrecisionMZ(int i){
  iMZPrecision=i;
}

bool MSReader::readFile(const char* c, Spectrum& s, int scNum){

  if(c!=NULL) lastFileFormat = checkFileFormat(c);
  switch(lastFileFormat){
		case ms1:
		case ms2:
		case  zs:
		case uzs:
			return readMSTFile(c,true,s,scNum);
			break;
		case bms1:
		case bms2:
			setCompression(false);
			return readMSTFile(c,false,s,scNum);
			break;
		case cms1:
		case cms2:
			setCompression(true);
			return readMSTFile(c,false,s,scNum);
			break;
		case mzXML:
		case mz5:
		case mzML:
		case mzXMLgz:
		case mzMLgz:
			return readMZPFile(c,s,scNum);
			break;
		case raw:
			#ifdef _MSC_VER
			//only read the raw file if the dll was present and loaded.
			if(cRAW.getStatus()) {
				cRAW.setMSLevelFilter(&filter);
				return cRAW.readRawFile(c,s,scNum);
			} else {
				cerr << "Could not read Thermo RAW file. The Thermo .dll likely was not loaded." << endl;
				return false;
			}
			#else
				cerr << "Thermo RAW file format not supported." << endl;
				return false;
			#endif
			break;
		case sqlite:
		case psm:
			#ifndef _NOSQLITE
			return readSqlite(c,s,scNum);
			#else
			//sqlite support disabled
			cerr << "SQLite support disabled." << endl;
			return false;
			#endif
			break;
		case dunno:
		default:
			return false;
			break;
  }
	return false;

}

#ifndef _NOSQLITE
bool MSReader::readSqlite(const char* c, Spectrum& s, int scNum)
{

  if(c != NULL)
    {
      sqlite3_open(c, &db);
      if(db == 0)
	{
	  cout<<"Error open database "<<c<<endl;
	  return false;
	}

      sql_stmt("PRAGMA synchronous=OFF");
      sql_stmt("PRAGMA cache_size=750000");
      sql_stmt("PRAGMA temp_store=MEMORY");

      char zSql[1024];
      strcpy(zSql, "select MAX(id),MAX(startScanNumber) from msScan");
      int iRow, iCol, rc;
      char** result;


      rc = sqlite3_get_table(db, zSql, &result, &iRow, &iCol, 0);

      if(rc == SQLITE_OK)
        {
	  lastIndex = atoi(result[2]);
          lastScanNumber=atoi(result[3]);
	}

      curIndex = 0;
    }

  s.clear();

  char zSql[2048];
  int rc;


  if(scNum != 0)
    {
      //first get the curIndex

      if(scNum > lastScanNumber)
	{
	  cout<<"Specified scan number doesn't exist!"<<endl;
	  return false;
	}

      sprintf(zSql, "select id from msScan where startScanNumber=%d", scNum);
      int iRow, iCol;
      char** result;


      rc = sqlite3_get_table(db, zSql, &result, &iRow, &iCol, 0);

      if(rc == SQLITE_OK)
	{
	  curIndex = atoi(result[1]);
	  // curIndex++;
	}
      else
	{
	  cout<<"Can't execute the SQL statement"<<zSql<<endl;

	}

      sprintf(zSql, "select * from msScan, msScanData where startScanNumber=%d "
	      "AND id=scanID", scNum);
      if(!executeSqlStmt(s,zSql))
	cout<<scNum<<" can't be found in the database!"<<endl;

    }
  else
    {
      while(true)
	{
	  curIndex++;
	  if(curIndex > lastIndex)
	    return false;

	  sprintf(zSql, "select * from msScan, msScanData where id=%d "
		  "AND id=scanID",curIndex);
	  if(executeSqlStmt(s,zSql))
	    break;
	}


    }

  return true;

}

bool MSReader::executeSqlStmt(Spectrum& s, char* zSql)
{
  bool isSameLevel=false;
  sqlite3_stmt *pStmt;
  int rc;

  rc = sqlite3_prepare(db, zSql, -1, &pStmt, 0);

  if( rc!=SQLITE_OK ){
    cout<<"can't prepare SQL statement!"<<rc<<endl;
    exit(1);
  }

  rc = sqlite3_step(pStmt);

  char actMethod[1024];
  int charge=-1;
  int msLevel=-1;
  int scanID;
  MSSpectrumType msType;

  if( rc==SQLITE_ROW ){

    //add filter if filter set
    msLevel = sqlite3_column_int(pStmt,4);
    switch(msLevel){
    case 1:
      msType = MS1;
      break;
    case 2:
      msType = MS2;
      break;
    case 3:
      msType = MS3;
      break;
    default:
      break;
    }

    if(find(filter.begin(),filter.end(),msType) != filter.end())
      {
	scanID=sqlite3_column_int(pStmt,0);
	s.setScanID(sqlite3_column_int(pStmt,0));

	s.setScanNumber(sqlite3_column_int(pStmt,2));
	s.setScanNumber(sqlite3_column_int(pStmt,3),true);
	s.setMsLevel(msLevel);
	s.setMZ(sqlite3_column_double(pStmt,5));
	s.setCharge(sqlite3_column_int(pStmt,6));
	readChargeTable(scanID,s);
	s.setRTime((float)sqlite3_column_double(pStmt,9));

	strcpy(actMethod,reinterpret_cast<const char*>(sqlite3_column_text(pStmt,10)));
	if(strcmp(actMethod,"CID") == 0)
	  s.setActivationMethod(mstCID);
	if(strcmp(actMethod, "ETD") == 0)
	  s.setActivationMethod(mstETD);
	if(strcmp(actMethod, "ECD") == 0)
	  s.setActivationMethod(mstECD);
	if(strcmp(actMethod, "PQD") == 0)
	  s.setActivationMethod(mstPQD);
	if(strcmp(actMethod, "HCD") == 0)
	  s.setActivationMethod(mstHCD);
	if(strcmp(actMethod, "UNKNOWN") == 0)
	  s.setActivationMethod(mstNA);

	int numPeaks = sqlite3_column_int(pStmt,12);

	int numBytes1=sqlite3_column_bytes(pStmt,14);
	unsigned char* comprM = (unsigned char*)sqlite3_column_blob(pStmt,14);
	int numBytes2=sqlite3_column_bytes(pStmt,15);
	unsigned char* comprI = (unsigned char*)sqlite3_column_blob(pStmt,15);


	getUncompressedPeaks(s,numPeaks, numBytes1,comprM, numBytes2,comprI);
	isSameLevel = true;
      }

  }
  rc = sqlite3_finalize(pStmt);
  return isSameLevel;

}

void MSReader::readChargeTable(int scanID, Spectrum& s)
{
  char zSql[8192];
  sprintf(zSql, "select charge, mass from MS2FileScanCharge where scanID=%d", scanID);
  sqlite3_stmt *pStmt;
  int rc;

  rc = sqlite3_prepare(db, zSql, -1, &pStmt, 0);

  if( rc!=SQLITE_OK ){
    cout<<"can't prepare SQL statement!"<<rc<<endl;
    exit(1);
  }

  rc = sqlite3_step(pStmt);
  int charge;
  double MH;
  while(rc == SQLITE_ROW)
    {
      charge = sqlite3_column_int(pStmt,0);
      MH = sqlite3_column_double(pStmt,1);

      s.addZState(charge, MH);
      rc=sqlite3_step(pStmt);
    }
  rc = sqlite3_finalize(pStmt);
}



void MSReader::getUncompressedPeaks(Spectrum& s, int& numPeaks, int& mzLen, unsigned char* comprM, int& intensityLen, unsigned char* comprI)
{
  int i;


  //variables for compressed files
  uLong uncomprLen;
  double *mz;
  float *intensity;

  mz = new double[numPeaks];
  uncomprLen=numPeaks*sizeof(double);
  uncompress((Bytef*)mz, &uncomprLen, comprM, mzLen);

  intensity = new float[numPeaks];
  uncomprLen=numPeaks*sizeof(float);
  uncompress((Bytef*)intensity, &uncomprLen, comprI, intensityLen);

  for(i=0;i<numPeaks;i++){
    s.add(mz[i],intensity[i]);

  }
  delete [] mz;
  delete [] intensity;
}

void MSReader::sql_stmt(const char* stmt)
{

  char *errmsg;
  int   ret;

  ret = sqlite3_exec(db, stmt, 0, 0, &errmsg);

  if (ret != SQLITE_OK)
    {
      printf("Error in statement: %s [%s].\n", stmt, errmsg);
    }
}

#endif

bool MSReader::readMZPFile(const char* c, Spectrum& s, int scNum){

	ramp_fileoffset_t indexOffset;
	ScanHeaderStruct scanHeader;
	RAMPREAL *pPeaks;
	int i,j;

	if(c!=NULL) {
		//open the file if new file was requested
		if(rampFileOpen) closeFile();
		rampFileIn = rampOpenFile(c);
		if (rampFileIn == NULL) {
      cerr << "ERROR: Failure reading input file " << c << endl;
      return false;
		}
		rampFileOpen=true;

		//read the index
		indexOffset = getIndexOffset(rampFileIn);
		pScanIndex = readIndex(rampFileIn,indexOffset,&rampLastScan);
		rampIndex=0;

	} else {
		//if no new file requested, check to see if one is open already
		if (rampFileIn == NULL) return false;
	}

	//clear any spectrum data
	s.clear();

	MSSpectrumType mslevel;

	//read scan header
	if(scNum!=0) {
    rampIndex=scNum;
    readHeader(rampFileIn, pScanIndex[rampIndex], &scanHeader);
    if (scanHeader.acquisitionNum != scNum && scanHeader.acquisitionNum != -1) {
      cerr << "ERROR: Failure reading scan, index corrupted.  Line endings may have changed during transfer." << flush;
      exit(1);
    }
		switch(scanHeader.msLevel){
		case 1:
		  mslevel = MS1;
		  break;
		case 2:
		  mslevel = MS2;
		  break;
		case 3:
		  mslevel = MS3;
		  break;
		default:
		  break;
		}

		if(find(filter.begin(), filter.end(), mslevel) != filter.end())	{
      if(scanHeader.centroid) s.setCentroidStatus(1);
      else s.setCentroidStatus(0);
		  s.setMsLevel(scanHeader.msLevel);
		  s.setScanNumber(scanHeader.acquisitionNum);
		  s.setScanNumber(scanHeader.acquisitionNum,true);
		  s.setRTime((float)scanHeader.retentionTime/60.0f);
		  if(strlen(scanHeader.activationMethod)>1){
		    if(strcmp(scanHeader.activationMethod,"CID")==0) s.setActivationMethod(mstCID);
          else if(strcmp(scanHeader.activationMethod,"ECD")==0) s.setActivationMethod(mstECD);
          else if(strcmp(scanHeader.activationMethod,"ETD")==0) s.setActivationMethod(mstETD);
          else if(strcmp(scanHeader.activationMethod,"PQD")==0) s.setActivationMethod(mstPQD);
          else if(strcmp(scanHeader.activationMethod,"HCD")==0) s.setActivationMethod(mstHCD);
		    else s.setActivationMethod(mstNA);
		  }
			if(scanHeader.msLevel>1) {
				s.setMZ(scanHeader.precursorMZ);
				s.setCharge(scanHeader.precursorCharge);
			} else {
				s.setMZ(0);
			}
		  if(scanHeader.precursorCharge>0) s.addZState(scanHeader.precursorCharge,scanHeader.precursorMZ*scanHeader.precursorCharge-(scanHeader.precursorCharge-1)*1.007276466);
		  pPeaks = readPeaks(rampFileIn, pScanIndex[rampIndex]);
		  j=0;
		  for(i=0;i<scanHeader.peaksCount;i++){
		  	s.add((double)pPeaks[j],(float)pPeaks[j+1]);
			  j+=2;
		  }
		}	else {
		  return false;
		}

  } else /* if scnum == 0 */ {

		if(rampIndex>rampLastScan) return false;

		//read next index
	  while(true){
	    rampIndex++;
			if(pScanIndex[rampIndex]<0) continue;

	    //reached end of file
	    if(rampIndex>rampLastScan) return false;

			readHeader(rampFileIn, pScanIndex[rampIndex], &scanHeader);
			switch(scanHeader.msLevel){
			case 1:
			  mslevel = MS1;
			  break;
			case 2:
			  mslevel = MS2;
			  break;
			case 3:
			  mslevel = MS3;
			  break;
			default:
			  break;
			}
			if(find(filter.begin(), filter.end(), mslevel) != filter.end()) break;
		}

    if(scanHeader.centroid) s.setCentroidStatus(1);
    else s.setCentroidStatus(0);
		s.setMsLevel(scanHeader.msLevel);
		s.setScanNumber(scanHeader.acquisitionNum);
		s.setScanNumber(scanHeader.acquisitionNum,true);
		s.setRTime((float)scanHeader.retentionTime/60.0f);
		if(strlen(scanHeader.activationMethod)>1){
		  if(strcmp(scanHeader.activationMethod,"CID")==0) s.setActivationMethod(mstCID);
        else if(strcmp(scanHeader.activationMethod,"ECD")==0) s.setActivationMethod(mstECD);
        else if(strcmp(scanHeader.activationMethod,"ETD")==0) s.setActivationMethod(mstETD);
        else if(strcmp(scanHeader.activationMethod,"PQD")==0) s.setActivationMethod(mstPQD);
        else if(strcmp(scanHeader.activationMethod,"HCD")==0) s.setActivationMethod(mstHCD);
		  else s.setActivationMethod(mstNA);
		}
		if(scanHeader.msLevel>1) {
			s.setMZ(scanHeader.precursorMZ);
			s.setCharge(scanHeader.precursorCharge);
		} else {
			s.setMZ(0);
		}
		if(scanHeader.precursorCharge>0) s.addZState(scanHeader.precursorCharge,scanHeader.precursorMZ*scanHeader.precursorCharge-(scanHeader.precursorCharge-1)*1.007276466);
		pPeaks = readPeaks(rampFileIn, pScanIndex[rampIndex]);
		j=0;
		for(i=0;i<scanHeader.peaksCount;i++){
			s.add((double)pPeaks[j],(float)pPeaks[j+1]);
			j+=2;
		}
	}

	free(pPeaks);
	return true;

}

void MSReader::setFilter(MSSpectrumType m){
  filter.clear();
  filter.push_back(m);
}

void MSReader::setFilter(vector<MSSpectrumType>& m){
  for(unsigned int i=0; i<m.size();i++)
    filter.push_back(m.at(i));
}

void MSReader::setCompression(bool b){
	compressMe=b;
}

void MSReader::setRawFilter(char *c){
	#ifdef _MSC_VER
	cRAW.setRawFilter(c);
	#endif
}

void MSReader::setHighResMGF(bool b){
  highResMGF=b;
}

void MSReader::writeCompressSpec(FILE* fileOut, Spectrum& s){

	int j;

	//file compression
	int err;
	uLong len;
	unsigned char *comprM, *comprI;
  uLong comprLenM, comprLenI;
	double *pD;
	float *pF;
	uLong sizeM;
	uLong sizeI;

	//Build arrays to hold scan prior to compression
	// Ideally, we would just use the scan vectors, but I don't know how yet.
	pD = new double[s.size()];
	pF = new float[s.size()];
	for(j=0;j<s.size();j++){
		pD[j]=s.at(j).mz;
		pF[j]=s.at(j).intensity;
	}

	//compress mz
	len = (uLong)s.size()*sizeof(double);
	sizeM = len;
	comprLenM = compressBound(len);
	comprM = (unsigned char*)calloc((uInt)comprLenM, 1);
	err = compress(comprM, &comprLenM, (const Bytef*)pD, len);

	//compress intensity
	len = (uLong)s.size()*sizeof(float);
	sizeI = len;
	comprLenI = compressBound(len);
	comprI = (unsigned char*)calloc((uInt)comprLenI, 1);
	err = compress(comprI, &comprLenI, (const Bytef*)pF, len);

	j=(int)comprLenM;
	fwrite(&j,4,1,fileOut);
	j=(int)comprLenI;
	fwrite(&j,4,1,fileOut);
	fwrite(comprM,comprLenM,1,fileOut);
	fwrite(comprI,comprLenI,1,fileOut);

	//clean up memory
	free(comprM);
	free(comprI);
	delete [] pD;
	delete [] pF;

}

void MSReader::readCompressSpec(FILE* fileIn, MSScanInfo& ms, Spectrum& s){

	int i;
	Peak_T p;

	//variables for compressed files
	uLong uncomprLen;
	uLong mzLen, intensityLen;
	unsigned char *compr;
	double *mz;
	float *intensity;

	fread(&i,4,1,fileIn);
	mzLen = (uLong)i;
	fread(&i,4,1,fileIn);
	intensityLen = (uLong)i;

	compr = new unsigned char[mzLen];
	mz = new double[ms.numDataPoints];
	uncomprLen=ms.numDataPoints*sizeof(double);
	fread(compr,mzLen,1,fileIn);
	uncompress((Bytef*)mz, &uncomprLen, compr, mzLen);
	delete [] compr;

	compr = new unsigned char[intensityLen];
	intensity = new float[ms.numDataPoints];
	uncomprLen=ms.numDataPoints*sizeof(float);
	fread(compr,intensityLen,1,fileIn);
	uncompress((Bytef*)intensity, &uncomprLen, compr, intensityLen);
	delete [] compr;

	for(i=0;i<ms.numDataPoints;i++){
		p.mz = mz[i];
		p.intensity = intensity[i];
		s.add(p);
	}

	delete [] mz;
	delete [] intensity;

}

void MSReader::writeTextSpec(FILE* fileOut, Spectrum& s) {

	int i,j,k;
	char t[64];

  if(exportMGF){
    //MGF spectrum header is here
    if(highResMGF){
      for(i=0;i<s.sizeZ();i++){
        fprintf(fileOut,"BEGIN IONS\n");
        fprintf(fileOut,"PEPMASS=%.*f\n",6,s.atZ(i).mz);
        fprintf(fileOut,"CHARGE=%d+\n",s.atZ(i).z);
        fprintf(fileOut,"TITLE=%s.%d.%d.%d %d %.4f\n","test",s.getScanNumber(),s.getScanNumber(true),s.atZ(i).z,i,s.getRTime());
        for(j=0;j<s.size();j++){
		      sprintf(t,"%.*f",iIntensityPrecision,s.at(j).intensity);
		      k=strlen(t);
		      if(k>2 && iIntensityPrecision>0){
		        if(t[0]=='0'){
		          fprintf(fileOut,"%.*f 0\n",iMZPrecision,s.at(j).mz);
			      } else if(t[k-1]=='0'){
			        fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision-1,s.at(j).intensity);
       			} else {
			        fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision,s.at(j).intensity);
			      }
		      } else {
			      fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision,s.at(j).intensity);
		      }
	      }
        fprintf(fileOut,"END IONS\n");
      }

    } else {
      fprintf(fileOut,"BEGIN IONS\n");
      fprintf(fileOut,"PEPMASS=%.*f\n",6,s.getMZ());
      if(s.sizeZ()==1){
        if(s.atZ(0).z==1) fprintf(fileOut,"CHARGE=1+\n");
      }
      fprintf(fileOut,"TITLE=%s.%d.%d.%d %d %.4f\n","test",s.getScanNumber(),s.getScanNumber(true),s.atZ(0).z,0,s.getRTime());
      for(j=0;j<s.size();j++){
		    sprintf(t,"%.*f",iIntensityPrecision,s.at(j).intensity);
		    k=strlen(t);
		    if(k>2 && iIntensityPrecision>0){
		      if(t[0]=='0'){
		        fprintf(fileOut,"%.*f 0\n",iMZPrecision,s.at(j).mz);
			    } else if(t[k-1]=='0'){
			      fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision-1,s.at(j).intensity);
      		} else {
			      fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision,s.at(j).intensity);
			    }
		    } else {
			    fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision,s.at(j).intensity);
		    }
	    }
      fprintf(fileOut,"END IONS\n");
    }
    return;
  }

  //Only use this code if not writing MGF file
	for(j=0;j<s.size();j++){
		sprintf(t,"%.*f",iIntensityPrecision,s.at(j).intensity);
		k=strlen(t);
		if(k>2 && iIntensityPrecision>0){
			if(t[0]=='0'){
				fprintf(fileOut,"%.*f 0\n",iMZPrecision,s.at(j).mz);
			} else if(t[k-1]=='0'){
				fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision-1,s.at(j).intensity);
			} else {
				fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision,s.at(j).intensity);
			}
		} else {
			fprintf(fileOut,"%.*f %.*f\n",iMZPrecision,s.at(j).mz,iIntensityPrecision,s.at(j).intensity);
		}
	}

}

void MSReader::writeBinarySpec(FILE* fileOut, Spectrum& s) {
	int j;

	for(j=0;j<s.size();j++){
		fwrite(&s.at(j).mz,8,1,fileOut);
		fwrite(&s.at(j).intensity,4,1,fileOut);
	}

}

void MSReader::writeSpecHeader(FILE* fileOut, bool text, Spectrum& s) {

	//MSScanInfo ms;
  double d;
  float f;
  int i;
	MSSpectrumType mft;
	int j;

	//output scan info
	if(text){

    //MSx spectrum header is here
		mft=s.getFileType();
   	if(mft==MS2 || mft==MS3 || mft==SRM){
    	fprintf(fileOut,"S\t%d\t%d",s.getScanNumber(),s.getScanNumber(true));
			for(i=0;i<s.sizeMZ();i++){
				fprintf(fileOut,"\t%.*lf",4,s.getMZ());
			}
			fprintf(fileOut,"\n");
		} else {
	  	fprintf(fileOut,"S\t%d\t%d\n",s.getScanNumber(),s.getScanNumber(true));
		}
  	if(s.getRTime()>0) fprintf(fileOut,"I\tRTime\t%.*f\n",4,s.getRTime());
    if(s.getBPI()>0) fprintf(fileOut,"I\tBPI\t%.*f\n",2,s.getBPI());
    if(s.getBPM()>0) fprintf(fileOut,"I\tBPM\t%.*f\n",4,s.getBPM());
    if(s.getConversionA()!=0) fprintf(fileOut,"I\tConvA\t%.*f\n",6,s.getConversionA());
    if(s.getConversionB()!=0) fprintf(fileOut,"I\tConvB\t%.*f\n",6,s.getConversionB());
		if(s.getConversionC()!=0) fprintf(fileOut,"I\tConvC\t%.*f\n",6,s.getConversionC());
    if(s.getConversionD()!=0) fprintf(fileOut,"I\tConvD\t%.*f\n",6,s.getConversionD());
		if(s.getConversionE()!=0) fprintf(fileOut,"I\tConvE\t%.*f\n",6,s.getConversionE());
    if(s.getConversionI()!=0) fprintf(fileOut,"I\tConvI\t%.*f\n",6,s.getConversionI());
    if(s.getTIC()>0) fprintf(fileOut,"I\tTIC\t%.*f\n",2,s.getTIC());
    if(s.getIonInjectionTime()>0) fprintf(fileOut,"I\tIIT\t%.*f\n",4,s.getIonInjectionTime());
    for(j=0;j<s.sizeEZ();j++){
      fprintf(fileOut,"I\tEZ\t%d\t%.*f\t%.*f\t%.*f\n",s.atEZ(j).z,4,s.atEZ(j).mh,4,s.atEZ(j).pRTime,1,s.atEZ(j).pArea);
  	}
	  for(j=0;j<s.sizeZ();j++){
		 	fprintf(fileOut,"Z\t%d\t%.*f\n",s.atZ(j).z,4,s.atZ(j).mz);
		}

	} else {
    i=s.getScanNumber();
    fwrite(&i,4,1,fileOut);

    i=s.getScanNumber(true);
    fwrite(&i,4,1,fileOut);

		i=s.sizeMZ();
		fwrite(&i,4,1,fileOut);
		for(i=0;i<s.sizeMZ();i++){
			d=s.getMZ(i);
			fwrite(&d,8,1,fileOut);
		}

    f=s.getRTime();
    fwrite(&f,4,1,fileOut);

    f=s.getBPI();
    fwrite(&f,4,1,fileOut);

    d=s.getBPM();
    fwrite(&d,8,1,fileOut);

    d=s.getConversionA();
    fwrite(&d,8,1,fileOut);

    d=s.getConversionB();
    fwrite(&d,8,1,fileOut);

		d=s.getConversionC();
    fwrite(&d,8,1,fileOut);

    d=s.getConversionD();
    fwrite(&d,8,1,fileOut);

		d=s.getConversionE();
    fwrite(&d,8,1,fileOut);

    d=s.getConversionI();
    fwrite(&d,8,1,fileOut);

    d=s.getTIC();
    fwrite(&d,8,1,fileOut);

    f=s.getIonInjectionTime();
    fwrite(&f,4,1,fileOut);

    i=s.sizeZ();
    fwrite(&i,4,1,fileOut);

    i=s.sizeEZ();
    fwrite(&i,4,1,fileOut);

    i=s.size();
    fwrite(&i,4,1,fileOut);
    /*
		ms.scanNumber[0]=ms.scanNumber[1]=s.getScanNumber();
		ms.rTime=s.getRTime();
		ms.numDataPoints=s.size();
		ms.numZStates=s.sizeZ();
		fwrite(&ms,sizeof(MSScanInfo),1,fileOut);
    */
    for(j=0;j<s.sizeZ();j++){
			fwrite(&s.atZ(j).z,4,1,fileOut);
			fwrite(&s.atZ(j).mz,8,1,fileOut);
		}

    for(j=0;j<s.sizeEZ();j++){
			fwrite(&s.atEZ(j).z,4,1,fileOut);
			fwrite(&s.atEZ(j).mh,8,1,fileOut);
      fwrite(&s.atEZ(j).pRTime,4,1,fileOut);
      fwrite(&s.atEZ(j).pArea,4,1,fileOut);
		}
	}

}

void MSReader::readSpecHeader(FILE *fileIn, MSScanInfo &ms){
	double d;

  fread(&ms.scanNumber[0],4,1,fileIn);
  if(feof(fileIn)) return;
  fread(&ms.scanNumber[1],4,1,fileIn);
	if(iVersion>=5){
		fread(&ms.mzCount,4,1,fileIn);
		if(ms.mz!=NULL) delete [] ms.mz;
		ms.mz = new double[ms.mzCount];
		for(int i=0;i<ms.mzCount;i++){
			fread(&d,8,1,fileIn);
			ms.mz[i]=d;
		}
	} else {
		fread(&ms.mz,8,1,fileIn);
	}
  fread(&ms.rTime,4,1,fileIn);

  if(iVersion>=2){
    fread(&ms.BPI,4,1,fileIn);
    fread(&ms.BPM,8,1,fileIn);
    fread(&ms.convA,8,1,fileIn);
    fread(&ms.convB,8,1,fileIn);
		if(iVersion>=4){
			fread(&ms.convC,8,1,fileIn);
			fread(&ms.convD,8,1,fileIn);
			fread(&ms.convE,8,1,fileIn);
			fread(&ms.convI,8,1,fileIn);
		}
    fread(&ms.TIC,8,1,fileIn);
    fread(&ms.IIT,4,1,fileIn);
  }

  fread(&ms.numZStates,4,1,fileIn);

  if(iVersion>=3) fread(&ms.numEZStates,4,1,fileIn);
  else ms.numEZStates=0;

  fread(&ms.numDataPoints,4,1,fileIn);

}

MSFileFormat MSReader::checkFileFormat(const char *fn){

  unsigned int i;
	char ext[32];
	char tmp[1024];
	char* c;

	//extract extension & capitalize
	c=(char*)strrchr(fn,'.');
	if(c==NULL) return dunno;
	strcpy(ext,c);
	for(i=0;i<strlen(ext);i++) ext[i]=toupper(ext[i]);

  //check extension first - we must trust MS1 & MS2 & ZS & UZS
  if(strcmp(ext,".MS1")==0 ) return ms1;
  if(strcmp(ext,".MS2")==0 ) return ms2;
	if(strcmp(ext,".BMS1")==0 ) return bms1;
  if(strcmp(ext,".BMS2")==0 ) return bms2;
	if(strcmp(ext,".CMS1")==0 ) return cms1;
  if(strcmp(ext,".CMS2")==0 ) return cms2;
  if(strcmp(ext,".ZS")==0 ) return zs;
  if(strcmp(ext,".UZS")==0 ) return uzs;
  if(strcmp(ext,".MSMAT")==0 ) return msmat_ff;
  if(strcmp(ext,".RAW")==0 ) return raw;
  if(strcmp(ext,".MZXML")==0 ) return mzXML;
  if(strcmp(ext,".MZ5")==0 ) return mz5;
	if(strcmp(ext,".MZML")==0 ) return mzML;
  if(strcmp(ext,".MGF")==0 ) return mgf;
	//add the sqlite3 format
  if(strcmp(ext,".SQLITE3")==0 ) return sqlite;
  if(strcmp(ext,".PSM") == 0) return psm;
	
	if(strcmp(ext,".GZ")==0 ) {
		i=c-fn;
		strncpy(tmp,fn,i);
		tmp[i]='\0';
		c=strrchr(tmp,'.');
		if(c==NULL) return dunno;
		strcpy(ext,c);
		for(i=0;i<strlen(ext);i++) ext[i]=toupper(ext[i]);
		if(strcmp(ext,".MZXML")==0 ) return mzXMLgz;
		if(strcmp(ext,".MZML")==0 ) return mzMLgz;
	}

  return dunno;

}

