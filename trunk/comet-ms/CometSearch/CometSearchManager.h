/*
   Copyright 2013 University of Washington

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

#ifndef _COMETSEARCHMANAGER_H_
#define _COMETSEARCHMANAGER_H_

#include "CometData.h"
#include "CometInterfaces.h"

using namespace CometInterfaces;

class CometSearchManager : public ICometSearchManager
{
public:
   CometSearchManager();
   ~CometSearchManager();

   std::map<std::string, CometParam*>& GetParamsMap();

   // Methods inherited from ICometSearchManager
   virtual bool DoSearch();
   virtual void AddInputFiles(vector<InputFileInfo*> &pvInputFiles);
   virtual void SetOutputFileBaseName(const char *pszBaseName);
   virtual void SetParam(const string &name, const string &strValue, const string &value);
   virtual bool GetParamValue(const string &name, string &value);
   virtual void SetParam(const string &name, const string &strValue, const int &value);
   virtual bool GetParamValue(const string &name, int &value);
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

   std::map<std::string, CometParam*> _mapStaticParams;
};

#endif
