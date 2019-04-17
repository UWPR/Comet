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

#include "CometInterfaces.h"
#include "CometSearchManager.h"

using namespace CometInterfaces;

CometSearchManager *g_pCometSearchManager = NULL;

ICometSearchManager *CometInterfaces::GetCometSearchManager()
{
   if (NULL == g_pCometSearchManager)
   {
      g_pCometSearchManager = new CometSearchManager();
   }

   ICometSearchManager *pCometSearchMgr = static_cast<ICometSearchManager*>(g_pCometSearchManager);
   return pCometSearchMgr;
}

void CometInterfaces::ReleaseCometSearchManager()
{
   if (NULL != g_pCometSearchManager)
   {
      delete g_pCometSearchManager;
      g_pCometSearchManager = NULL;
   }
}


