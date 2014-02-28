/*
   Copyright 2014 University of Washington

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

#include "CometMemMgr.h"
#include "CometStatus.h"

CometMemMgr g_cometMemMgr;

#ifdef _WIN32

#define VIRTUAL_ALLOC_64KB_BLOCK    65536

CometMemMgr::CometMemMgr()
{
}

CometMemMgr::~CometMemMgr()
{
}

bool CometMemMgr::CometMemInit()
{
   HANDLE hProcess;
   if (0 == DuplicateHandle(GetCurrentProcess(), 
                            GetCurrentProcess(), 
                            GetCurrentProcess(),
                            &hProcess, 
                            0,
                            TRUE,
                            DUPLICATE_SAME_ACCESS))
   {
      char szErrorMsg[256];
      szErrorMsg[0] = '\0';
      sprintf(szErrorMsg,  " Error - CometMemMgr::CometMemInit::DuplicateHandle failed with error = 0x%lx.", GetLastError());

      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetError(true, strErrorMsg);      

      logerr("%s\n\n", szErrorMsg);

      return false;
   }

   MEMORYSTATUSEX statex;
   statex.dwLength = sizeof(statex);
   if (0 == GlobalMemoryStatusEx (&statex))
   {
      char szErrorMsg[256];
      szErrorMsg[0] = '\0';
      sprintf(szErrorMsg,  " Error - CometMemMgr::CometMemInit::GlobalMemoryStatusEx failed with error = 0x%lx.", GetLastError());

      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetError(true, strErrorMsg);      

      logerr("%s\n\n", szErrorMsg);
      return false;
   }
  
   if (0 == SetProcessWorkingSetSize(hProcess, statex.ullAvailPhys, statex.ullAvailPhys)) 
   {
      char szErrorMsg[256];
      szErrorMsg[0] = '\0';
      sprintf(szErrorMsg,  " Error - CometMemMgr::CometMemInit::SetProcessWorkingSetSize failed with error = 0x%lx.", GetLastError());

      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetError(true, strErrorMsg);      

      logerr("%s\n\n", szErrorMsg);

      return false;
   }

   return true;
}

void* CometMemMgr::CometMemAlloc(size_t size)
{
   size_t paddedSize = size;
   int iNumPages = size / VIRTUAL_ALLOC_64KB_BLOCK;
   if (0 != (size - (iNumPages * VIRTUAL_ALLOC_64KB_BLOCK)))
   {
       iNumPages++;
       paddedSize = iNumPages*VIRTUAL_ALLOC_64KB_BLOCK;
   }

   return VirtualAlloc(NULL, paddedSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void* CometMemMgr::CometMemAlloc(size_t num, size_t size)
{
   size_t actualSize = num*size;
   size_t paddedSize = actualSize;
   int iNumPages = actualSize/VIRTUAL_ALLOC_64KB_BLOCK;
   if (0 != (actualSize - (iNumPages*VIRTUAL_ALLOC_64KB_BLOCK)))
   {
       iNumPages++;
       paddedSize = iNumPages*VIRTUAL_ALLOC_64KB_BLOCK;
   }

   return VirtualAlloc(NULL, paddedSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

bool CometMemMgr::CometMemFree(void* pvAddress)
{
   if (0 == VirtualFree(pvAddress, 0, MEM_RELEASE))
   {
      char szErrorMsg[256];
      szErrorMsg[0] = '\0';
      sprintf(szErrorMsg,  " Error - CometMemMgr::CometMemFree::VirtualFree failed with error = 0x%lx.", GetLastError());

      string strErrorMsg(szErrorMsg);
      g_cometStatus.SetError(true, strErrorMsg);      

      logerr("%s\n\n", szErrorMsg);

      return false;
   }

   return true;
}

bool CometMemMgr::CometMemVirtualLock(void* pvAddress, size_t size)
{
   return VirtualLock(pvAddress, size);
}

bool CometMemMgr::CometMemVirtualUnlock(void* pvAddress, size_t size)
{
   return VirtualUnlock(pvAddress, size);
}

DWORD CometMemMgr::CometMemGetLastError()
{
   return GetLastError();
}

#else

CometMemMgr::CometMemMgr()
{
}

CometMemMgr::~CometMemMgr()
{
}

bool CometMemMgr::CometMemInit()
{
   return true;
}

void* CometMemMgr::CometMemAlloc(size_t size)
{
   return malloc(size);
}

void* CometMemMgr::CometMemAlloc(size_t num, size_t size)
{
   return calloc(num, size);
}

bool CometMemMgr::CometMemFree(void* pvAddress)
{
   free(pvAddress);
   return true;
}

bool CometMemMgr::CometMemVirtualLock(void* pvAddress, size_t size)
{
   return true;
}

bool CometMemMgr::CometMemVirtualUnlock(void* pvAddress, size_t size)
{
   return true;
}

DWORD CometMemMgr::CometMemGetLastError()
{
   return 0;
}

#endif // _WIN32
