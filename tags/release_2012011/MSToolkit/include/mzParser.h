/*
	mzParser - All-in-one header file. No need to link to multiple
	libraries or header files. This distribution contains novel code
	and publicly available open source utilities. The novel code is
	open source under the FreeBSD License, please see LICENSE file
	for detailed information.

	Copyright (C) 2011, Mike Hoopmann, Institute for Systems Biology
	Version 1.0, January 4, 2011.

	Additional code/libraries obtained from:

	X!Tandem 2010.12.01: http://thegpm.org/
	eXpat 2.0.1: http://expat.sourceforge.net/
	zlib 1.2.5: http://www.zlib.net/

	Copyright and license information for these free utilities are
	provided in the LICENSE file. Note that many changes were made
	to the code adapted from X!Tandem.
*/

#ifndef _MZPARSER_H
#define _MZPARSER_H

//------------------------------------------------
// Standard libraries
//------------------------------------------------
#include <vector>
#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include "expat.h"
#include "zlib.h"

using namespace std;

//------------------------------------------------
// MACROS
//------------------------------------------------

//#define OSX				// to compile with cc on Macintosh OSX
//#define OSX_TIGER // use with OSX if OSX Tiger is being used
//#define OSX_INTEL // compile with cc on Macintosh OSX on Intel-style processors
//#define GCC				// to compile with gcc (or g++) on LINUX
#define __LINUX__				// to compile with gcc (or g++) on LINUX
//#define _MSC_VER	// to compile with Microsoft Studio

#define XMLCLASS		
#ifndef XML_STATIC
#define XML_STATIC	// to statically link the expat libraries
#endif

//For Windows
#ifdef _MSC_VER
#undef GCC
#undef __LINUX__
#define __inline__ _inline
typedef _int64  __int64_t;
typedef unsigned _int32 uint32_t;
typedef unsigned _int64 uint64_t;
typedef __int64 f_off;
#define mzpfseek(h,p,o) _fseeki64(h,p,o)
#define mzpftell(h) _ftelli64(h)
#define mzpatoi64(h) _atoi64(h)
#endif

//For Windows
#ifdef __MINGW__
#undef GCC
typedef __int64 f_off;
#define mzpfseek(h,p,o) fseeko64(h,p,o)
#define mzpftell(h) ftello64(h)
#define mzpatoi64(h) _atoi64(h)
#endif

#if defined(GCC) || defined(__LINUX__)
#include <inttypes.h>
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



//------------------------------------------------
// mzMLParser structures, definitions, and enums
//------------------------------------------------
//specDP (Spectrum Data Point) is the basic content element of a mass spectrum.
typedef struct specDP{
	double mz;
	double intensity;
} specDP;


enum enumActivation {
	none=0,
	CID=1,
	HCD=2,
	ETD=3,
	ETDSA=4,
	ECD=5,
	PQD=6,
	IRMPD=7,
};




//------------------------------------------------
// mzMLParser classes
//------------------------------------------------
//For holding mzML and mzXML indexes
class cindex	{
public:
	int scanNum;
	string idRef;
	f_off offset;
};

//For instrument information
class instrumentInfo {
public:
	string analyzer;
	string detector;
	string id;
	string ionization;
	string manufacturer;
	string model;
	instrumentInfo(){
		analyzer="";
		detector="";
		id="";
		ionization="";
		manufacturer="";
		model="";
	}
	void clear(){
		analyzer="";
		detector="";
		id="";
		ionization="";
		manufacturer="";
		model="";
	}
};

class BasicSpectrum	{
public:

	//Constructors & Destructors
	BasicSpectrum();
	BasicSpectrum(const BasicSpectrum& s);
	~BasicSpectrum();

	//Operator overloads
	BasicSpectrum& operator=(const BasicSpectrum& s);
	specDP& operator[ ](const unsigned int index);

