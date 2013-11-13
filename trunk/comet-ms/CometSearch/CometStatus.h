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

class CometStatus
{
public:
   CometStatus()
   {
       _bError = false;
       _strErrorMessage = "";

       Threading::CreateMutex(&_statusCheckMutex);
   }

   ~CometStatus()
   {
       Threading::DestroyMutex(_statusCheckMutex);
   }

   void GetError(bool &bError)
   {
       Threading::LockMutex(_statusCheckMutex);
       bError = _bError;
       Threading::UnlockMutex(_statusCheckMutex);
   }

   void SetError(bool bError)
   {
       Threading::LockMutex(_statusCheckMutex);
       _bError = bError;
       Threading::UnlockMutex(_statusCheckMutex);
   }

   void SetError(bool bError, std::string& strErrorMsg)
   {
       Threading::LockMutex(_statusCheckMutex);
       _bError = bError;
       _strErrorMessage = strErrorMsg;
       Threading::UnlockMutex(_statusCheckMutex);
   }

   void GetErrorMsg(std::string& strErrorMsg)
   {
       Threading::LockMutex(_statusCheckMutex);
       strErrorMsg = _strErrorMessage;
       Threading::UnlockMutex(_statusCheckMutex);
   }

   void SetErrorMsg(std::string &strErrorMsg)
   {
       Threading::LockMutex(_statusCheckMutex);
       _strErrorMessage = strErrorMsg;
       Threading::UnlockMutex(_statusCheckMutex);
   }

private:
   bool _bError;
   std::string _strErrorMessage;
   Mutex _statusCheckMutex;
};

extern CometStatus g_cometStatus;


#endif // _COMETSTATUS_H_
