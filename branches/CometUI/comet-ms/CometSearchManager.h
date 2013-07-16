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

class CometSearchManager
{
public:
   CometSearchManager();
   ~CometSearchManager();

   void DoSearch();

private:
    void Initialize();
    void GetHostName();
    void UpdateInputFile(InputFileInfo *pFileInfo);
    void SetMSLevelFilter(MSReader &mstReader);
    void AllocateResultsMem();
    static bool compareByPeptideMass(Query const* a, Query const* b);
    void CalcRunTime(time_t tStartTime);
};

#endif