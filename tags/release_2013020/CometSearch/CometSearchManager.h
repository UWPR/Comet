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

class CometSearchManager
{
public:
   CometSearchManager();
   ~CometSearchManager();

   void DoSearch();
   std::map<std::string, CometParam*>& GetParamsMap();
   void AddInputFiles(vector<InputFileInfo*> &pvInputFiles);
   void SetOutputFileBaseName(const char *pszBaseName);
   void SetParam(const string &name, const string &strValue, const string &value);
   bool GetParamValue(const string &name, string &value);
   void SetParam(const string &name, const string &strValue, const int &value);
   bool GetParamValue(const string &name, int &value);
   void SetParam(const string &name, const string &strValue, const double &value);
   bool GetParamValue(const string &name, double &value);
   void SetParam(const string &name, const string &strValue, const VarMods &value);
   bool GetParamValue(const string &name, VarMods &value);
   void SetParam(const string &name, const string &strValue, const DoubleRange &value);
   bool GetParamValue(const string &name, DoubleRange &value);
   void SetParam(const string &name, const string &strValue, const IntRange &value);
   bool GetParamValue(const string &name, IntRange &value);
   void SetParam(const string &name, const string &strValue, const EnzymeInfo &value);
   bool GetParamValue(const string &name, EnzymeInfo &value);   


private:
   void InitializeStaticParams();
   
   std::map<std::string, CometParam*> _mapStaticParams;
};

#endif