	//Modifiers
	void addDP(specDP dp);
	void clear();
	void setActivation(int a);
	void setBasePeakIntensity(double d);
	void setBasePeakMZ(double d);
	void setCentroid(bool b);
	void setCollisionEnergy(double d);
	void setCompensationVoltage(double d);
	void setHighMZ(double d);
	void setLowMZ(double d);
	void setMSLevel(int level);
	void setPeaksCount(int i);
	void setPositiveScan(bool b);
	void setPrecursorCharge(int z);
	void setPrecursorIntensity(double d);
	void setPrecursorMZ(double mz);
	void setPrecursorScanNum(int i);
	void setRTime(float t);
	void setScanIndex(int num);
	void setScanNum(int num);
	void setTotalIonCurrent(double d);

	//Accessors
	int						getActivation();
	double				getBasePeakIntensity();
	double				getBasePeakMZ();
	bool					getCentroid();
	double				getCollisionEnergy();
	double				getCompensationVoltage();
	double				getHighMZ();
	double				getLowMZ();
	int						getMSLevel();
	int						getPeaksCount();
	bool					getPositiveScan();
	int						getPrecursorCharge();
	double				getPrecursorIntensity();
	double				getPrecursorMZ();
	int						getPrecursorScanNum();
	float					getRTime(bool min=true);
	int						getScanIndex();
	int						getScanNum();
	double				getTotalIonCurrent();
	unsigned int	size();

protected:

	//Data Members (protected)
	int							activation;
	double					basePeakIntensity;
	double					basePeakMZ;
	bool						centroid;
	double					collisionEnergy;
	double					compensationVoltage;	//FAIMS compensation voltage
	double					highMZ;
	double					lowMZ;
	int							msLevel;
	int							peaksCount;
	bool						positiveScan;
	int							precursorCharge;			//Precursor ion charge; 0 if no precursor or unknown
	double					precursorIntensity;		//Precursor ion intensity; 0 if no precursor or unknown
	double					precursorMZ;					//Precursor ion m/z value; 0 if no precursor or unknown
	int							precursorScanNum;			//Precursor scan number; 0 if no precursor or unknown
	float						rTime;								//always stored in minutes
	int							scanIndex;						//when scan numbers aren't enough, there are indexes (start at 1)
	int							scanNum;							//identifying scan number
	double					totalIonCurrent;
	vector<specDP>	vData;								//Spectrum data points
	   
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
// X!Tandem headers
//------------------------------------------------

int b64_decode_mio (char *dest, char *src, size_t size);

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
	void setGZCompression(bool b);

	inline void setFileName(const char* fileName) {
		m_strFileName = fileName;
	}

	//  SAXHandler helper functions
	inline bool isElement(const char *n1, const XML_Char *n2)
	{	return (strcmp(n1, n2) == 0); }

	inline bool isAttr(const char *n1, const XML_Char *n2)
	{	return (strcmp(n1, n2) == 0); }

	inline const char* getAttrValue(const char* name, const XML_Char **attr) {
		for (int i = 0; attr[i]; i += 2) {
			if (isAttr(name, attr[i])) return attr[i + 1];
		}
		return "";
	}

protected:

	XML_Parser m_parser;
	string  m_strFileName;
	bool m_bStopParse;
	bool m_bGZCompression;
	FILE* fptr;
	Czran gzObj;

};

class SAXMzmlHandler : public mzpSAXHandler {
public:
	SAXMzmlHandler(BasicSpectrum* bs);
	~SAXMzmlHandler();

	//  Overrides of SAXHandler functions
	void startElement(const XML_Char *el, const XML_Char **attr);
	void endElement(const XML_Char *el);
	void characters(const XML_Char *s, int len);

	//  SAXMzmlHandler public functions
	vector<cindex>*					getIndex();
	f_off										getIndexOffset();
	vector<instrumentInfo>*	getInstrument();
	int											getPeaksCount();
	int											highScan();
	bool										load(const char* fileName);
	int											lowScan();
	bool										readHeader(int num=-1);
	bool										readSpectrum(int num=-1);
	
protected:

private:

