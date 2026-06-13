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
      BuildNames(ctx, ".mzid", ".decoy.mzid", ".target.mzid", _sTarget, _sDecoy);

      _fpout = fopen(_sTarget.c_str(), "w");
      if (!_fpout)
      {
         std::string msg = " Error - cannot write to file \"" + _sTarget + "\".\n";
         g_cometStatus.SetStatus(CometResult_Failed, msg); logerr(msg);
         return false;
      }
      if (!OpenTmp(_sTarget, _sTgtTmp, _fpoutTmp)) return false;

      if (ctx.iDecoySearch == 2)
      {
         _fpoutd = fopen(_sDecoy.c_str(), "w");
         if (!_fpoutd)
         {
            std::string msg = " Error - cannot write to decoy file \"" + _sDecoy + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, msg); logerr(msg);
            return false;
         }
         if (!OpenTmp(_sDecoy, _sDecTmp, _fpoutdTmp)) return false;
      }
      return true;
   }

   bool write(const WriterWriteCtx& ctx) override
   {
      _fpdb = ctx.fpdb;   // remember for close()
      CometWriteMzIdentML::WriteMzIdentMLTmp(_fpoutTmp, _fpoutdTmp, ctx.iBatchNum);
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
   CometSearchManager* _pMgr     = nullptr;
   FILE*               _fpout    = nullptr;
   FILE*               _fpoutd   = nullptr;
   FILE*               _fpoutTmp  = nullptr;
   FILE*               _fpoutdTmp = nullptr;
   FILE*               _fpdb     = nullptr;
   std::string _sTarget, _sDecoy, _sTgtTmp, _sDecTmp;

   bool OpenTmp(const std::string& sBase, std::string& sTmp, FILE*& fp)
   {
      sTmp = sBase + ".XXXXXX";
#ifdef _WIN32
      if (_mktemp_s(&sTmp[0], sTmp.size() + 1) != 0)
#else
      if (mkstemp(&sTmp[0]) == -1)
#endif
      {
         std::string msg = " Error - cannot create temporary file \"" + sTmp + "\".\n";
         g_cometStatus.SetStatus(CometResult_Failed, msg); logerr(msg);
         return false;
      }
      fp = fopen(sTmp.c_str(), "w");
      if (!fp)
      {
         std::string msg = " Error - cannot write to temporary file \"" + sTmp + "\".\n";
         g_cometStatus.SetStatus(CometResult_Failed, msg); logerr(msg);
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
            CometWriteMzIdentML::WriteMzIdentML(fpFinal, _fpdb, sTmp, *_pMgr);
            fclose(fpTmp); fpTmp = nullptr;
            if (!bEmpty) remove(sTmp.c_str());
         }
      }
      else if (fpTmp)
      {
         fclose(fpTmp); fpTmp = nullptr;
         if (!bEmpty) remove(sTmp.c_str());
      }
      fclose(fpFinal); fpFinal = nullptr;
   }

   static void BuildNames(const WriterOpenCtx& ctx,
                          const char* ext,
                          const char* extDecoy,
                          const char* extTargetCrux,
                          std::string& sTarget,
                          std::string& sDecoy)
   {
      std::string base = std::string(ctx.szBaseName) + ctx.szOutputSuffix;
      std::string range;
      if (!ctx.bEntireFile)
         range = "." + std::to_string(ctx.iFirstScan) + "-" + std::to_string(ctx.iLastScan);
#ifdef CRUX
      if (ctx.iDecoySearch == 2)
         { sTarget = base + range + extTargetCrux; sDecoy = base + range + extDecoy; }
      else
         sTarget = base + range + ext;
#else
      (void)extTargetCrux;
      sTarget = base + range + ext;
      if (ctx.iDecoySearch == 2) sDecoy = base + range + extDecoy;
#endif
   }
};

#endif // _MZIDENTMLWRITER_H_
