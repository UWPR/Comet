/*
MIT License

Copyright (c) 2023 University of Washington's Proteomics Resource

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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
