// Copyright 2012-2026 Jimmy Eng
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


#include "CometInterfaces.h"
#include "CometSearchManager.h"
#include <mutex>

using namespace CometInterfaces;

CometSearchManager *g_pCometSearchManager = NULL;

// Reference-counted: every CometSearchManagerWrapper instance calls
// GetCometSearchManager() once (constructor) and ReleaseCometSearchManager() once
// (destructor/Dispose), but they all share this one process-wide native singleton.
// With no refcounting, disposing any one live wrapper deleted the singleton out from
// under every other still-live wrapper -- a use-after-free on their next call. The
// mutex guards both functions since multiple wrapper instances can construct/dispose
// concurrently from different C# threads.
static int g_iCometSearchManagerRefCount = 0;
static std::mutex g_cometSearchManagerMutex;

ICometSearchManager *CometInterfaces::GetCometSearchManager()
{
   std::lock_guard<std::mutex> lock(g_cometSearchManagerMutex);

   if (NULL == g_pCometSearchManager)
   {
      g_pCometSearchManager = new CometSearchManager();

      // reset the static parameters to their default if a new manager is created.
      g_staticParams.RestoreDefaults();
   }

   ++g_iCometSearchManagerRefCount;

   ICometSearchManager *pCometSearchMgr = static_cast<ICometSearchManager*>(g_pCometSearchManager);
   return pCometSearchMgr;
}

void CometInterfaces::ReleaseCometSearchManager()
{
   std::lock_guard<std::mutex> lock(g_cometSearchManagerMutex);

   if (NULL != g_pCometSearchManager && --g_iCometSearchManagerRefCount <= 0)
   {
      delete g_pCometSearchManager;
      g_pCometSearchManager = NULL;
      g_iCometSearchManagerRefCount = 0;
   }
}


