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


#include "Common.h"
#include "CometSpecLib.h"
#include "CometSearch.h"
#include "ThreadPool.h"
#include "CometStatus.h"
#include "CometMassSpecUtils.h"

#include <stdio.h>


Mutex CometSpecLib::g_vSpecLibMutex;


CometSpecLib::CometSpecLib()
{
}

CometSpecLib::~CometSpecLib()
{
}


bool CometSpecLib::ReadSpecLib(void)
{
   FILE *fp;
   size_t tTmp;
   char szBuf[SIZE_BUF];

   if (g_bSpecLibRead)
      return true;

   if (sqlite file)
   {
      ReadSpecLibSqlite(g_staticParams.speclibInfo.strSpecLib);
   }
   else if (Thermo raw file)
   {
      ReadSpecLibRaw(g_staticParams.speclibInfo.strSpecLib);
   }
   else
   {
      printf(" Error, expecting sqlite .db or Thermo .raw file for the spectral library.\n");
      return false;
   }

   g_bSpecLibRead = true;

   return true;
}


bool CometSpecLib::ReadSpecLibSqlite(string strSpecLib)
{
}


bool CometSpecLib::ReadSpecLibRaw(string strSpecLib)
{
}


bool CometSpecLib::SearchSpecLib(int iWhichQuery,
                                 ThreadPool *tp
{
}
