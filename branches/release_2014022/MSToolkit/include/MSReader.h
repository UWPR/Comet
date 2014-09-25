#ifndef _MSREADER_H
#define _MSREADER_H

#include "Spectrum.h"
#include "MSObject.h"
#include "mzParser.h"
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#ifdef MST_MZ5
#define MZP_MZ5 1
#endif

//For mzXML Writing
//#include "mzXMLWriter.h"
//#include "MSToolkitInterface.h"

#ifdef _MSC_VER
//#include <atlstr.h>
//#import "XRawfile2.dll" rename_namespace("XRawfile")
//using namespace XRAWFILE2Lib;

//Explicit path is needed on systems where Xcalibur installs with errors
//For example, Vista-64bit
//#import "C:\Xcalibur\system\programs\XRawfile2.dll" 
#import "MSFileReader.XRawfile2.dll" rename_namespace("XRawfile")
using namespace XRawfile;
#endif

#ifndef _NOSQLITE
#include <sqlite3.h>
#endif


//Macros for 64-bit file support
#ifdef _MSC_VER
#include "RAWReader.h"
//extern "C" int __cdecl _fseeki64(FILE *, __int64, int);
//extern "C" __int64 __cdecl _ftelli64(FILE *);
typedef __int64 f_off;
#define fseek(h,p,o) _fseeki64(h,p,o)
#define ftell(h) _ftelli64(h)
#else

#ifndef _LARGEFILE_SOURCE
#error "need to define _LARGEFILE_SOURCE!!"
#endif    /* end _LARGEFILE_SOURCE */

typedef off_t f_off;
#define fseek(h,p,o) fseeko(h,p,o)
#define ftell(h) ftello(h)

#endif /* end _MSC_VER */ 


using namespace std;

namespace MSToolkit {
class MSReader {
 public:
  //Constructors & Destructors
  MSReader();
  ~MSReader();

  //Functions
	void addFilter(MSSpectrumType m);
  void appendFile(char* c, bool text, Spectrum& s);
  void appendFile(char* c, bool text, MSObject& m);
  void appendFile(char* c, Spectrum& s);
  void appendFile(char* c, MSObject& m);
  
  MSFileFormat checkFileFormat(const char *fn);

  MSHeader& getHeader();
  MSSpectrumType getFileType();
  int getPercent();
  int getLastScan();

  //get specific header informations
  void setPrecision(int i, int j);
  void setPrecisionInt(int i);
  void setPrecisionMZ(int i);
  void writeFile(const char* c, bool text, MSObject& m);
  void writeFile(const char* c, MSFileFormat ff, MSObject& m, char* sha1Report='\0');

  bool readMSTFile(const char* c, bool text, Spectrum& s, int scNum=0);
  bool readMZPFile(const char* c, Spectrum& s, int scNum=0);
  bool readFile(const char* c, Spectrum& s, int scNum=0);
  
  void setFilter(vector<MSSpectrumType>& m);
  void setFilter(MSSpectrumType m);

  //For RAW files
  bool lookupRT(char* c, int scanNum, float& rt);
  void setAverageRaw(bool b, int width=1, long cutoff=1000);
  void setLabel(bool b);  //label data contains all centroids (including noise and excluded peaks)
  void setRawFilter(char* c);
  void setRawFilterExact(bool b);

  //For MGF files
  void setHighResMGF(bool b);

  //File compression
  void setCompression(bool b);

  //for Sqlite
  void createIndex(); 

 protected:

 private:
  //Data Members
  FILE *fileIn;
  MSHeader header;
  int headerIndex;
  MSSpectrumType fileType;
  f_off lEnd;
  f_off lPivot;
  f_off lFWidth;
  int iIntensityPrecision;
  int iMZPrecision;
  int iVersion;
  int iFType;
  MSFileFormat lastFileFormat;

  //File compression
  bool compressMe;

  //mzXML support variables;
  ramp_fileoffset_t  *pScanIndex;
  RAMPFILE  *rampFileIn;
  bool rampFileOpen;
  int rampLastScan;
  int rampIndex;
  vector<MSSpectrumType> filter;

  //for RAW file support (even if not on windows)
  bool rawFileOpen;

  //for mgf output
  bool exportMGF;
  bool highResMGF;

  //Functions
  void closeFile();
  int openFile(const char* c, bool text=false);
  bool findSpectrum(int i);
  void readCompressSpec(FILE* fileIn, MSScanInfo& ms, Spectrum& s);
  void readSpecHeader(FILE* fileIn, MSScanInfo& ms);

  void writeBinarySpec(FILE* fileOut, Spectrum& s);
  void writeCompressSpec(FILE* fileOut, Spectrum& s);
  void writeTextSpec(FILE* fileOut, Spectrum& s);
  void writeSpecHeader(FILE* fileOut, bool text, Spectrum& s);
  
  //support for rawfiles
  #ifdef _MSC_VER
	RAWReader cRAW;
  #endif

  //support for sqlite
  #ifndef _NOSQLITE
  bool readSqlite(const char* c, Spectrum& s, int scNum);
  void getUncompressedPeaks(Spectrum& s, int& numPeaks, int& mzLen, unsigned char* comprM, int& intensityLen, unsigned char* comprI);
  int curIndex;  //remember where we are
  int lastScanNumber;
  int lastIndex;
  sqlite3* db;
  void sql_stmt(const char* stmt);
  bool executeSqlStmt(Spectrum& s, char* zSql);
  void appendFile(Spectrum& s);
  void writeSqlite(const char* c, MSObject& m, char* sha1Report);
  void readChargeTable(int scanID, Spectrum& s);
  vector<int> estimateCharge(Spectrum& s);
  #endif 

};

}

#endif

