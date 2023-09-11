/*
MIT License

Copyright (c) 2023 University of Washington's Proteomics Resource

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _COMETINTERFACES_H_
#define _COMETINTERFACES_H_

#include "Common.h"
#include "CometData.h"
#include "Threading.h"
#include "ThreadPool.h"

using namespace std;

namespace CometInterfaces
{
   class ICometSearchManager
   {
public:
      virtual ~ICometSearchManager() {}
      virtual bool CreateIndex() = 0;
      virtual bool DoSearch() = 0;
      virtual bool InitializeSingleSpectrumSearch() = 0;
      virtual void FinalizeSingleSpectrumSearch() = 0;
      virtual bool DoSingleSpectrumSearch(const int iPrecursorCharge,
                                          const double dMZ,
                                          double* dMass,
                                          double* dInten,
                                          const int iNumPeaks,
                                          string& strReturnPeptide,
                                          string& strReturnProtein,
                                          vector<Fragment> & matchedFragments,
                                          Scores & scores) = 0;
      virtual void AddInputFiles(vector<InputFileInfo*> &pvInputFiles) = 0;
      virtual void SetOutputFileBaseName(const char *pszBaseName) = 0;
      virtual void SetParam(const string &name, const string &strValue, const string &value) = 0;
      virtual bool GetParamValue(const string &name, string &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const int &value) = 0;
      virtual bool GetParamValue(const string &name, int &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const long &value) = 0;
      virtual bool GetParamValue(const string &name, long &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const double &value) = 0;
      virtual bool GetParamValue(const string &name, double &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const VarMods &value) = 0;
      virtual bool GetParamValue(const string &name, VarMods &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const DoubleRange &value) = 0;
      virtual bool GetParamValue(const string &name, DoubleRange &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const IntRange &value) = 0;
      virtual bool GetParamValue(const string &name, IntRange &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const EnzymeInfo &value) = 0;
      virtual bool GetParamValue(const string &name, EnzymeInfo &value) = 0;
      virtual void SetParam(const string &name, const string &strValue, const vector<double> &value) = 0;
      virtual bool GetParamValue(const string &name, vector<double> &value) = 0;
      virtual bool IsValidCometVersion(const string &version) = 0;
      virtual bool IsSearchError() = 0;
      virtual void GetStatusMessage(string &strStatusMsg) = 0;
      virtual void CancelSearch() = 0;
      virtual bool IsCancelSearch() = 0;
      virtual void ResetSearchStatus() = 0;
   };

   ICometSearchManager *GetCometSearchManager();
   void ReleaseCometSearchManager();

   static ThreadPool* _tp;
}

#endif // _COMETINTERFACES_H_
