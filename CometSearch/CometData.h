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


#ifndef _COMETDATA_H_
#define _COMETDATA_H_

#define SIZE_BUF                    8192
#define SIZE_FILE                   4096
#define SIZE_FILE2                  SIZE_FILE + 600
#define SIZE_ERROR                  SIZE_FILE2 + 400

#define MAX_THREADS                 128

#define MAX_ENZYME_AA               20       // max # of AA for enzyme break point
#define MAX_VARMOD_AA               32       // max # of modified AAs in a peptide per variable modification

#define ENZYME_NAME_LEN             48

#define MAX_FRAGMENT_CHARGE         5
#define MAX_PRECURSOR_CHARGE        9
#define MAX_PRECURSOR_NL_SIZE       5

#define MAX_PERMUTATIONS            10000

#define SPARSE_MATRIX_SIZE          100

struct DoubleRange
{
   double dStart;
   double dEnd;

   DoubleRange()
   {
      dStart = 0.0;
      dEnd = 0.0;
   }

   DoubleRange(const DoubleRange& a)
   {
      dStart = a.dStart;
      dEnd = a.dEnd;
   }

   DoubleRange(double dStart_in, double dEnd_in)
   {
      dStart = dStart_in;
      dEnd = dEnd_in;
   }

   DoubleRange& operator=(DoubleRange& a)
   {
      dStart = a.dStart;
      dEnd = a.dEnd;
      return *this;
   }
};

struct IntRange
{
   int iStart;
   int iEnd;

   IntRange()
   {
      iStart = 0;
      iEnd = 0;
   }

   IntRange(const IntRange& a)
   {
      iStart = a.iStart;
      iEnd = a.iEnd;
   }

   IntRange(int iStart_in, int iEnd_in)
   {
      iStart = iStart_in;
      iEnd = iEnd_in;
   }

   IntRange& operator=(IntRange& a)
   {
      iStart = a.iStart;
      iEnd = a.iEnd;
      return *this;
   }
};

struct Scores
{
    double xCorr;
    double dSp;
    double dCn;
    double dExpect;
    double mass;
    int matchedIons;
    int totalIons;

    Scores() :
        xCorr(0),
        dSp(0),
        dCn(0),
        dExpect(0),
        mass(0),
        matchedIons(0),
        totalIons(0)
    { }

    Scores(double xCorr, double dSp, double dCn, double dExpect, double mass, int matchedIons, int totalIons) :
        xCorr(xCorr),
        dSp(dSp),
        dCn(dCn),
        dExpect(dExpect),
        mass(mass),
        matchedIons(matchedIons),
        totalIons(totalIons)
    { }

    Scores(const Scores& a) :
        xCorr(a.xCorr),
        dSp(a.dSp),
        dCn(a.dCn),
        dExpect(a.dExpect),
        mass(a.mass),
        matchedIons(a.matchedIons),
        totalIons(a.totalIons)
    { }

    Scores& operator=(Scores& a)
    {
        xCorr = a.xCorr;
        dSp = a.dSp;
        dCn = a.dCn;
        dExpect = a.dExpect;
        mass = a.mass;
        matchedIons = a.matchedIons;
        totalIons = a.totalIons;
        return *this;
    }
};

struct Fragment
{
    double mass;
    double intensity;
    int type;
    int number;
    int charge;
    bool neutralLoss;
    double neutralLossMass;

    Fragment() :
        mass(0),
        intensity(0),
        type(0),
        number(0),
        charge(0),
        neutralLoss(false),
        neutralLossMass(0)
    { }

    Fragment(double mass, double intensity, int type, int number, int charge) :
        mass(mass),
        intensity(intensity),
        type(type),
        number(number),
        charge(charge)
    { }

    Fragment(double mass, double intensity, int type, int number, int charge, bool neutralLoss, double neutralLossMass) :
        mass(mass),
        intensity(intensity),
        type(type),
        number(number),
        charge(charge),
        neutralLoss(neutralLoss),
        neutralLossMass(neutralLossMass)
     { }


    Fragment(const Fragment& a) :
        mass(a.mass),
        intensity(a.intensity),
        type(a.type),
        number(a.number),
        charge(a.charge),
        neutralLoss(a.neutralLoss),
        neutralLossMass(a.neutralLossMass)