	//  SAXMzmlHandler subclasses
	class cvParam	{
	public:
		string refGroupName;
		string name;
		string accession;
		string value;
		string unitAccession;
		string unitName;
	};

	//  SAXMzmlHandler private functions
	void	processData();
	void	processCVParam(const char* name, const char* accession, const char* value, const char* unitName="0", const char* unitAccession="0");
	void	pushSpectrum();	// Load current data into pvSpec, may have to guess charge
	f_off readIndexOffset();
	void	stopParser();

	//  SAXMzmlHandler Base64 conversion functions
	void decode32(vector<double>& d);
	void decode64(vector<double>& d);
	void decompress32(vector<double>& d);
	void decompress64(vector<double>& d);
	unsigned long dtohl(uint32_t l, bool bNet);
	uint64_t dtohl(uint64_t l, bool bNet);

	//  SAXMzmlHandler Flags indicating parser is inside a particular tag.
	bool m_bInIndexedMzML;
	bool m_bInRefGroup;
	bool m_bInmzArrayBinary;
	bool m_bInintenArrayBinary;
	bool m_bInSpectrumList;
	bool m_bInChromatogramList;
	bool m_bInIndexList;

	//  SAXMzmlHandler procedural flags.
	bool m_bCompressedData;
	bool m_bHeaderOnly;
	bool m_bLowPrecision;
	bool m_bNetworkData;	// i.e. big endian
	bool m_bNoIndex;
	bool m_bSpectrumIndex;
	
	//  SAXMzmlHandler index data members.
	vector<cindex>		m_vIndex;
	cindex						curIndex;
	int								posIndex;
	f_off							indexOffset;

	//  SAXMzmlHandler data members.
	string									m_ccurrentRefGroupName;
	instrumentInfo					m_instrument;
	int											m_peaksCount;						// Count of peaks in spectrum
	vector<cvParam>					m_refGroupCvParams;
	int											m_scanSPECCount;
	int											m_scanIDXCount;
	int											m_scanPRECCount;
	double									m_startTime;						//in minutes
	double									m_stopTime;							//in minutes
	string									m_strData;							// For collecting character data.
	vector<instrumentInfo>	m_vInstrument;
	BasicSpectrum*					spec;
	vector<double>					vdI;
	vector<double>					vdM;										// Peak list vectors (masses and charges)

};

class SAXMzxmlHandler : public mzpSAXHandler {
public:
	SAXMzxmlHandler(BasicSpectrum* bs);
	~SAXMzxmlHandler();

	//  Overrides of SAXHandler functions
	void startElement(const XML_Char *el, const XML_Char **attr);
	void endElement(const XML_Char *el);
	void characters(const XML_Char *s, int len);

	//  SAXMzxmlHandler public functions
	vector<cindex>*	getIndex();
	f_off						getIndexOffset();
	instrumentInfo	getInstrument();
	int							getPeaksCount();
	int							highScan();
	bool						load(const char* fileName);
	int							lowScan();
	bool						readHeader(int num=-1);
	bool						readSpectrum(int num=-1);
	
protected:

private:

	//  SAXMzxmlHandler private functions
	void	pushSpectrum();	// Load current data into pvSpec, may have to guess charge
	f_off readIndexOffset();
	void	stopParser();

	//  SAXMzxmlHandler Base64 conversion functions
	void decode32();
	void decode64();
	void decompress32();
	void decompress64();
	unsigned long dtohl(uint32_t l, bool bNet);
	uint64_t dtohl(uint64_t l, bool bNet);

	//  SAXMzxmlHandler Flags indicating parser is inside a particular tag.
	bool m_bInDataProcessing;
	bool m_bInIndex;
	bool m_bInMsInstrument;
	bool m_bInMsRun;
	bool m_bInPeaks;
	bool m_bInPrecursorMz;
	bool m_bInScan;

