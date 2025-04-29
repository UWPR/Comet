/*
  mzParser - This distribution contains novel code
  and publicly available open source utilities. The novel code is
  open source under the FreeBSD License, please see LICENSE file
  for detailed information.

  Copyright (C) 2011, Mike Hoopmann, Institute for Systems Biology
  Version 1.0, January 4, 2011.
  Version 1.1, March 14, 2012.

  Additional code/libraries obtained from:

  X!Tandem 2010.12.01: http://thegpm.org/

  Copyright and license information for these free utilities are
  provided in the LICENSE file. Note that many changes were made
  to the code adapted from X!Tandem.
*/

#ifndef _MZPARSER_H
#define _MZPARSER_H

//------------------------------------------------
// Standard libraries
//------------------------------------------------
#include <algorithm>
#include <vector>
#include <map>
#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include "expat.h"
#include "zlib.h"
#include "MSNumpress.hpp"

#ifdef MZP_HDF
#include "hdf5.h"
#include "H5Cpp.h"
#endif

//------------------------------------------------
// MACROS
//------------------------------------------------

//#define OSX    // to compile with cc on Macintosh OSX
//#define OSX_TIGER // use with OSX if OSX Tiger is being used
//#define OSX_INTEL // compile with cc on Macintosh OSX on Intel-style processors
//#define MINGW    // compiling with MinGW gcc toolset
// The following should be defined by the compiler already
//#define __GNUC__  // compiling with gcc (or g++)
//#define _MSC_VER  // compiling with Microsoft Studio

#define XMLCLASS    
#ifndef XML_STATIC
#define XML_STATIC  // to statically link the expat libraries
#endif

//For Windows
#ifdef _MSC_VER
#define __inline__ __inline
typedef _int64  __int64_t;
typedef unsigned _int32 uint32_t;
typedef unsigned _int64 uint64_t;
typedef __int64 f_off;
#define mzpfseek(h,p,o) _fseeki64(h,p,o)
#define mzpftell(h) _ftelli64(h)
#define mzpatoi64(h) _atoi64(h)
#define strtok_r strtok_s
#endif

//For MinGW toolset, which lacks the ftello, fseeko, etc functions
#ifdef MINGW
//typedef __int64 f_off;
//#define __int64_t int64_t
#define mzpfseek(h,p,o) fseeko64(h,p,o)
#define mzpftell(h) ftello64(h)
#define mzpatoi64(h) _atoi64(h)
//#include <stdexcept>
#endif

#if defined(GCC) || defined(__LINUX__) || defined(GNUC) || defined(__MINGW32__)
#include <stdint.h>
#include <stdexcept>
#ifndef _LARGEFILE_SOURCE
#error "need to define _LARGEFILE_SOURCE!!"
#endif    /* end _LARGEFILE_SOURCE */
#if _FILE_OFFSET_BITS<64
#error "need to define _FILE_OFFSET_BITS=64"
#endif

typedef off_t f_off;
#define mzpfseek(h,p,o) fseeko(h,p,o)
#define mzpftell(h) ftello(h)
#define mzpatoi64(h) atoll(h)
#endif

#ifdef OSX
#define __inline__ inline
#ifndef OSX_TIGER
#define __int64_t int64_t
#endif
#endif

// this define for the INTEL-based OSX platform is untested and may not work
#ifdef OSX_INTEL
#define __inline__ inline
#endif

// this define should work for most LINUX and UNIX platforms

namespace mzParser {

//------------------------------------------------
// mzMLParser structures, definitions, and enums
//------------------------------------------------
//specDP (Spectrum Data Point) is the basic content element of a mass spectrum.
typedef struct specDP{
  double mz;
  double intensity;
} specDP;

typedef struct specIonMobDP {
  double mz;
  double intensity;
  double ionMobility;
} specIonMobDP;

typedef struct MZIntensityPair {
  double mz;
  double intensity;
  MZIntensityPair() : mz(0), intensity(0) {}
  MZIntensityPair(double mz, double intensity) : mz(mz), intensity(intensity) {}
  bool operator==(const MZIntensityPair& that) const;
} MZIntensityPair;

typedef struct TimeIntensityPair{
  double time;
  double intensity;
} TimeIntensityPair;

typedef struct TimeIntensityIonMob {
  double time;
  double intensity;
  double ionMobility;
} TimeIntensityIonMob;

enum enumActivation {
  none=0,
  CID=1,
  HCD=2,
  ETD=3,
  ETDSA=4,
  ECD=5,
};



//------------------------------------------------
// mzMLParser classes
//------------------------------------------------
//For holding mzML and mzXML indexes
class cindex  { 
public:

  ////MH: Fix this!!!
  //static int compare (const void* a, const void* b) {
  //  if (*(size_t*)a < *(size_t*)b) return -1;
  //  if (*(size_t*)a > *(size_t*)b) return 1;
  //  return 0;
  //}
  static bool compare(cindex& a, cindex& b) {
    if (a.scanNum > b.scanNum) return false;
    if (a.scanNum < b.scanNum) return true;
    return true;
  }

  size_t scanNum;
  std::string idRef;
  f_off offset;
  uint64_t size;  //for mzMLB support
};



//For instrument information
class instrumentInfo {
public:
  std::string analyzer;
  std::string detector;
  std::string id;
  std::string ionization;
  std::string manufacturer;
  std::string model;
  std::string serial;
  instrumentInfo(){
    analyzer="";
    detector="";
    id="";
    ionization="";
    manufacturer="";
    model="";
    serial="";
  }
  void clear(){
    analyzer="";
    detector="";
    id="";
    ionization="";
    manufacturer="";
    model="";
    serial="";
  }
};

typedef struct sPrecursorIon{
  enumActivation activation=none;
  double intensity=0;
  double isoLowerMZ=0;      //lower offset of the isolation window
  double isoLowerOffset=0;
  double isoMZ=0;           //the mz of the isolation window. Often this is the mz of the ion to be isolated
  double isoUpperMZ=0;      //upper offset of the isolation window
  double isoUpperOffset=0;
  double mz=0;              //selected ion mz; is this always redundant with isoMZ? probably on thermo instruments
  double monoMZ=0;          //the true monoisotopic mz as determined by Thermo.
  std::vector<int> possibleCharges;
  int charge=0;
  int msLevel=0;
  int scanNumber=-1;

  void clear(){
    activation=none;
    intensity=0;
    isoLowerMZ = 0;
    isoLowerOffset=0;
    isoMZ = 0;
    isoUpperMZ = 0;
    isoUpperOffset=0;
    mz=0;
    monoMZ=0;
    possibleCharges.clear();
    charge=0;
    msLevel=0;
    scanNumber=0;
  }
} sPrecursorIon;

typedef struct sUParam{
  std::string name;
  std::string value;
  std::string type;
} sUParam;

class BasicSpectrum  {
public:

  //Constructors & Destructors
  BasicSpectrum();
  BasicSpectrum(const BasicSpectrum& s);
  ~BasicSpectrum();

  //Operator overloads
  BasicSpectrum& operator=(const BasicSpectrum& s);
  specDP& operator[ ](const size_t& index);