    { }

    Fragment& operator=(Fragment& a)
    {
        mass = a.mass;
        intensity = a.intensity;
        type = a.type;
        number = a.number;
        charge = a.charge;
        neutralLoss = a.neutralLoss;
        neutralLossMass = a.neutralLossMass;
        return *this;
    }

    double ToMz()
    {
        return (mass + (charge - 1.0)*1.00727646688) / charge;
    }
};

struct VarMods
{
   double dVarModMass;
   double dNeutralLoss;
   int    iBinaryMod;
   int    iMaxNumVarModAAPerMod;
   int    iMinNumVarModAAPerMod;
   int    iVarModTermDistance;
   int    iWhichTerm;
   int    iRequireThisMod;  // 0=no; 1=required; negative number = different functionality allowing only one from a set of mods
   char   szVarModChar[MAX_VARMOD_AA];
   bool   bNtermMod;  // set to true if n-term mod
   bool   bCtermMod;  // set to true if c-term mod
   bool   bUseMod;    // set to true if non-zero mod mass

   VarMods()
   {
      iBinaryMod = 0;
      iRequireThisMod = 0;
      iMaxNumVarModAAPerMod = 0;
      iMinNumVarModAAPerMod = 0;
      iVarModTermDistance = -1;
      iWhichTerm = 0;
      dVarModMass = 0.0;
      dNeutralLoss = 0.0;
      szVarModChar[0] = '\0';
      bNtermMod = false;
      bCtermMod = false;
      bUseMod = false;
   }

   VarMods(const VarMods& a)
   {
      iBinaryMod = a.iBinaryMod;
      iRequireThisMod = a.iRequireThisMod;
      iMaxNumVarModAAPerMod = a.iMaxNumVarModAAPerMod;
      iMinNumVarModAAPerMod = a.iMinNumVarModAAPerMod;
      iVarModTermDistance = a.iVarModTermDistance;
      iWhichTerm = a.iWhichTerm;
      dVarModMass = a.dVarModMass;
      dNeutralLoss = a.dNeutralLoss;
      strcpy(szVarModChar, a.szVarModChar);
      bNtermMod  = a.bNtermMod;
      bCtermMod  = a.bCtermMod;
      bUseMod  = a.bUseMod;
   }

   VarMods& operator=(VarMods& a)
   {
      iBinaryMod = a.iBinaryMod;
      iRequireThisMod = a.iRequireThisMod;
      iMaxNumVarModAAPerMod = a.iMaxNumVarModAAPerMod;
      iMinNumVarModAAPerMod = a.iMinNumVarModAAPerMod;
      iVarModTermDistance = a.iVarModTermDistance;
      iWhichTerm = a.iWhichTerm;
      dVarModMass = a.dVarModMass;
      strcpy(szVarModChar, a.szVarModChar);
      dNeutralLoss = a.dNeutralLoss;
      bNtermMod  = a.bNtermMod;
      bCtermMod  = a.bCtermMod;
      bUseMod  = a.bUseMod;

      return *this;
   }
};

struct EnzymeInfo
{
   bool bNoEnzymeSelected;   // set to true if enzyme is no-enzyme (cut everwhere)
   bool bNoEnzyme2Selected;  // set to true if 2nd enzyme is no-enzyme (cut everywhere)

   int  iAllowedMissedCleavage;

   int  iSearchEnzymeOffSet;
   char szSearchEnzymeName[ENZYME_NAME_LEN];
   char szSearchEnzymeBreakAA[MAX_ENZYME_AA];
   char szSearchEnzymeNoBreakAA[MAX_ENZYME_AA];

   int  iSearchEnzyme2OffSet;   // 2nd enzyme; this will be set to -9 if second enzyme is not used
   char szSearchEnzyme2Name[ENZYME_NAME_LEN];
   char szSearchEnzyme2BreakAA[MAX_ENZYME_AA];
   char szSearchEnzyme2NoBreakAA[MAX_ENZYME_AA];

   int  iSampleEnzymeOffSet;
   char szSampleEnzymeName[ENZYME_NAME_LEN];
   char szSampleEnzymeBreakAA[MAX_ENZYME_AA];
   char szSampleEnzymeNoBreakAA[MAX_ENZYME_AA];