	//  SAXMzxmlHandler procedural flags.
	bool m_bCompressedData;
	bool m_bHeaderOnly;
	bool m_bLowPrecision;
	bool m_bNetworkData;	// i.e. big endian
	bool m_bNoIndex;
	bool m_bScanIndex;
	
	//  SAXMzxmlHandler index data members.
	vector<cindex>		m_vIndex;
	cindex						curIndex;
	int								posIndex;
	f_off							indexOffset;

	//  SAXMzxmlHandler data members.
	uLong										m_compressLen;	// For compressed data
	instrumentInfo					m_instrument;
	int											m_peaksCount;		// Count of peaks in spectrum
	string									m_strData;			// For collecting character data.
	vector<instrumentInfo>	m_vInstrument;
	BasicSpectrum*					spec;
	vector<double>					vdI;
	vector<double>					vdM;						// Peak list vectors (masses and charges)

};

//------------------------------------------------
// RAMP API
//------------------------------------------------
#ifndef _RAMPFACE_H
#define _RAMPFACE_H

#define INSTRUMENT_LENGTH 2000
#define SCANTYPE_LENGTH 32
#define CHARGEARRAY_LENGTH 128

typedef double RAMPREAL; 
typedef f_off ramp_fileoffset_t;

typedef struct RAMPFILE{
	BasicSpectrum* bs;
	SAXMzmlHandler* mzML;
	SAXMzxmlHandler* mzXML;
	int fileType;
	int bIsMzData;
	RAMPFILE(){
		bs=NULL;
		mzML=NULL;
		mzXML=NULL;
		fileType=0;
		bIsMzData=0;
	}
	~RAMPFILE(){
		if(bs!=NULL) delete bs;
		if(mzML!=NULL) delete mzML;
		if(mzXML!=NULL) delete mzXML;
		bs=NULL;
		mzML=NULL;
		mzXML=NULL;
		
	}
} RAMPFILE;

static vector<const char *> data_Ext;

struct ScanHeaderStruct {
   
   int								acquisitionNum; // scan number as declared in File (may be gaps)
	 int								mergedScan;  /* only if MS level > 1 */
   int								mergedResultScanNum; /* scan number of the resultant merged scan */
   int								mergedResultStartScanNum; /* smallest scan number of the scanOrigin for merged scan */
   int								mergedResultEndScanNum; /* largest scan number of the scanOrigin for merged scan */
   int								msLevel;
	 int								numPossibleCharges;
   int								peaksCount;
	 int								precursorCharge;  /* only if MS level > 1 */
	 int								precursorScanNum; /* only if MS level > 1 */
	 int								scanIndex; //a sequential index for non-sequential scan numbers (1-based)
   int								seqNum; // number in sequence observed file (1-based)
	 
   double							basePeakIntensity;
   double							basePeakMZ;
   double							collisionEnergy;
   double							highMZ;
   double							ionisationEnergy;
   double							lowMZ;
	 double							precursorIntensity;  /* only if MS level > 1 */
	 double							compensationVoltage;  /* only if MS level > 1 */
   double							precursorMZ;  /* only if MS level > 1 */
   double							retentionTime;        /* in seconds */
	 double							totIonCurrent;
   
   char								activationMethod[SCANTYPE_LENGTH];
   char								possibleCharges[SCANTYPE_LENGTH];
   char								scanType[SCANTYPE_LENGTH];
   
   bool								possibleChargesArray[CHARGEARRAY_LENGTH]; /* NOTE: does NOT include "precursorCharge" information; only from "possibleCharges" */
   
	 ramp_fileoffset_t	filePosition; /* where in the file is this header? */
};

struct RunHeaderStruct {
  int			scanCount;

	double	dEndTime;
	double	dStartTime;
	double	endMZ;
	double	highMZ;
  double	lowMZ;
  double	startMZ;
};

