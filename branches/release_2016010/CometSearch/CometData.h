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

#ifndef _COMETDATA_H_
#define _COMETDATA_H_

#define SIZE_BUF                    8192
#define SIZE_FILE                   512

#define MAX_THREADS                 64

#define MAX_ENZYME_AA               20       // max # of AA for enzyme break point
#define MAX_VARMOD_AA               20       // max # of modified AAs in a peptide per variable modification

#define ENZYME_NAME_LEN             48

#define MAX_FRAGMENT_CHARGE         5
#define MAX_PRECURSOR_CHARGE        9

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

struct VarMods
{
   double dVarModMass;
   int    iBinaryMod;
   int    bRequireThisMod;
   int    iMaxNumVarModAAPerMod;
   int    iVarModTermDistance;
   int    iWhichTerm;
   char   szVarModChar[MAX_VARMOD_AA];

   VarMods()
   {
      iBinaryMod = 0;
      bRequireThisMod = 0;
      iMaxNumVarModAAPerMod = 0;
      iVarModTermDistance = -1;
      iWhichTerm = 0;
      dVarModMass = 0.0;
      szVarModChar[0] = '\0';
   }

   VarMods(const VarMods& a)
   {
      iBinaryMod = a.iBinaryMod;
      bRequireThisMod = a.bRequireThisMod;
      iMaxNumVarModAAPerMod = a.iMaxNumVarModAAPerMod;
      iVarModTermDistance = a.iVarModTermDistance;
      iWhichTerm = a.iWhichTerm;
      dVarModMass = a.dVarModMass;
      strcpy(szVarModChar, a.szVarModChar);
   }

   VarMods& operator=(VarMods& a)
   {
      iBinaryMod = a.iBinaryMod;
      bRequireThisMod = a.bRequireThisMod;
      iMaxNumVarModAAPerMod = a.iMaxNumVarModAAPerMod;
      iVarModTermDistance = a.iVarModTermDistance;
      iWhichTerm = a.iWhichTerm;
      dVarModMass = a.dVarModMass;
      strcpy(szVarModChar, a.szVarModChar);

      return *this;
   }
};

struct EnzymeInfo
{
   int  iAllowedMissedCleavage;

   int  iSearchEnzymeOffSet;
   char szSearchEnzymeName[ENZYME_NAME_LEN];
   char szSearchEnzymeBreakAA[MAX_ENZYME_AA];
   char szSearchEnzymeNoBreakAA[MAX_ENZYME_AA];

   int  iSampleEnzymeOffSet;
   char szSampleEnzymeName[ENZYME_NAME_LEN];
   char szSampleEnzymeBreakAA[MAX_ENZYME_AA];
   char szSampleEnzymeNoBreakAA[MAX_ENZYME_AA];

   EnzymeInfo()
   {
      iAllowedMissedCleavage = 0;
      iSearchEnzymeOffSet = 0;
      iSampleEnzymeOffSet = 0;

      szSearchEnzymeName[0] = '\0';
      szSearchEnzymeBreakAA[0] = '\0';
      szSearchEnzymeNoBreakAA[0] = '\0';

      szSampleEnzymeName[0] = '\0';
      szSampleEnzymeBreakAA[0] = '\0';
      szSampleEnzymeNoBreakAA[0] = '\0';
   }

   EnzymeInfo(const EnzymeInfo& a)
   {
      iAllowedMissedCleavage = a.iAllowedMissedCleavage;
      iSearchEnzymeOffSet = a.iSearchEnzymeOffSet;
      iSampleEnzymeOffSet = a.iSampleEnzymeOffSet;

      int i;

      for (i = 0; i < ENZYME_NAME_LEN; i++)
      {
         szSearchEnzymeName[i] = a.szSearchEnzymeName[i];
         szSampleEnzymeName[i] = a.szSampleEnzymeName[i];
      }

      for (i = 0; i < MAX_ENZYME_AA; i++)
      {
         szSearchEnzymeBreakAA[i] = a.szSearchEnzymeBreakAA[i];
         szSearchEnzymeNoBreakAA[i] = a.szSearchEnzymeNoBreakAA[i];
         szSampleEnzymeBreakAA[i] = a.szSampleEnzymeBreakAA[i];
         szSampleEnzymeNoBreakAA[i] = a.szSampleEnzymeNoBreakAA[i];
      }
   }

   EnzymeInfo& operator=(EnzymeInfo& a)
   {
      iAllowedMissedCleavage = a.iAllowedMissedCleavage;
      iSearchEnzymeOffSet = a.iSearchEnzymeOffSet;
      iSampleEnzymeOffSet = a.iSampleEnzymeOffSet;

      int i;

      for (i = 0; i < ENZYME_NAME_LEN; i++)
      {
         szSearchEnzymeName[i] = a.szSearchEnzymeName[i];
         szSampleEnzymeName[i] = a.szSampleEnzymeName[i];
      }

      for (i = 0; i < MAX_ENZYME_AA; i++)
      {
         szSearchEnzymeBreakAA[i] = a.szSearchEnzymeBreakAA[i];
         szSearchEnzymeNoBreakAA[i] = a.szSearchEnzymeNoBreakAA[i];
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
   int  iFirstScan;
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

enum CometParamType
{
   CometParamType_Unknown = 0,
   CometParamType_Int,
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
