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

#include "CometPreprocess.h"
#include "CometData.h"

class CometSearchManager
{
public:
   //CometSearchManager(StaticParams &staticParams, vector<InputFileInfo*> &pvInputFiles, char *pszParamsFile);
   CometSearchManager();
   ~CometSearchManager();

   void DoSearch();
   void SetParam(const string &name, const string &strValue, const string &value);
   bool GetParam(const string &name, string &value);
   void SetParam(const string &name, const string &strValue, const int &value);
   bool GetParam(const string &name, int &value);
   void SetParam(const string &name, const string &strValue, const double &value);
   bool GetParam(const string &name, double &value);
   void SetParam(const string &name, const string &strValue, const VarMods &value);
   bool GetParam(const string &name, VarMods &value);
   void SetParam(const string &name, const string &strValue, const DoubleRange &value);
   bool GetParam(const string &name, DoubleRange &value);
   void SetParam(const string &name, const string &strValue, const IntRange &value);
   bool GetParam(const string &name, IntRange &value);
   void SetParam(const string &name, const string &strValue, const EnzymeInfo &value);
   bool GetParam(const string &name, EnzymeInfo &value);
   void AddInputFiles(vector<InputFileInfo*> &pvInputFiles);
   StaticParams& GetStaticParams();
   void SetStaticParams(StaticParams &staticParams);
   void InitializeStaticParams();

private:
    void GetHostName();
    void UpdateInputFile(InputFileInfo *pFileInfo);
    void SetMSLevelFilter(MSReader &mstReader);
    void AllocateResultsMem();
    static bool compareByPeptideMass(Query const* a, Query const* b);
    void CalcRunTime(time_t tStartTime);

    bool _bStaticParamsInitialized;
    std::string _strParamsFile;
    std::map<std::string, CometParam*> _mapStaticParams;
};

#endif
