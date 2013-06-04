#ifndef _SPECTRUM_H
#define _SPECTRUM_H

#include "MSToolkitTypes.h"
#include <vector>
#include <cstring>
#include <cstdlib>
#include <iomanip>

using namespace std;

class Spectrum {
 public:
  //Constructors & Destructors
  Spectrum();
  Spectrum(char*);
  Spectrum(char, unsigned int);
  Spectrum(const Spectrum&);
  ~Spectrum();

  //Operator Functions
  Spectrum& operator=(const Spectrum&);
	Peak_T& operator[](const int&);

  //Functions
  void	    			add(Peak_T&);
  void			    	add(double,float);
  void            addEZState(int,double,float,float);
  void            addEZState(EZState&);
	void						addMZ(double);
  void    				addZState(int,double);
  void		    		addZState(ZState&);
  Peak_T&			    at(const int&);
  Peak_T&	    		at(const unsigned int&);
  EZState&        atEZ(const int&);
  EZState&        atEZ(const unsigned int&);
  ZState&			    atZ(const int&);
  ZState&	    		atZ(const unsigned int&);
  void			    	clear();
	void						clearMZ();
	void						clearPeaks();
  void				    erase(unsigned int);
  void				    erase(unsigned int, unsigned int);
  void            eraseEZ(unsigned int);
  void            eraseEZ(unsigned int, unsigned int);
  void				    eraseZ(unsigned int);
  void				    eraseZ(unsigned int, unsigned int);
  MSActivation    getActivationMethod();
  float           getArea();
  float           getBPI();
  double          getBPM();
  int             getCentroidStatus();
  int				      getCharge();
  double          getConversionA();
  double          getConversionB();
  double          getConversionC();
  double          getConversionD();
  double          getConversionE();
  double          getConversionI();
  MSSpectrumType  getFileType();
  float           getIonInjectionTime();
  double    			getMZ(int index=0);
  bool            getRawFilter(char*,int,bool bLock=false);
  float		    		getRTime();
  float           getRTimeApex();
  int	      			getScanNumber(bool second=false);
  double          getTIC();
  int             getMsLevel();
  void            setActivationMethod(MSActivation);
  void            setArea(float);
  void            setBPI(float);
  void            setBPM(double);
  void            setCentroidStatus(int);
  void			    	setCharge(int);
  void            setConversionA(double);
  void            setConversionB(double);
  void            setConversionC(double);
  void            setConversionD(double);
  void            setConversionE(double);
  void            setConversionI(double);
  void    				setFileType(MSSpectrumType);
  void            setIonInjectionTime(float);
  void		    		setMZ(double);
  void            setRawFilter(char*);
  void				    setRTime(float);
  void            setRTimeApex(float);
  void    				setScanNumber(int, bool second=false);
  void            setTIC(double);
  void            setMsLevel(int level);
  int			      	size();
  int             sizeEZ();
	int							sizeMZ();
  int     				sizeZ();
  void		    		sortIntensity();
  void				    sortIntensityRev();
  void    				sortMZ();
  void            setPeaks( std::vector<Peak_T> peaks);
  void		    		sortMZRev();

  //for sqlite format
  void setScanID(int scanID);
  int getScanID();

  //const vector<Peak_T>* getPeaks();
  vector<Peak_T>* getPeaks();
  //void setPeaks(vector<Peak_T> peaks);
  float getTotalIntensity();
 
  //for debugging
  void printMe();

 protected:

 //Data Members
  vector<Peak_T>   *vPeaks;
  vector<EZState>  *vEZ;
  vector<ZState>   *vZ;
  int		           charge;
  float		         rTime;
  int		           scanNumber;
  int              scanNumber2;
  int              msLevel;
  vector<double>   *mz;
  MSSpectrumType   fileType;
  MSActivation     actMethod;
  int              scanID;       //index for sqlite
  float            IIT;
  float            BPI;          //Base Peak Intensity
  double           convA;
  double           convB;
	double           convC;
	double           convD;
	double           convE;
	double           convI;
  double           TIC;
  double           BPM;          //Base Peak Mass
  float            rTimeApex;    //retention time of precursor apex (MS2)
  float            area;         //summed peak areas of precursor (MS2)
  char             rawFilter[256]; //RAW file header line
  int              centroidStatus; //0=profile, 1=centroid, 2=unknown

  //private:
  //Functions
  static int compareIntensity(const void *p1,const void *p2);
  static int compareMZ(const void *p1,const void *p2);
  static int compareIntensityRev(const void *p1,const void *p2);
  static int compareMZRev(const void *p1,const void *p2);

};


#endif

