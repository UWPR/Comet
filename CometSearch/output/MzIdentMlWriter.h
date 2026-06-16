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

#ifndef _MZIDENTMLWRITER_H_
#define _MZIDENTMLWRITER_H_

#include "output/IResultWriter.h"
#include "CometWriteMzIdentML.h"
#include "CometStatus.h"
#include "Common.h"

class MzIdentMlWriter : public IResultWriter
{
public:
   explicit MzIdentMlWriter(CometSearchManager* pMgr) : _pMgr(pMgr) {}

   bool open(const WriterOpenCtx& ctx) override
   {
      _bIdxNoFasta = ctx.bIdxNoFasta;
      _pStatus = ctx.pStatus;
      BuildNames(ctx, ".mzid", ".decoy.mzid", _sTarget, _sDecoy, ".target.mzid");

      _fpout = fopen(_sTarget.c_str(), "w");
      if (!_fpout)
      {
         std::string msg = " Error - cannot write to file \"" + _sTarget + "\".\n";
         ctx.pStatus->SetStatus(CometResult_Failed, msg); logerr(msg);
         return false;
      }
      if (!OpenTmp(_sTarget, _sTgtTmp, _fpoutTmp, ctx.pStatus)) return false;

      if (ctx.iDecoySearch == 2)
      {
         _fpoutd = fopen(_sDecoy.c_str(), "w");
         if (!_fpoutd)
         {
            std::string msg = " Error - cannot write to decoy file \"" + _sDecoy + "\".\n";
            ctx.pStatus->SetStatus(CometResult_Failed, msg); logerr(msg);
            return false;
         }
         if (!OpenTmp(_sDecoy, _sDecTmp, _fpoutdTmp, ctx.pStatus)) return false;
      }
      return true;
   }

   bool write(const WriterWriteCtx& ctx) override
   {
      _fpdb = ctx.fpdb;   // remember for close()
      CometWriteMzIdentML::WriteMzIdentMLTmp(_fpoutTmp, _fpoutdTmp, ctx.iBatchNum, *ctx.pQueries);
      return true;
   }

   void close(bool bSucceeded, bool bEmpty) override
   {
      FinalizeOne(_fpout, _fpoutTmp, _sTgtTmp, bSucceeded, bEmpty);
      FinalizeOne(_fpoutd, _fpoutdTmp, _sDecTmp, bSucceeded, bEmpty);
      if (bEmpty)
      {
         if (!_sTarget.empty()) remove(_sTarget.c_str());
         if (!_sDecoy.empty())  remove(_sDecoy.c_str());
         if (!_sTgtTmp.empty()) remove(_sTgtTmp.c_str());
         if (!_sDecTmp.empty()) remove(_sDecTmp.c_str());
      }
   }

private:
   CometSearchManager* _pMgr        = nullptr;
   CometStatus*        _pStatus     = nullptr;
   FILE*               _fpout       = nullptr;
   FILE*               _fpoutd      = nullptr;
   FILE*               _fpoutTmp    = nullptr;
   FILE*               _fpoutdTmp   = nullptr;
   FILE*               _fpdb        = nullptr;
   bool                _bIdxNoFasta = false;
   std::string _sTarget, _sDecoy, _sTgtTmp, _sDecTmp;

   bool OpenTmp(const std::string& sBase, std::string& sTmp, FILE*& fp, CometStatus* pStatus)
   {
      sTmp = sBase + ".XXXXXX";
      bool bTmpOk;
#ifdef _WIN32
      bTmpOk = (_mktemp_s(&sTmp[0], sTmp.size() + 1) == 0);
#else
      {
         int fd = mkstemp(&sTmp[0]);
         if (fd != -1) ::close(fd);   // release kernel fd; fopen below opens its own handle
         bTmpOk = (fd != -1);
      }
#endif
      if (!bTmpOk)
      {
         std::string msg = " Error - cannot create temporary file \"" + sTmp + "\".\n";
         pStatus->SetStatus(CometResult_Failed, msg); logerr(msg);
         return false;
      }
      fp = fopen(sTmp.c_str(), "w");
      if (!fp)
      {
         std::string msg = " Error - cannot write to temporary file \"" + sTmp + "\".\n";
         pStatus->SetStatus(CometResult_Failed, msg); logerr(msg);
         return false;
      }
      return true;
   }

   void FinalizeOne(FILE*& fpFinal, FILE*& fpTmp, const std::string& sTmp,
                    bool bSucceeded, bool bEmpty)
   {
      if (!fpFinal) return;
      if (bSucceeded && fpTmp)
      {
         fclose(fpTmp);
         fpTmp = fopen(sTmp.c_str(), "r");
         if (fpTmp)
         {
            CometWriteMzIdentML::WriteMzIdentML(fpFinal, _fpdb, sTmp, *_pMgr, _bIdxNoFasta);
            fclose(fpTmp); fpTmp = nullptr;
            if (!bEmpty) remove(sTmp.c_str());
         }
         else
         {
            std::string msg = " Error - cannot reopen temporary mzIdentML file \"" + sTmp + "\" for merge.\n";
            _pStatus->SetStatus(CometResult_Failed, msg); logerr(msg);
         }
      }
      else if (fpTmp)
      {
         fclose(fpTmp); fpTmp = nullptr;
         if (!bEmpty) remove(sTmp.c_str());
      }
      fclose(fpFinal); fpFinal = nullptr;
   }

};

#endif // _MZIDENTMLWRITER_H_