   EnzymeInfo()
   {
      bNoEnzymeSelected = 1;
      bNoEnzyme2Selected = 1;
      iAllowedMissedCleavage = 0;
      iSearchEnzymeOffSet = 1;
      iSearchEnzyme2OffSet = 0;
      iSampleEnzymeOffSet = 0;

      szSearchEnzymeName[0] = '\0';
      szSearchEnzymeBreakAA[0] = '\0';
      szSearchEnzymeNoBreakAA[0] = '\0';

      szSearchEnzyme2Name[0] = '\0';
      szSearchEnzyme2BreakAA[0] = '\0';
      szSearchEnzyme2NoBreakAA[0] = '\0';

      szSampleEnzymeName[0] = '\0';
      szSampleEnzymeBreakAA[0] = '\0';
      szSampleEnzymeNoBreakAA[0] = '\0';
   }

   EnzymeInfo(const EnzymeInfo& a)
   {
      bNoEnzymeSelected = a.bNoEnzymeSelected;
      bNoEnzyme2Selected = a.bNoEnzyme2Selected;
      iAllowedMissedCleavage = a.iAllowedMissedCleavage;
      iSearchEnzymeOffSet = a.iSearchEnzymeOffSet;
      iSearchEnzyme2OffSet = a.iSearchEnzyme2OffSet;
      iSampleEnzymeOffSet = a.iSampleEnzymeOffSet;

      for (int i = 0; i < ENZYME_NAME_LEN; ++i)
      {
         szSearchEnzymeName[i] = a.szSearchEnzymeName[i];
         szSearchEnzyme2Name[i] = a.szSearchEnzyme2Name[i];
         szSampleEnzymeName[i] = a.szSampleEnzymeName[i];
      }

      for (int i = 0; i < MAX_ENZYME_AA; ++i)
      {
         szSearchEnzymeBreakAA[i] = a.szSearchEnzymeBreakAA[i];
         szSearchEnzymeNoBreakAA[i] = a.szSearchEnzymeNoBreakAA[i];
         szSearchEnzyme2BreakAA[i] = a.szSearchEnzyme2BreakAA[i];
         szSearchEnzyme2NoBreakAA[i] = a.szSearchEnzyme2NoBreakAA[i];
         szSampleEnzymeBreakAA[i] = a.szSampleEnzymeBreakAA[i];
         szSampleEnzymeNoBreakAA[i] = a.szSampleEnzymeNoBreakAA[i];
      }
   }

   EnzymeInfo& operator=(EnzymeInfo& a)
   {
      bNoEnzymeSelected = a.bNoEnzymeSelected;
      bNoEnzyme2Selected = a.bNoEnzyme2Selected;
      iAllowedMissedCleavage = a.iAllowedMissedCleavage;
      iSearchEnzymeOffSet = a.iSearchEnzymeOffSet;
      iSearchEnzyme2OffSet = a.iSearchEnzyme2OffSet;
      iSampleEnzymeOffSet = a.iSampleEnzymeOffSet;

      for (int i = 0; i < ENZYME_NAME_LEN; ++i)
      {
         szSearchEnzymeName[i] = a.szSearchEnzymeName[i];
         szSearchEnzyme2Name[i] = a.szSearchEnzyme2Name[i];
         szSampleEnzymeName[i] = a.szSampleEnzymeName[i];
      }

      for (int i = 0; i < MAX_ENZYME_AA; ++i)
      {
         szSearchEnzymeBreakAA[i] = a.szSearchEnzymeBreakAA[i];
         szSearchEnzymeNoBreakAA[i] = a.szSearchEnzymeNoBreakAA[i];
         szSearchEnzyme2BreakAA[i] = a.szSearchEnzyme2BreakAA[i];
         szSearchEnzyme2NoBreakAA[i] = a.szSearchEnzyme2NoBreakAA[i];
         szSampleEnzymeBreakAA[i] = a.szSampleEnzymeBreakAA[i];
         szSampleEnzymeNoBreakAA[i] = a.szSampleEnzymeNoBreakAA[i];
      }

      return *this;
   }
};