typedef struct InstrumentStruct {
   char manufacturer[INSTRUMENT_LENGTH];
   char model[INSTRUMENT_LENGTH];
   char ionisation[INSTRUMENT_LENGTH];
   char analyzer[INSTRUMENT_LENGTH];
   char detector[INSTRUMENT_LENGTH];
} InstrumentStruct;

struct ScanCacheStruct {
	int seqNumStart;    // scan at which the cache starts
	int size;           // number of scans in the cache
	struct ScanHeaderStruct *headers;
	RAMPREAL **peaks;
};

int									checkFileType(const char* fname);
ramp_fileoffset_t		getIndexOffset(RAMPFILE *pFI);
InstrumentStruct*		getInstrumentStruct(RAMPFILE *pFI);
void								getScanSpanRange(const struct ScanHeaderStruct *scanHeader, int *startScanNum, int *endScanNum);
void								rampCloseFile(RAMPFILE *pFI);
string							rampConstructInputFileName(const string &basename);
char*								rampConstructInputFileName(char *buf,int buflen,const char *basename);
char*								rampConstructInputPath(char *buf, int inbuflen, const char *dir_in, const char *basename);
const char**				rampListSupportedFileTypes();
RAMPFILE*						rampOpenFile(const char *filename);
char*								rampValidFileType(const char *buf);
void								readHeader(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex, struct ScanHeaderStruct *scanHeader);
ramp_fileoffset_t*	readIndex(RAMPFILE *pFI, ramp_fileoffset_t indexOffset, int *iLastScan);
int									readMsLevel(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex);
void								readMSRun(RAMPFILE *pFI, struct RunHeaderStruct *runHeader);
RAMPREAL*						readPeaks(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex);
int									readPeaksCount(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex);
void								readRunHeader(RAMPFILE *pFI, ramp_fileoffset_t *pScanIndex, struct RunHeaderStruct *runHeader, int iLastScan);

//MH:Cached RAMP functions
void														clearScanCache(struct ScanCacheStruct* cache);
void														freeScanCache(struct ScanCacheStruct* cache);
int															getCacheIndex(struct ScanCacheStruct* cache, int seqNum);
struct ScanCacheStruct*					getScanCache(int size);
const struct ScanHeaderStruct*	readHeaderCached(struct ScanCacheStruct* cache, int seqNum, RAMPFILE* pFI, ramp_fileoffset_t lScanIndex);
int															readMsLevelCached(struct ScanCacheStruct* cache, int seqNum, RAMPFILE* pFI, ramp_fileoffset_t lScanIndex);
const RAMPREAL*									readPeaksCached(struct ScanCacheStruct* cache, int seqNum, RAMPFILE* pFI, ramp_fileoffset_t lScanIndex);
void														shiftScanCache(struct ScanCacheStruct* cache, int nScans);

//MH:Unimplimented functions. These just bark cerr when used.
int									isScanAveraged(struct ScanHeaderStruct *scanHeader);
int									isScanMergedResult(struct ScanHeaderStruct *scanHeader);
int									rampSelfTest(char *filename);
char*								rampTrimBaseName(char *buf);
int									rampValidateOrDeriveInputFilename(char *inbuf, int inbuflen, char *spectrumName);
double							readStartMz(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex);
double							readEndMz(RAMPFILE *pFI, ramp_fileoffset_t lScanIndex);
void								setRampOption(long option);


#endif

//------------------------------------------------
// MzParser Interface
//------------------------------------------------
class MzParser {
public:
	//Constructors and Destructors
	MzParser(BasicSpectrum* s);
	~MzParser();

	//User functions
	int		highScan();
	bool	load(char* fname);
	int		lowScan();
	bool	readSpectrum(int num=0);
	bool  readSpectrumHeader(int num=0);

protected:
	SAXMzmlHandler* mzML;
	SAXMzxmlHandler* mzXML;

private:
	//private functions
	int checkFileType(char* fname);

	//private data members
	int							fileType;
	BasicSpectrum*	spec;

};

#endif