  //Modifiers
  void addDP(specDP dp);
  void addDP(specIonMobDP dp);
  void clear();
  void clearPrecursor();
  void setActivation(int a);
  void setBasePeakIntensity(double d);
  void setBasePeakMZ(double d);
  void setCentroid(bool b);
  void setCollisionEnergy(double d);
  void setCompensationVoltage(double d);
  void setFilterLine(char* str);
  void setHighMZ(double d);
  void setIDString(const char* str);
  void setInverseReducedIonMobility(double d);
  void setIonInjectionTime(double d);
  void setIonMobilityDriftTime(double d);
  void setIonMobilityScan(bool b);
  void setLowMZ(double d);
  void setMSLevel(int level);
  void setPeaksCount(int i);
  void setPositiveScan(bool b);
  void setPrecursorCharge(int charge);
  void setPrecursorIntensity(double intensity);
  void setPrecursorIon(sPrecursorIon& pi);
  void setPrecursorIon(double mz, double monoMZ, double intensity, int charge, int possibleChargeCount, int* possibleCharges);
  void setPrecursorMZ(double mz);
  void setPrecursorScanNum(int i);
  void setRTime(float t);
  void setScanIndex(int num);
  void setScanNum(int num);
  void setTotalIonCurrent(double d);
  void setUserParam(std::string name, std::string value, std::string type="");

  //Accessors
  int           getActivation();
  double        getBasePeakIntensity();
  double        getBasePeakMZ();
  bool          getCentroid();
  double        getCollisionEnergy();
  double        getCompensationVoltage();
  double        getInverseReducedIonMobility();
  void          getFilterLine(std::string& str);
  double        getHighMZ();
  void          getIDString(std::string& str);
  double        getIonInjectionTime();
  double        getIonMobilityDriftTime();
  specIonMobDP& getIonMobDP(const size_t& index);
  bool          getIonMobilityScan();
  double        getLowMZ();
  int           getMSLevel();
  int           getPeaksCount();
  bool          getPositiveScan();
  sPrecursorIon getPrecursorIon(int i=0);
  int           getPrecursorIonCount();
  int           getPrecursorCharge(int i=0);
  int           getPrecursorChargeCount(int i=0);
  double        getPrecursorIntensity(int i=0);
  double        getPrecursorMonoMZ(int i=0);
  double        getPrecursorMZ(int i=0);
  int           getPrecursorScanNum();
  float         getRTime(bool min=true);
  int           getScanIndex();
  int           getScanNum();
  double        getTotalIonCurrent();
  sUParam       getUserParam(const size_t& index);
  size_t        size();
  

protected:

  //Data Members (protected)
  int             activation;
  double          basePeakIntensity;
  double          basePeakMZ;
  bool            centroid;
  double          collisionEnergy;
  double          compensationVoltage;  //FAIMS compensation voltage
  double          inverseReducedIonMobility;
  bool            ionMobilityScan;
  char            filterLine[128];
  double          highMZ;
  char            idString[128];
  double          ionInjectionTime;
  double          ionMobilityDriftTime;
  double          lowMZ;
  int             msLevel;
  int             peaksCount;
  bool            positiveScan;
  int             precursorScanNum;      //Precursor scan number; 0 if no precursor or unknown
  float           rTime;                //always stored in minutes
  int             scanIndex;            //when scan numbers aren't enough, there are indexes (start at 1)
  int             scanNum;              //identifying scan number
  double          totalIonCurrent;
  std::vector<specDP>* vData;                //Spectrum data points
  std::vector<specIonMobDP>* vDataIonMob;    //Ion Mobility Spectrum data points
  std::vector<sPrecursorIon>* vPrecursor;
  std::vector<sUParam>* vUserParam;
     
};

class BasicChromatogram  {
public:

  //Constructors & Destructors
  BasicChromatogram();
  BasicChromatogram(const BasicChromatogram& c);
  ~BasicChromatogram();

  //Operator overloads
  BasicChromatogram& operator=(const BasicChromatogram& c);
  TimeIntensityPair& operator[ ](const unsigned int index);

  //Modifiers
  void addTIP(TimeIntensityPair tip);
  void clear();
  void setIDString(char* str);
  void setPrecursor(double mz, int z, double offLow, double offHigh);
  void setProduct(double mz, double offLow, double offHigh);

  //Accessors
  int                         getCharge();
  std::vector<TimeIntensityPair>&  getData();
  void                        getIDString(std::string& str);
  double                      getPreMZ();
  double                      getPreOffsetLower();
  double                      getPreOffsetUpper();
  double                      getProdMZ();
  double                      getProdOffsetLower();
  double                      getProdOffsetUpper();
  size_t                      size();

protected:

  //Data Members (protected)
  int                         charge;
  char                        idString[128];
  double                      precursorMZ;
  double                      precursorOffsetLower;
  double                      precursorOffsetUpper;
  double                      productMZ;
  double                      productOffsetLower;
  double                      productOffsetUpper;
  std::vector<TimeIntensityPair>   vData;          //Chromatogram data points
     
};

//------------------------------------------------
// Random access gz (zran from zlib source code)
//------------------------------------------------
#define SPAN 1048576L       // desired distance between access points
#define WINSIZE 32768U      // sliding window size
#define CHUNK 32768         // file input buffer size
#define READCHUNK 16384

// access point entry 
typedef struct point {
  f_off out;          // corresponding offset in uncompressed data 
  f_off in;           // offset in input file of first full byte
  int bits;           // number of bits (1-7) from byte at in - 1, or 0
  unsigned char window[WINSIZE];  // preceding 32K of uncompressed data
} point;

// access point list 
typedef struct gz_access {
  int have;           // number of list entries filled in 
  int size;           // number of list entries allocated 
  point *list; // allocated list
} gz_access;

class Czran{
public:

  Czran();
  ~Czran();

  void free_index();
  gz_access *addpoint(int bits, f_off in, f_off out, unsigned left, unsigned char *window);
  int build_index(FILE *in, f_off span);
  int build_index(FILE *in, f_off span, gz_access **built);
  int extract(FILE *in, f_off offset, unsigned char *buf, int len);
  int extract(FILE *in, f_off offset);
  f_off getfilesize();

protected:
private:
  gz_access* index;
  
  unsigned char* buffer;
  f_off bufferOffset;
  int bufferLen;

  unsigned char* lastBuffer;
  f_off lastBufferOffset;
  int lastBufferLen;

  f_off fileSize;

};



//------------------------------------------------
// X!Tandem borrowed headers
//------------------------------------------------

enum eElementState {
  esChromatogram,
  esIndex,
  esPrecursor,
  esSpectrum
};

int b64_decode_mio (char *dest, const char *src, size_t size);
int b64_encode (char *dest, const char *src, int len);

class mzpSAXHandler{
public:

  //  SAXHandler constructors and destructors
  mzpSAXHandler();
  virtual ~mzpSAXHandler();

  //  SAXHandler eXpat event handlers
  virtual void startElement(const XML_Char *el, const XML_Char **attr);
  virtual void endElement(const XML_Char *el);
  virtual void characters(const XML_Char *s, int len);

  //  SAXHandler Parsing functions.
  bool open(const char* fileName);
  bool parse();
  bool parseOffset(f_off offset);
  void parserReset();
  void setGZCompression(bool b);