// *IMPORTANT* If you change this enum, please also change the corresponding
// enum in CometDataWrapper.h in the CometWrapper namespace.
enum AnalysisType
{
   AnalysisType_Unknown = 0,
   AnalysisType_DTA,
   AnalysisType_SpecificScan,
   AnalysisType_SpecificScanRange,
   AnalysisType_EntireFile
};

// *IMPORTANT* If you change this enum, please also change the corresponding
// enum in CometDataWrapper.h in the CometWrapper namespace.
enum InputType
{
   InputType_UNKNOWN = -1,
   InputType_MS2 = 0,           // ms2, cms2, bms2, etc.
   InputType_MZXML,
   InputType_MZML,
   InputType_RAW,
   InputType_MGF
};

struct InputFileInfo
{
   int  iInputType;
   int  iAnalysisType;
   int  iFirstScan;         // for scan range specified with file on command line; otherwise replicates g_staticParams.options.scanRange.iStart
   int  iLastScan;
   char szFileName[SIZE_FILE];
   char szBaseName[SIZE_FILE];

   InputFileInfo()
   {
      iInputType = 0;
      iAnalysisType = AnalysisType_Unknown;
      iFirstScan = 0;
      iLastScan = 0;

      szFileName[0] = '\0';
      szBaseName[0] = '\0';
   }

   InputFileInfo(const InputFileInfo& inputObj)
   {
      iInputType = inputObj.iInputType;
      iAnalysisType = inputObj.iAnalysisType;
      iFirstScan = inputObj.iFirstScan;
      iLastScan = inputObj.iLastScan;

      szBaseName[0] = '\0';
      strcpy(szBaseName, inputObj.szBaseName);

      szFileName[0] = '\0';
      strcpy(szFileName, inputObj.szFileName);
   }

   InputFileInfo(char *pszFileName)
   {
      iInputType = 0;
      iAnalysisType = AnalysisType_Unknown;
      iFirstScan = 0;
      iLastScan = 0;

      szBaseName[0] = '\0';

      pszFileName[0] = '\0';
      strcpy(szFileName, pszFileName);
   }

   InputFileInfo& operator = (InputFileInfo &inputObj)
   {
      iInputType = inputObj.iInputType;
      iAnalysisType = inputObj.iAnalysisType;
      iFirstScan = inputObj.iFirstScan;
      iLastScan = inputObj.iLastScan;

      szBaseName[0] = '\0';
      strcpy(szBaseName, inputObj.szBaseName);

      szFileName[0] = '\0';
      strcpy(szFileName, inputObj.szFileName);
      return *this;
   }
};

struct SingleSpectrumStruct

{
   double dMass;
   double dInt;
};

enum CometParamType
{
   CometParamType_Unknown = 0,
   CometParamType_Bool,
   CometParamType_Int,
   CometParamType_Long,
   CometParamType_Double,
   CometParamType_String,
   CometParamType_VarMods,
   CometParamType_DoubleRange,
   CometParamType_IntRange,
   CometParamType_EnzymeInfo,
   CometParamType_DoubleVector
};


// A virtual class that provides a generic data structure to store any Comet
// parameter so that we can store all parameters in one data container
// (e.g. std::map). The specific type of parameter will use the TypedCometParam
// class which inherits from this class and specifies _paramType and
// _strValue, a string representation of the value of the param

class CometParam
{
public:
   CometParam(CometParamType paramType, const string& strValue)
      : _paramType(paramType), _strValue(strValue) {}
   virtual ~CometParam() {}
   string& GetStringValue() { return _strValue; }
private:
   CometParamType _paramType;
   string _strValue;
};


// A templated class to store Comet parameters of any type, specifying the type
// T upon creation. It inherits from CometParam so after creation, an object of
// this class type can be stored as a CometParam and cast back to
// TypedCometParam to access the GetValue() method when needed.

template< typename T >
class TypedCometParam : public CometParam
{
public:
   TypedCometParam (CometParamType paramType, const string& strValue, const T& value)
      : CometParam(paramType, strValue), _value(value) {}

   T& GetValue() { return _value; }

private:
   T _value;
};

#endif // _COMETDATA_H_
