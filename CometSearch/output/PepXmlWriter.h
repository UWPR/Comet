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

#ifndef _PEPXMLWRITER_H_
#define _PEPXMLWRITER_H_

#include "output/IResultWriter.h"
#include "CometWritePepXML.h"
#include "CometStatus.h"
#include "Common.h"

class PepXmlWriter : public IResultWriter
{
public:
   bool open(const WriterOpenCtx& ctx) override
   {
      BuildNames(ctx, ".pep.xml", ".decoy.pep.xml", ".target.pep.xml", _sTarget, _sDecoy);

      if ((_fpout = fopen(_sTarget.c_str(), "w")) == NULL)
      {
         std::string msg = " Error - cannot write to file \"" + _sTarget + "\".\n";
         g_cometStatus.SetStatus(CometResult_Failed, msg); logerr(msg);
         return false;
      }
      if (!CometWritePepXML::WritePepXMLHeader(_fpout, *ctx.pMgr))
         return false;

      if (ctx.iDecoySearch == 2)
      {
         if ((_fpoutd = fopen(_sDecoy.c_str(), "w")) == NULL)
         {
            std::string msg = " Error - cannot write to decoy file \"" + _sDecoy + "\".\n";
            g_cometStatus.SetStatus(CometResult_Failed, msg); logerr(msg);
            return false;
         }
         if (!CometWritePepXML::WritePepXMLHeader(_fpoutd, *ctx.pMgr))
            return false;
      }
      return true;
   }

   bool write(const WriterWriteCtx& ctx) override
   {
      CometWritePepXML::WritePepXML(_fpout, _fpoutd, ctx.fpdb, ctx.iScanOffset, *ctx.pQueries);
      return true;
   }

   void close(bool bSucceeded, bool bEmpty) override
   {
      if (_fpout)
      {
         if (bSucceeded) CometWritePepXML::WritePepXMLEndTags(_fpout);
         fclose(_fpout); _fpout = nullptr;
         if (bEmpty) remove(_sTarget.c_str());
      }
      if (_fpoutd)
      {
         if (bSucceeded) CometWritePepXML::WritePepXMLEndTags(_fpoutd);
         fclose(_fpoutd); _fpoutd = nullptr;
         if (bEmpty && !_sDecoy.empty()) remove(_sDecoy.c_str());
      }
   }

private:
   FILE*       _fpout  = nullptr;
   FILE*       _fpoutd = nullptr;
   std::string _sTarget;
   std::string _sDecoy;

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

#endif // _PEPXMLWRITER_H_