  inline void setFileName(const char* fileName) {
    m_strFileName = fileName;
  }

  //  SAXHandler helper functions
  inline bool isElement(const char *n1, const XML_Char *n2)
  {  return (strcmp(n1, n2) == 0); }

  inline bool isAttr(const char *n1, const XML_Char *n2)
  {  return (strcmp(n1, n2) == 0); }

  inline const char* getAttrValue(const char* name, const XML_Char **attr) {
    for (int i = 0; attr[i]; i += 2) {
      if (isAttr(name, attr[i])) return attr[i + 1];
    }
    return "";
  }

  bool getIonMobility() {
    return m_bionMobility;
  }

protected:

  XML_Parser m_parser;
  std::string  m_strFileName;
  bool m_bStopParse;
  bool m_bGZCompression;
  bool m_bionMobility;

  FILE* fptr;
  Czran gzObj;

};

class mzpSAXMzmlHandler : public mzpSAXHandler {
public:
  mzpSAXMzmlHandler(BasicSpectrum* bs);
  mzpSAXMzmlHandler(BasicSpectrum* bs, BasicChromatogram* bc);
  ~mzpSAXMzmlHandler();

  //  Overrides of SAXHandler functions
  void startElement(const XML_Char *el, const XML_Char **attr);
  void endElement(const XML_Char *el);
  void characters(const XML_Char *s, int len);

  //  SAXMzmlHandler public functions
  std::vector<cindex>*         getChromatIndex();
  f_off                   getIndexOffset();
  std::vector<instrumentInfo>* getInstrument();
  int                     getPeaksCount();
  std::vector<cindex>*         getSpecIndex();
  int                     highChromat();
  int                     highScan();
  bool                    load(const char* fileName);
  int                     lowScan();
  bool                    readChromatogram(int num=-1);
  bool                    readHeader(int num=-1);
  bool                    readHeaderFromOffset(f_off offset, int scNm=-1);
  bool                    readSpectrum(int num=-1);
  bool                    readSpectrumFromOffset(f_off offset, int scNm=-1);
  void                    setMZMLB(bool b);
  
protected:

private:

  //  mzpSAXMzmlHandler subclasses
  class cvParam  {
  public:
    std::string refGroupName;
    std::string name;
    std::string accession;
    std::string value;
    std::string unitAccession;
    std::string unitName;
  };

  //  mzpSAXMzmlHandler private functions
  bool  parseHDFOffset(int index);
  void  processData();
  void  processCVParam(const char* name, const char* accession, const char* value, const char* unitName="0", const char* unitAccession="0");
  void  pushChromatogram();
  void  pushSpectrum();  // Load current data into pvSpec, may have to guess charge
  f_off readIndexOffset();
  void  stopParser();
  bool  generateIndexOffset();

  //  mzpSAXMzmlHandler Base64 conversion functions
  void decode(std::vector<double>& d);
  //void decode32(vector<double>& d);
  //void decode64(vector<double>& d);
  //void decompress32(vector<double>& d);
  //void decompress64(vector<double>& d);
  unsigned long dtohl(uint32_t l, bool bNet);
  uint64_t dtohl(uint64_t l, bool bNet);

  //mzMLb support
#ifdef MZP_HDF
  void  closeHDF();
  bool  openHDF(const char* filename);
  void  readHDFHead();
  void  readHDFIndex();
  hid_t m_hdfFile;
  std::string m_strHDFDatasetID;
  hsize_t m_hdfOffset;
  hsize_t m_hdfArraySz;
  hid_t m_hdfmzml;
  hid_t m_hdfmzData;
  hid_t m_hdfintData;
  hid_t m_hdfmzSpace;
  hid_t m_hdfintSpace;
#endif

  //  mzpSAXMzmlHandler Flags indicating parser is inside a particular tag.
  bool m_bInIndexedMzML;
  bool m_bInRefGroup;
  bool m_bInmzArrayBinary;
  bool m_bInintenArrayBinary;
  bool m_bInionMobilityArrayBinary;
  bool m_bInSpectrumList;
  bool m_bInChromatogramList;
  bool m_bInIndexList;
  bool m_bInProduct;

  //  mzpSAXMzmlHandler procedural flags.
  bool m_bChromatogramIndex;
  bool m_bHeaderOnly;
  bool m_bLowPrecision;
  bool m_bMZMLB;
  bool m_bNetworkData;  // i.e. big endian
  bool m_bNumpressLinear;
  bool m_bNumpressPic;
  bool m_bNumpressSlof;
  bool m_bNoIndex;
  bool m_bSpectrumIndex;
  bool m_bZlib;
  int  m_iDataType;   //0=unspecified, 1=32-bit float, 2=64-bit float
  bool m_bIndexSorted;
  //  mzpSAXMzmlHandler index data members.
  std::vector<cindex>    m_vIndex;
  std::map<std::string, size_t> m_mIndex; //Map of index for quick-lookup of indexed scan numbers
  cindex            curIndex;
  int               posIndex;
  f_off             indexOffset;

  std::vector<cindex>    m_vChromatIndex;
  std::map<std::string, size_t> m_mChromatIndex; //Map of index for quick-lookup of indexed chromatogram numbers
  cindex            curChromatIndex;
  int               posChromatIndex;

  std::vector<eElementState> m_vState;

  //  mzpSAXMzmlHandler data members.
  BasicChromatogram*      chromat;
  std::string             m_ccurrentRefGroupName;
  long                    m_encodedLen;            // For compressed data
  instrumentInfo          m_instrument;
  sPrecursorIon           m_precursorIon;
  int                     m_peaksCount;            // Count of peaks in spectrum
  std::vector<cvParam>    m_refGroupCvParams;
  int                     m_scanIDXCount;
  int                     m_scanNumOverride;
  double                  m_startTime;             //in minutes
  double                  m_stopTime;              //in minutes
  std::string             m_strData;               // For collecting character data.
  std::vector<instrumentInfo>  m_vInstrument;
  BasicSpectrum*          spec;
  std::vector<double>     vdI;
  std::vector<double>     vdM;                     // Peak list std::vectors (masses and charges)
  std::vector<double>     vdIM;                    // Peak list std::vectors Ion Mobility

};

class mzpSAXMzxmlHandler : public mzpSAXHandler {
public:
  mzpSAXMzxmlHandler(BasicSpectrum* bs);
  mzpSAXMzxmlHandler(BasicSpectrum* bs, BasicChromatogram* bc);
  ~mzpSAXMzxmlHandler();

  //  Overrides of SAXHandler functions
  void startElement(const XML_Char *el, const XML_Char **attr);
  void endElement(const XML_Char *el);
  void characters(const XML_Char *s, int len);

  //  mzpSAXMzxmlHandler public functions
  std::vector<cindex>* getIndex();
  f_off           getIndexOffset();
  instrumentInfo  getInstrument();
  int             getPeaksCount();
  int             highScan();
  bool            load(const char* fileName);
  int             lowScan();
  bool            readChromat(int num=-1);
  bool            readHeader(int num=-1);
  bool            readHeaderFromOffset(f_off offset);
  bool            readSpectrum(int num=-1);
  bool            readSpectrumFromOffset(f_off offset);
  
protected:

private:

