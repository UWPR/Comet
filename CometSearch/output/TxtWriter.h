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

#ifndef _TXTWRITER_H_
#define _TXTWRITER_H_

#include "output/IResultWriter.h"
#include "CometWriteTxt.h"
#include "CometStatus.h"
#include "Common.h"

class TxtWriter : public IResultWriter
{
public:
   bool open(const WriterOpenCtx& ctx) override
   {
      std::string ext       = std::string(".")        + ctx.szTxtFileExt;
      std::string extDecoy  = std::string(".decoy.")  + ctx.szTxtFileExt;
      std::string extTarget = std::string(".target.") + ctx.szTxtFileExt;
      BuildNames(ctx, ext.c_str(), extDecoy.c_str(), _sTarget, _sDecoy, extTarget.c_str());

      if ((_fpout = fopen(_sTarget.c_str(), "w")) == NULL)
      {
         std::string msg = " Error - cannot write to file \"" + _sTarget + "\".\n";
         ctx.pStatus->SetStatus(CometResult_Failed, msg); logerr(msg);
         return false;
      }
      CometWriteTxt::PrintTxtHeader(_fpout);
      fflush(_fpout);

      if (ctx.iDecoySearch == 2)
      {
         if ((_fpoutd = fopen(_sDecoy.c_str(), "w")) == NULL)
         {
            std::string msg = " Error - cannot write to decoy file \"" + _sDecoy + "\".\n";
            ctx.pStatus->SetStatus(CometResult_Failed, msg); logerr(msg);
            return false;
         }
         CometWriteTxt::PrintTxtHeader(_fpoutd);
      }
      return true;
   }

   bool write(const WriterWriteCtx& ctx) override
   {
      CometWriteTxt::WriteTxt(_fpout, _fpoutd, ctx.fpdb, *ctx.pQueries);
      return true;
   }

   void close(bool /*bSucceeded*/, bool bEmpty) override
   {
      if (_fpout)
      {
         fclose(_fpout); _fpout = nullptr;
         if (bEmpty) remove(_sTarget.c_str());
      }
      if (_fpoutd)
      {
         fclose(_fpoutd); _fpoutd = nullptr;
         if (bEmpty && !_sDecoy.empty()) remove(_sDecoy.c_str());
      }
   }

private:
   FILE*       _fpout  = nullptr;
   FILE*       _fpoutd = nullptr;
   std::string _sTarget;
   std::string _sDecoy;

};

#endif // _TXTWRITER_H_
