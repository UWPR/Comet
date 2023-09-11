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

#ifndef _COMETSEARCHMANAGER_H_
#define _COMETSEARCHMANAGER_H_

#include "CometData.h"
#include "CometDataInternal.h"
#include "CometInterfaces.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdarg.h>
#ifdef _WIN32
#include <io.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <err.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

using namespace CometInterfaces;

class CometSearchManager : public ICometSearchManager
{
public:
   CometSearchManager();
   ~CometSearchManager();

   std::map<std::string, CometParam*>& GetParamsMap();

   // Methods inherited from ICometSearchManager
   virtual bool CreateIndex();
   virtual bool DoSearch();
   virtual bool InitializeSingleSpectrumSearch();
   virtual void FinalizeSingleSpectrumSearch();
   virtual bool DoSingleSpectrumSearch(const int iPrecursorCharge,
                                       const double dMZ,
                                       double* dMass,
                                       double* dInten,
                                       const int iNumPeaks,
                                       string& strReturnPeptide,
                                       string& strReturnProtein,
                                       vector<Fragment> & matchedFragments,
                                       Scores & pScores);
   virtual void AddInputFiles(vector<InputFileInfo*> &pvInputFiles);
   virtual void SetOutputFileBaseName(const char *pszBaseName);
   virtual void SetParam(const string &name, const string &strValue, const string &value);
   virtual bool GetParamValue(const string &name, string &value);
   virtual void SetParam(const string &name, const string &strValue, const bool &value);
   virtual bool GetParamValue(const string &name, bool &value);
   virtual void SetParam(const string &name, const string &strValue, const int &value);
   virtual bool GetParamValue(const string &name, int &value);
   virtual void SetParam(const string &name, const string &strValue, const long &value);
   virtual bool GetParamValue(const string &name, long &value);
   virtual void SetParam(const string &name, const string &strValue, const double &value);
   virtual bool GetParamValue(const string &name, double &value);
   virtual void SetParam(const string &name, const string &strValue, const VarMods &value);
   virtual bool GetParamValue(const string &name, VarMods &value);
   virtual void SetParam(const string &name, const string &strValue, const DoubleRange &value);
   virtual bool GetParamValue(const string &name, DoubleRange &value);
   virtual void SetParam(const string &name, const string &strValue, const IntRange &value);
   virtual bool GetParamValue(const string &name, IntRange &value);
   virtual void SetParam(const string &name, const string &strValue, const EnzymeInfo &value);
   virtual bool GetParamValue(const string &name, EnzymeInfo &value);
   virtual void SetParam(const string &name, const string &strValue, const vector<double> &value);
   virtual bool GetParamValue(const string &name, vector<double> &value);
   virtual bool IsValidCometVersion(const string &version);
   virtual bool IsSearchError();
   virtual void GetStatusMessage(string &strStatusMsg);
   virtual void CancelSearch();
   virtual bool IsCancelSearch();
   virtual void ResetSearchStatus();


private:
   bool InitializeStaticParams();
   static void UpdatePrevNextAA(int iWhichQuery,
                                int iPrintTargetDecoy);
   bool singleSearchInitializationComplete;
   int singleSearchThreadCount;
   std::map<std::string, CometParam*> _mapStaticParams;
};

#endif