  //  mzpSAXMzxmlHandler private functions
  void  pushSpectrum();  // Load current data into pvSpec, may have to guess charge
  f_off readIndexOffset();
  void  stopParser();
  bool  generateIndexOffset();

  //  mzpSAXMzxmlHandler Base64 conversion functions
  void decode32();
  void decode64();
  void decompress32();
  void decompress64();
  unsigned long dtohl(uint32_t l, bool bNet);
  uint64_t dtohl(uint64_t l, bool bNet);

  //  mzpSAXMzxmlHandler Flags indicating parser is inside a particular tag.
  bool m_bInDataProcessing;
  bool m_bInIndex;
  bool m_bInMsInstrument;
  bool m_bInMsRun;
  bool m_bInPeaks;
  bool m_bInPrecursorMz;
  bool m_bInScan;

  //  mzpSAXMzxmlHandler procedural flags.
  bool m_bCompressedData;
  bool m_bHeaderOnly;
  bool m_bLowPrecision;
  bool m_bNetworkData;  // i.e. big endian
  bool m_bNoIndex;
  bool m_bScanIndex;
  bool m_bIndexSorted;
  //  mzpSAXMzxmlHandler index data members.
  std::vector<cindex>    m_vIndex;
  cindex            curIndex;
  int               posIndex;
  f_off             indexOffset;

