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

#ifndef _COMETSTATUS_H_
#define _COMETSTATUS_H_

#include "Common.h"
#include "Threading.h"

enum CometResult
{
    CometResult_Succeeded = 0,
    CometResult_Failed,
    CometResult_Cancelled
};

class CometStatus
{
public:
   CometStatus()
   {
       _cometResult = CometResult_Succeeded;
      _strStatusMessage = "";

      Threading::CreateMutex(&_statusCheckMutex);
   }

   ~CometStatus()
   {
      Threading::DestroyMutex(_statusCheckMutex);
   }

   CometResult GetStatus()
   {
      CometResult result;
      Threading::LockMutex(_statusCheckMutex);
      result = _cometResult;
      Threading::UnlockMutex(_statusCheckMutex);

      return result;
   }

   CometResult GetStatus(std::string& strStatusMsg)
   {
      CometResult result;
      Threading::LockMutex(_statusCheckMutex);
      result = _cometResult;
      strStatusMsg = _strStatusMessage;
      Threading::UnlockMutex(_statusCheckMutex);

      return result;
   }

   void SetStatus(CometResult result)
   {
      Threading::LockMutex(_statusCheckMutex);
      _cometResult = result;
      Threading::UnlockMutex(_statusCheckMutex);
   }

   void SetStatus(CometResult result, const std::string &strStatusMsg)
   {
      Threading::LockMutex(_statusCheckMutex);
      _cometResult = result;
      _strStatusMessage = strStatusMsg;
      Threading::UnlockMutex(_statusCheckMutex);
   }

   void GetStatusMsg(std::string& strStatusMsg)
   {
      Threading::LockMutex(_statusCheckMutex);
      strStatusMsg = _strStatusMessage;
      Threading::UnlockMutex(_statusCheckMutex);
   }

   void SetStatusMsg(const std::string &strStatusMsg)
   {
      Threading::LockMutex(_statusCheckMutex);
      _strStatusMessage = strStatusMsg;
      Threading::UnlockMutex(_statusCheckMutex);
   }

   void ResetStatus()
   {
      Threading::LockMutex(_statusCheckMutex);
      _cometResult = CometResult_Succeeded;
      _strStatusMessage = "";
      Threading::UnlockMutex(_statusCheckMutex);
   }

   bool IsError()
   {
      bool bError;
      Threading::LockMutex(_statusCheckMutex);
      bError = _cometResult == CometResult_Failed;
      Threading::UnlockMutex(_statusCheckMutex);

      return bError;
   }

   bool IsCancel()
   {
      bool bCancelled;
      Threading::LockMutex(_statusCheckMutex);
      bCancelled = _cometResult == CometResult_Cancelled;
      Threading::UnlockMutex(_statusCheckMutex);

      return bCancelled;
   }

private:
   CometResult _cometResult;
   std::string _strStatusMessage;
   Mutex _statusCheckMutex;
};

extern CometStatus g_cometStatus;


#endif // _COMETSTATUS_H_