  //  mzpSAXMzxmlHandler data members.
  uLong                   m_compressLen;  // For compressed data
  instrumentInfo          m_instrument;
  int                     m_peaksCount;    // Count of peaks in spectrum
  sPrecursorIon           m_precursorIon;
  std::string                  m_strData;      // For collecting character data.
  std::vector<instrumentInfo>  m_vInstrument;
  BasicSpectrum*          spec;
  std::vector<double>          vdI;
  std::vector<double>          vdM;            // Peak list vectors (masses and charges)

};

//------------------------------------------------
// mz5 Support
//------------------------------------------------

#ifdef MZP_HDF

//mz5 constants
#define CVL 128
#define USRVL 128
#define USRNL 256
#define USRTL 64

static unsigned short MZ5_FILE_MAJOR_VERSION = 0;
static unsigned short MZ5_FILE_MINOR_VERSION = 9;

//forward declarations
class mzpMz5Config;

enum MZ5DataSets {
  ControlledVocabulary,
  FileContent,
  Contact,
  CVReference,
  CVParam,
  UserParam,
  RefParam,
  ParamGroups,
  SourceFiles,
  Samples,
  Software,
  ScanSetting,
  InstrumentConfiguration,
  DataProcessing,
  Run,
  SpectrumMetaData,
  SpectrumBinaryMetaData,
  SpectrumIndex,
  SpectrumMZ,
  SpectrumIntensity,
  ChromatogramMetaData,
  ChromatogramBinaryMetaData,
  ChromatogramIndex,
  ChromatogramTime,
  ChromatogramIntensity,
  FileInformation
};

enum SpectrumLoadPolicy {
  SLP_InitializeAllOnCreation,
  SLP_InitializeAllOnFirstCall
};

enum ChromatogramLoadPolicy {
  CLP_InitializeAllOnCreation,
  CLP_InitializeAllOnFirstCall
};

struct FileInformationMZ5Data  {
  unsigned short majorVersion;
  unsigned short minorVersion;
  unsigned short didFiltering;
  unsigned short deltaMZ;
  unsigned short translateInten;
};
  
struct FileInformationMZ5: public FileInformationMZ5Data {
  FileInformationMZ5();
  FileInformationMZ5(const FileInformationMZ5&);
  FileInformationMZ5(const mzpMz5Config&);
  ~FileInformationMZ5();
  FileInformationMZ5& operator=(const FileInformationMZ5&);
  void init(const unsigned short majorVersion, const unsigned short minorVersion, const unsigned didFiltering, const unsigned deltaMZ, const unsigned translateInten);
  static H5::CompType getType();
};

struct ContVocabMZ5Data  {
  char* uri;
  char* fullname;
  char* id;
  char* version;
};

struct ContVocabMZ5: public ContVocabMZ5Data {
  ContVocabMZ5();
  ContVocabMZ5(const std::string& uri, const std::string& fullname, const std::string& id, const std::string& version);
  ContVocabMZ5(const char* uri, const char* fullname, const char* id, const char* version);
  ContVocabMZ5(const ContVocabMZ5&);
  ContVocabMZ5& operator=(const ContVocabMZ5&);
  ~ContVocabMZ5();
  void init(const std::string&, const std::string&, const std::string&, const std::string&);
  static H5::CompType getType();
};

struct CVRefMZ5Data {
  char* name;
  char* prefix;
  unsigned long accession;
};

struct CVRefMZ5: public CVRefMZ5Data {
  CVRefMZ5();
  CVRefMZ5(const CVRefMZ5&);
  CVRefMZ5& operator=(const CVRefMZ5&);
  ~CVRefMZ5();
  void init(const char* name, const char* prefix,  const unsigned long accession);
  static H5::CompType getType();
};

struct UserParamMZ5Data {
  char name[USRNL];
  char value[USRVL];
  char type[USRTL];
  unsigned long unitCVRefID;
};

struct UserParamMZ5: public UserParamMZ5Data {
  UserParamMZ5();
  UserParamMZ5(const UserParamMZ5&);
  UserParamMZ5& operator=(const UserParamMZ5&);
  ~UserParamMZ5();
  void init(const char* name, const char* value, const char* type, const unsigned long urefid);
  static H5::CompType getType();
};

struct CVParamMZ5Data {
  char value[CVL];
  unsigned long typeCVRefID;
  unsigned long unitCVRefID;
};

struct CVParamMZ5: public CVParamMZ5Data {
  CVParamMZ5();
  CVParamMZ5(const CVParamMZ5&);
  CVParamMZ5& operator=(const CVParamMZ5&);
  ~CVParamMZ5();
  void init(const char* value, const unsigned long& cvrefid, const unsigned long& urefid);
  static H5::CompType getType();
};

struct RefMZ5Data {
  unsigned long refID;
};

struct RefMZ5: public RefMZ5Data {
  RefMZ5();
  RefMZ5(const RefMZ5&);
  RefMZ5& operator=(const RefMZ5&);
  ~RefMZ5();
  static H5::CompType getType();
};

struct RefListMZ5Data {
  size_t len;
  RefMZ5* list;
};

struct RefListMZ5: RefListMZ5Data {
  RefListMZ5();
  RefListMZ5(const RefListMZ5&);
  RefListMZ5& operator=(const RefListMZ5&);
  ~RefListMZ5();
  void init(const RefMZ5* list, const size_t len);
  static H5::VarLenType getType();
};

struct ParamListMZ5Data {
  unsigned long cvParamStartID;
  unsigned long cvParamEndID;
  unsigned long userParamStartID;
  unsigned long userParamEndID;
  unsigned long refParamGroupStartID;
  unsigned long refParamGroupEndID;
};

struct ParamListMZ5: ParamListMZ5Data {
  ParamListMZ5();
  ParamListMZ5(const ParamListMZ5&);
  ParamListMZ5& operator=(const ParamListMZ5&);
  ~ParamListMZ5();
  void init(const unsigned long cvstart, const unsigned long cvend, const unsigned long usrstart, const unsigned long usrend, const unsigned long refstart, const unsigned long refend);
  static H5::CompType getType();
};

struct ParamGroupMZ5 {
  char* id;
  ParamListMZ5 paramList;
  ParamGroupMZ5();
  ParamGroupMZ5(const ParamGroupMZ5&);
  ParamGroupMZ5& operator=(const ParamGroupMZ5&);
  ~ParamGroupMZ5();
  void init(const ParamListMZ5& params, const char* id);
  static H5::CompType getType();
};

struct SourceFileMZ5 {
  char* id;
  char* location;
  char* name;
  ParamListMZ5 paramList;
  SourceFileMZ5();
  SourceFileMZ5(const SourceFileMZ5&);
  SourceFileMZ5& operator=(const SourceFileMZ5&);
  ~SourceFileMZ5();
  void init(const ParamListMZ5& params, const char* id, const char* location, const char* name);
  static H5::CompType getType();
};

struct SampleMZ5 {
  char* id;
  char* name;
  ParamListMZ5 paramList;
  SampleMZ5();
  SampleMZ5(const SampleMZ5&);
  SampleMZ5& operator=(const SampleMZ5&);
  ~SampleMZ5();
  void init(const ParamListMZ5& params, const char* id, const char* name);
  static H5::CompType getType();
};

struct SoftwareMZ5 {
  char* id;
  char* version;
  ParamListMZ5 paramList;
  SoftwareMZ5();
  SoftwareMZ5(const SoftwareMZ5&);
  SoftwareMZ5& operator=(const SoftwareMZ5&);
  ~SoftwareMZ5();
  void init(const ParamListMZ5& params, const char* id, const char* version);
  static H5::CompType getType();
};

struct ParamListsMZ5 {
  size_t len;
  ParamListMZ5* lists;
  ParamListsMZ5();
  ParamListsMZ5(const ParamListsMZ5&);
  ParamListsMZ5& operator=(const ParamListsMZ5&);
  ~ParamListsMZ5();
  void init(const ParamListMZ5* list, const size_t len);
  static H5::VarLenType getType();
};

struct ScanSettingMZ5 {
  char* id;
  ParamListMZ5 paramList;
  RefListMZ5 sourceFileIDs;
  ParamListsMZ5 targetList;
  ScanSettingMZ5();
  ScanSettingMZ5(const ScanSettingMZ5&);
  ScanSettingMZ5& operator=(const ScanSettingMZ5&);
  ~ScanSettingMZ5();
  void init(const ParamListMZ5& params, const RefListMZ5& refSourceFiles, const ParamListsMZ5 targets, const char* id);
  static H5::CompType getType();
};

struct ComponentMZ5 {
  ParamListMZ5 paramList;
  unsigned long order;
  ComponentMZ5();
  ComponentMZ5(const ComponentMZ5&);
  ComponentMZ5& operator=(const ComponentMZ5&);
  ~ComponentMZ5();
  void init(const ParamListMZ5&, const unsigned long order);
  static H5::CompType getType();
};

struct ComponentListMZ5 {
  size_t len;
  ComponentMZ5* list;
  ComponentListMZ5();
  ComponentListMZ5(const ComponentListMZ5&);
  ComponentListMZ5(const std::vector<ComponentMZ5>&);
  ComponentListMZ5& operator=(const ComponentListMZ5&);
  ~ComponentListMZ5();
  void init(const ComponentMZ5*, const size_t&);
  static H5::VarLenType getType();
};

struct ComponentsMZ5 {
  ComponentListMZ5 sources;
  ComponentListMZ5 analyzers;
  ComponentListMZ5 detectors;
  ComponentsMZ5();
  ComponentsMZ5(const ComponentsMZ5&);
  ComponentsMZ5& operator=(const ComponentsMZ5&);
  ~ComponentsMZ5();
  void init(const ComponentListMZ5& sources, const ComponentListMZ5& analyzers,  const ComponentListMZ5& detectors);
  static H5::CompType getType();
};

struct InstrumentConfigurationMZ5 {
  char* id;
  ParamListMZ5 paramList;
  ComponentsMZ5 components;
  RefMZ5 scanSettingRefID;
  RefMZ5 softwareRefID;
  InstrumentConfigurationMZ5();
  InstrumentConfigurationMZ5(const InstrumentConfigurationMZ5&);
  InstrumentConfigurationMZ5& operator=(const InstrumentConfigurationMZ5&);
  ~InstrumentConfigurationMZ5();
  void init(const ParamListMZ5& params, const ComponentsMZ5& components, const RefMZ5& refScanSetting, const RefMZ5& refSoftware, const char* id);
  static H5::CompType getType();
};

struct ProcessingMethodMZ5 {
  ParamListMZ5 paramList;
  RefMZ5 softwareRefID;
  unsigned long order;
  ProcessingMethodMZ5();
  ProcessingMethodMZ5(const ProcessingMethodMZ5&);
  ProcessingMethodMZ5& operator=(const ProcessingMethodMZ5&);
  ~ProcessingMethodMZ5();
  void init(const ParamListMZ5& params, const RefMZ5& refSoftware, const unsigned long order);
  static H5::CompType getType();
};

struct ProcessingMethodListMZ5 {
  size_t len;
  ProcessingMethodMZ5* list;
  ProcessingMethodListMZ5();
  ProcessingMethodListMZ5(const ProcessingMethodListMZ5&);
  ProcessingMethodListMZ5& operator=(const ProcessingMethodListMZ5&);
  ~ProcessingMethodListMZ5();
  void init(const ProcessingMethodMZ5* list, const size_t len);
  static H5::VarLenType getType();
};

struct DataProcessingMZ5  {
  char* id;
  ProcessingMethodListMZ5 processingMethodList;
  DataProcessingMZ5();
  DataProcessingMZ5(const DataProcessingMZ5&);
  DataProcessingMZ5& operator=(const DataProcessingMZ5&);
  ~DataProcessingMZ5();
  void init(const ProcessingMethodListMZ5&, const char* id);
  static H5::CompType getType();
};

struct PrecursorMZ5  {
  char* externalSpectrumId;
  ParamListMZ5 activation;
  ParamListMZ5 isolationWindow;
  ParamListsMZ5 selectedIonList;
  RefMZ5 spectrumRefID;
  RefMZ5 sourceFileRefID;
  PrecursorMZ5();
  PrecursorMZ5(const PrecursorMZ5&);
  PrecursorMZ5& operator=(const PrecursorMZ5&);
  ~PrecursorMZ5();
  void init(const ParamListMZ5& activation,  const ParamListMZ5& isolationWindow, const ParamListsMZ5 selectedIonList, const RefMZ5& refSpectrum, const RefMZ5& refSourceFile, const char* externalSpectrumId);
  static H5::CompType getType();
};

struct PrecursorListMZ5 {
  size_t len;
  PrecursorMZ5* list;
  PrecursorListMZ5();
  PrecursorListMZ5(const PrecursorListMZ5&);
  PrecursorListMZ5& operator=(const PrecursorListMZ5&);
  ~PrecursorListMZ5();
  void init(const PrecursorMZ5* list, const size_t len);
  static H5::VarLenType getType();
};

struct ChromatogramMZ5 {
  char* id;
  ParamListMZ5 paramList;
  PrecursorMZ5 precursor;
  ParamListMZ5 productIsolationWindow;
  RefMZ5 dataProcessingRefID;
  unsigned long index;
  ChromatogramMZ5();
  ChromatogramMZ5(const ChromatogramMZ5&);
  ChromatogramMZ5& operator=(const ChromatogramMZ5&);
  ~ChromatogramMZ5();
  void init(const ParamListMZ5& params, const PrecursorMZ5& precursor, const ParamListMZ5& productIsolationWindow, const RefMZ5& refDataProcessing, const unsigned long index, const char* id);
  static H5::CompType getType();
};

struct ScanMZ5 {
  char* externalSpectrumID;
  ParamListMZ5 paramList;
  ParamListsMZ5 scanWindowList;
  RefMZ5 instrumentConfigurationRefID;
  RefMZ5 sourceFileRefID;
  RefMZ5 spectrumRefID;
  ScanMZ5();
  ScanMZ5(const ScanMZ5&);
  ScanMZ5& operator=(const ScanMZ5&);
  ~ScanMZ5();
  void init(const ParamListMZ5& params, const ParamListsMZ5& scanWindowList, const RefMZ5& refInstrument, const RefMZ5& refSourceFile, const RefMZ5& refSpectrum, const char* externalSpectrumID);
  static H5::CompType getType();
};

struct ScanListMZ5 {
  size_t len;
  ScanMZ5* list;
  ScanListMZ5();
  ScanListMZ5(const ScanListMZ5&);
  //ScanListMZ5(const std::vector<ScanMZ5>&);
  ScanListMZ5& operator=(const ScanListMZ5&);
  ~ScanListMZ5();
  void init(const ScanMZ5* list, const size_t len);
  static H5::VarLenType getType();
};

struct ScansMZ5 {
  ParamListMZ5 paramList;
  ScanListMZ5 scanList;
  ScansMZ5();
  ScansMZ5(const ScansMZ5&);
  ScansMZ5& operator=(const ScansMZ5&);
  ~ScansMZ5();
  void init(const ParamListMZ5& params, const ScanListMZ5& scanList);
  static H5::CompType getType();
};

struct SpectrumMZ5 {
  char* id;
  char* spotID;
  ParamListMZ5 paramList;
  ScansMZ5 scanList;
  PrecursorListMZ5 precursorList;
  ParamListsMZ5 productList;
  RefMZ5 dataProcessingRefID;
  RefMZ5 sourceFileRefID;
  unsigned int index;
  SpectrumMZ5();
  SpectrumMZ5(const SpectrumMZ5&);
  SpectrumMZ5& operator=(const SpectrumMZ5&);
  ~SpectrumMZ5();
  void init(const ParamListMZ5& params, const ScansMZ5& scanList, const PrecursorListMZ5& precursors, const ParamListsMZ5& productIonIsolationWindows, const RefMZ5& refDataProcessing, const RefMZ5& refSourceFile, const unsigned long index, const char* id, const char* spotID);
  static H5::CompType getType();
};

struct RunMZ5 {
  char* id;
  char* startTimeStamp;
  char* fid;
  char* facc;
  ParamListMZ5 paramList;
  RefMZ5 defaultSpectrumDataProcessingRefID;
  RefMZ5 defaultChromatogramDataProcessingRefID;
  RefMZ5 defaultInstrumentConfigurationRefID;
  RefMZ5 sourceFileRefID;
  RefMZ5 sampleRefID;
  RunMZ5();
  RunMZ5(const RunMZ5&);
  RunMZ5& operator=(const RunMZ5&);
  ~RunMZ5();
  void init(const ParamListMZ5& params, const RefMZ5& refSpectrumDP, const RefMZ5& refChromatogramDP, const RefMZ5& refDefaultInstrument, const RefMZ5& refSourceFile, const RefMZ5& refSample, const char* id, const char* startTimeStamp, const char* fid, const char* facc);
  static H5::CompType getType();
};

struct BinaryDataMZ5 {
  ParamListMZ5 xParamList;
  ParamListMZ5 yParamList;
  RefMZ5 xDataProcessingRefID;
  RefMZ5 yDataProcessingRefID;
  BinaryDataMZ5();
  BinaryDataMZ5(const BinaryDataMZ5&);
  BinaryDataMZ5& operator=(const BinaryDataMZ5&);
  ~BinaryDataMZ5();
  void init(const ParamListMZ5& xParams, const ParamListMZ5& yParams, const RefMZ5& refDPx, const RefMZ5& refDPy);
  static H5::CompType getType();
};

struct CVRefItem {
  int group; //0=MS, 1=UO, don't know what others there are.
  int ref;
};

class mzpMz5Config{
public:
  mzpMz5Config();
  ~mzpMz5Config();

  static bool PRINT_HDF5_EXCEPTIONS;

  const bool      doFiltering() const;
  const bool      doTranslating() const;
  const size_t    getBufferInB();
  const H5::DataType& getDataTypeFor(const MZ5DataSets v);
  const std::string&    getNameFor(const MZ5DataSets v);
  const size_t&    getRdccSlots();
  MZ5DataSets      getVariableFor(const std::string& name);
  void            setFiltering(const bool flag) const;
  void            setTranslating(const bool flag) const;

protected:

private:

  size_t                      bufferInMB_;
  ChromatogramLoadPolicy      chromatogramLoadPolicy_;
  int                          deflateLvl_;
  mutable bool                doFiltering_;
  mutable bool                doTranslating_;
  size_t                      rdccSolts_;
  SpectrumLoadPolicy          spectrumLoadPolicy_;
  std::map<MZ5DataSets, size_t>        variableBufferSizes_;
  std::map<MZ5DataSets, hsize_t>       variableChunkSizes_;
  std::map<MZ5DataSets, std::string>   variableNames_;
  std::map<MZ5DataSets, H5::DataType>  variableTypes_;
  std::map<std::string, MZ5DataSets>   variableVariables_; //Really? variableVariables? Was this written by Donald Rumsfeld?

  void init(const bool filter, const bool deltamz, const bool translateinten);

};

class cMz5Index : public cindex  {
public:
  unsigned long cvStart;
  unsigned long cvLen;
};

class mzpMz5Handler{
public:
  mzpMz5Handler(mzpMz5Config* c, BasicSpectrum* s);
  mzpMz5Handler(mzpMz5Config* c, BasicSpectrum* s, BasicChromatogram* bc);
  ~mzpMz5Handler();

  void                            clean(const MZ5DataSets v, void* data, const size_t dsend);
  std::vector<cMz5Index>*              getChromatIndex();
  void                            getData(std::vector<double>& data, const MZ5DataSets v, const hsize_t start, const hsize_t end);
  const std::map<MZ5DataSets, size_t>&  getFields();
  std::vector<cMz5Index>*              getSpecIndex();
  int                              highChromat();
  int                              highScan();
  int                              lowScan();
  void                            processCVParams(unsigned long index);
  bool                            readChromatogram(int num=-1);
  void*                            readDataSet(const MZ5DataSets v, size_t& dsend, void* ptr=0);
  bool                            readFile(const std::string filename);
  bool                            readHeader(int num=-1);
  bool                            readSpectrum(int num=-1);

protected:

private:

  //  mzpMz5Handler index data members.
  cMz5Index          curIndex;
  f_off              indexOffset;
  int                m_scanIDXCount;
  std::vector<cMz5Index>  m_vIndex;
  int                posIndex;

  cMz5Index          curChromatIndex;
  std::vector<cMz5Index>  m_vChromatIndex;
  int                posChromatIndex;

  std::map<MZ5DataSets, H5::DataSet> bufferMap_;
  BasicChromatogram*                 chromat;
  bool                               closed_;
  mzpMz5Config*                      config_;
  std::vector<CVRefItem>             cvRef;
  std::vector<CVParamMZ5>            cvParams_;
  std::vector<SpectrumMZ5>           specMetaData_;
  std::map<MZ5DataSets, size_t>      fields_;
  H5::H5File*                        file_;
  BasicSpectrum*                     spec;
};

#endif

//------------------------------------------------
// RAMP API
//------------------------------------------------
#define INSTRUMENT_LENGTH 2000
#define SCANTYPE_LENGTH 32
#define CHARGEARRAY_LENGTH 128
#define PRECURSORARRAY_LENGTH 1024

typedef double RAMPREAL; 
typedef f_off ramp_fileoffset_t;

typedef struct RAMPFILE{
  BasicSpectrum* bs;
  mzpSAXMzmlHandler* mzML;
  mzpSAXMzxmlHandler* mzXML;
  #ifdef MZP_HDF
  mzpMz5Config* mz5Config;
  mzpMz5Handler* mz5;
  #endif
  int fileType;
  int bIsMzData;
  std::string fileName;

  int peakCapacity;
  RAMPREAL* pPeaks;
  ramp_fileoffset_t lLastScanIndex;
  RAMPFILE(){
    bs=NULL;
    mzML=NULL;
    mzXML=NULL;
    pPeaks=NULL;
    peakCapacity=0;
    #ifdef MZP_HDF
    mz5=NULL;
    mz5Config=NULL;
    #endif
    fileType=0;
    bIsMzData=0;
    lLastScanIndex=0;
  }
  ~RAMPFILE(){
    if(bs!=NULL) delete bs;
    if(mzML!=NULL) delete mzML;
    if(mzXML!=NULL) delete mzXML;
    if(pPeaks!=NULL) free(pPeaks);
    bs=NULL;
    mzML=NULL;
    mzXML=NULL;
    #ifdef MZP_HDF
    if(mz5!=NULL) delete mz5;
    if(mz5Config!=NULL) delete mz5Config;
    mz5=NULL;
    mz5Config=NULL;
    #endif
  }
} RAMPFILE;

static std::vector<const char *> data_Ext;

struct ScanHeaderStruct {
   
  int    acquisitionNum;            // scan number as declared in File (may be gaps)
  int    mergedScan;                // only if MS level > 1 
  int    mergedResultScanNum;       // scan number of the resultant merged scan 
  int    mergedResultStartScanNum;  // smallest scan number of the scanOrigin for merged scan 
  int    mergedResultEndScanNum;    // largest scan number of the scanOrigin for merged scan 
  int    msLevel;
  int    numPossibleCharges;
  int    peaksCount;
  int    precursorCharge;           // only if MS level > 1 
  int    precursorCount;   
  int    precursorScanNum;          // only if MS level > 1 
  int    scanIndex;                 //a sequential index for non-sequential scan numbers (1-based)
  int    seqNum;                    //number in sequence observed file (1-based)
   
  double basePeakIntensity;
  double basePeakMZ;
  double collisionEnergy;
  double compensationVoltage;   // only if MS level > 1
  double inverseReducedIonMobility;   // only if MS level > 1
  double highMZ;
  double ionInjectionTime;
  double ionMobilityDriftTime;
  double ionisationEnergy;
  double isolationMZ;           //conforms to MS:1000827 in mzML files
  double isolationWindowLower;  //similar to selection window, but instead just defines the relative width
  double isolationWindowUpper;  //of the isolation window in mz, rather than its absolute width.
  double lowMZ;
  double precursorIntensity;    // only if MS level > 1
  double precursorMonoMZ;
  double precursorMZ;           //only if MS level > 1 
  double retentionTime;         //in seconds
  double selectedIonMZ;         //matches MS:1000744 in mzML files.
  double selectionWindowLower;  //the range of ions acquired
  double selectionWindowUpper;  //in DDA, for example, +/-1 Da around precursor
  double totIonCurrent;
   
  char   activationMethod[SCANTYPE_LENGTH];
  char   additionalPrecursors[PRECURSORARRAY_LENGTH];
  char   filterLine[CHARGEARRAY_LENGTH];
  char   idString[CHARGEARRAY_LENGTH];
  char   possibleCharges[SCANTYPE_LENGTH];
  char   scanType[SCANTYPE_LENGTH];
  char   scanDescription[SCANTYPE_LENGTH]; //specific to Thermo instruments. Currently parsed from userParams in mzML...
        
  bool   centroid; //true if spectrum is centroided
  bool   ionMobility; //true if spectrum contains ion mobility data
  bool   possibleChargesArray[CHARGEARRAY_LENGTH]; /* NOTE: does NOT include "precursorCharge" information; only from "possibleCharges" */
   
  ramp_fileoffset_t    filePosition; /* where in the file is this header? */
};

struct RunHeaderStruct {
  int     scanCount;

  double  dEndTime;
  double  dStartTime;
  double  endMZ;
  double  highMZ;
  double  lowMZ;
  double  startMZ;
};

typedef struct InstrumentStruct {
   char manufacturer[INSTRUMENT_LENGTH];
   char model[INSTRUMENT_LENGTH];
   char ionisation[INSTRUMENT_LENGTH];
   char analyzer[INSTRUMENT_LENGTH];
   char detector[INSTRUMENT_LENGTH];
   char serial[INSTRUMENT_LENGTH];
} InstrumentStruct;

struct ScanCacheStruct {
  int seqNumStart;    // scan at which the cache starts
  int size;           // number of scans in the cache
  struct ScanHeaderStruct *headers;
  RAMPREAL **peaks;
};

int                 checkFileType(const char* fname);
ramp_fileoffset_t   getIndexOffset(RAMPFILE *pFI);
InstrumentStruct*   getInstrumentStruct(RAMPFILE *pFI);
void                getPrecursor(const struct ScanHeaderStruct *scanHeader, int index, double &mz, double &monoMZ, double &intensity, int &charge, int &possibleCharges, int *&possibleChargeArray);
void                getPrecursor(const struct ScanHeaderStruct* scanHeader, int index, sPrecursorIon& pi);
int                 getScanNumberFromOffset(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex);
void                getScanSpanRange(const struct ScanHeaderStruct *scanHeader, int *startScanNum, int *endScanNum);
void                rampCloseFile(RAMPFILE *pFI);
std::string         rampConstructInputFileName(const std::string &basename);
char*               rampConstructInputFileName(char *buf,int buflen,const char *basename);
char*               rampConstructInputPath(char *buf, int inbuflen, const char *dir_in, const char *basename);
const char**        rampListSupportedFileTypes();
RAMPFILE*           rampOpenFile(const char *filename);
char*               rampValidFileType(const char *buf);
void                readHeader(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex, struct ScanHeaderStruct *scanHeader, int iIndex=-1, BasicSpectrum **bs=NULL);
ramp_fileoffset_t*  readIndex(RAMPFILE *pFI, ramp_fileoffset_t indexOffset, int *iLastScan);
int                 readMsLevel(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex);
void                readMSRun(RAMPFILE *pFI, struct RunHeaderStruct *runHeader);
RAMPREAL*           readPeaks(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex, int iIndex=-1, bool ionMobility = false);
int                 readPeaksCount(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex);
void                readRunHeader(RAMPFILE *pFI, ramp_fileoffset_t *pScanIndex, struct RunHeaderStruct *runHeader, int iLastScan);

//MH:Cached RAMP functions
void                            clearScanCache(struct ScanCacheStruct* cache);
void                            freeScanCache(struct ScanCacheStruct* cache);
int                             getCacheIndex(struct ScanCacheStruct* cache, int seqNum);
struct ScanCacheStruct*         getScanCache(int size);
const struct ScanHeaderStruct*  readHeaderCached(struct ScanCacheStruct* cache, int seqNum, RAMPFILE* pFI, ramp_fileoffset_t lScanIndex);
int                             readMsLevelCached(struct ScanCacheStruct* cache, int seqNum, RAMPFILE* pFI, ramp_fileoffset_t lScanIndex);
const RAMPREAL*                 readPeaksCached(struct ScanCacheStruct* cache, int seqNum, RAMPFILE* pFI, ramp_fileoffset_t lScanIndex);
void                            shiftScanCache(struct ScanCacheStruct* cache, int nScans);

//MH:Unimplimented functions. These just bark cerr when used.
int                 isScanAveraged(struct ScanHeaderStruct *scanHeader);
int                 isScanMergedResult(struct ScanHeaderStruct *scanHeader);
int                 rampSelfTest(char *filename);
char*               rampTrimBaseName(char *buf);
int                 rampValidateOrDeriveInputFilename(char *inbuf, int inbuflen, char *spectrumName);
double              readStartMz(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex);
double              readEndMz(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex);
void                setRampOption(long option);


//------------------------------------------------
// PWiz API
//------------------------------------------------
class Chromatogram{
public:
  Chromatogram();
  ~Chromatogram();

  BasicChromatogram*  bc;
  std::string         id;

  void getTimeIntensityPairs(std::vector<TimeIntensityPair>& v);
};
typedef class Chromatogram* ChromatogramPtr;

class ChromatogramList{
public:
  ChromatogramList();
  ChromatogramList(mzpSAXMzmlHandler* ml, void* m5, BasicChromatogram* bc);
  ~ChromatogramList();

  ChromatogramPtr   chromatogram(int index, bool binaryData = false);
  bool              get();
  size_t            size();

  std::vector<cindex>*     vChromatIndex;
  #ifdef MZP_HDF
  std::vector<cMz5Index>*  vMz5Index;
  #endif

private:
  mzpSAXMzmlHandler*  mzML;
  #ifdef MZP_HDF
  mzpMz5Handler*      mz5;
  #endif
  ChromatogramPtr     chromat;
};
typedef class ChromatogramList* ChromatogramListPtr;

class Spectrum {
public:
  Spectrum();
  ~Spectrum();

  BasicSpectrum* bs;
  std::string              id;

  //void getTimeIntensityPairs(std::vector<TimeIntensityPair>& v);
};
typedef class Spectrum* SpectrumPtr;

class SpectrumList {
public:
  SpectrumList();
  SpectrumList(mzpSAXMzmlHandler* ml, void* m5, BasicSpectrum* bs);
  ~SpectrumList();

  SpectrumPtr    spectrum(int index, bool binaryData = false);
  bool           get();
  size_t         size();

  std::vector<cindex>* vSpecIndex;
#ifdef MZP_HDF
  std::vector<cMz5Index>* vMz5Index;
#endif

private:
  mzpSAXMzmlHandler* mzML;
#ifdef MZP_HDF
  mzpMz5Handler* mz5;
#endif
  SpectrumPtr      spec;
};
typedef class SpectrumList* SpectrumListPtr;

class PwizRun{
public:
  PwizRun();
  PwizRun(mzpSAXMzmlHandler* ml, void* m5, BasicChromatogram* b);
  ~PwizRun();

  ChromatogramListPtr chromatogramListPtr;
  SpectrumListPtr     spectrumListPtr;

  bool get();
  void set(mzpSAXMzmlHandler* ml, void* m5, BasicChromatogram* b, BasicSpectrum* s);

private:
  mzpSAXMzmlHandler*  mzML;
  #ifdef MZP_HDF
  mzpMz5Handler*      mz5;
  #endif
  BasicChromatogram*  bc;
  BasicSpectrum*      bs;
};

class MSDataFile{
public:
  MSDataFile(std::string s);
  ~MSDataFile();

  PwizRun run;

private:
  BasicSpectrum*      bs;
  BasicChromatogram*  bc;
  mzpSAXMzmlHandler*  mzML;
  #ifdef MZP_HDF
  mzpMz5Config*       mz5Config;
  mzpMz5Handler*      mz5;
  #endif
};

class SpectrumInfo {
public:
  SpectrumInfo();
  void update(const Spectrum& spectrum, bool getBinaryData = false);

  struct PrecursorInfo {
    size_t index;
    double mz;
    double intensity;
    double charge;
    PrecursorInfo() : index((size_t)-1), mz(0), intensity(0), charge(0) {}
  };

  size_t index;
  std::string id;
  int scanNumber;
  int scanEvent;
  int msLevel;
  bool isZoomScan;
  double retentionTime; // seconds
  std::string filterString;
  double mzLow;
  double mzHigh;
  double basePeakMZ;
  double basePeakIntensity;
  double totalIonCurrent;
  double thermoMonoisotopicMZ;
  double ionInjectionTime;
  std::vector<PrecursorInfo> precursors;
  size_t dataSize;
  std::vector<MZIntensityPair> data;

};


//------------------------------------------------
// MzParser Interface
//------------------------------------------------
class MzParser {
public:
  //Constructors and Destructors
  MzParser(BasicSpectrum* s);
  MzParser(BasicSpectrum* s, BasicChromatogram* c);
  ~MzParser();

  //User functions
  std::vector<cindex>*  getChromatIndex();
  std::vector<cindex>*  getSpectrumIndex();
  int   highChromat();
  int   highScan();
  bool  load(const char* fname);
  int   lowScan();
  bool  readChromatogram(int num=-1);
  bool  readSpectrum(int num=-1);
  bool  readSpectrumHeader(int num=-1);

protected:
  mzpSAXMzmlHandler*    mzML;
  mzpSAXMzxmlHandler*   mzXML;
  #ifdef MZP_HDF
  mzpMz5Handler*        mz5;
  mzpMz5Config*         mz5Config;
  #endif

private:
  //private functions
  int checkFileType(const char* fname);

  //private data members
  BasicChromatogram*  chromat;
  int                 fileType;
  BasicSpectrum*      spec;

};

}

#endif
