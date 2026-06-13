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

#ifndef _PERCOLATORWRITER_H_
#define _PERCOLATORWRITER_H_

#include "output/IResultWriter.h"
#include "CometWritePercolator.h"
#include "CometStatus.h"
#include "Common.h"

class PercolatorWriter : public IResultWriter
{
public:
   bool open(const WriterOpenCtx& ctx) override
   {
      std::string base = std::string(ctx.szBaseName) + ctx.szOutputSuffix;
      std::string range;
      if (!ctx.bEntireFile)
         range = "." + std::to_string(ctx.iFirstScan) + "-" + std::to_string(ctx.iLastScan);
      _sPath = base + range + ".pin";

      _fpout = fopen(_sPath.c_str(), "w");
      if (!_fpout)
      {
         std::string msg = " Error - cannot write to file \"" + _sPath + "\".\n";
         g_cometStatus.SetStatus(CometResult_Failed, msg); logerr(msg);
         return false;
      }
      CometWritePercolator::WritePercolatorHeader(_fpout);
      return true;
   }

   bool write(const WriterWriteCtx& ctx) override
   {
      return CometWritePercolator::WritePercolator(_fpout, ctx.fpdb, *ctx.pQueries);
   }

   void close(bool /*bSucceeded*/, bool bEmpty) override
   {
      if (_fpout)
      {
         fclose(_fpout); _fpout = nullptr;
         if (bEmpty) remove(_sPath.c_str());
      }
   }

private:
   FILE*       _fpout = nullptr;
   std::string _sPath;
};

#endif // _PERCOLATORWRITER_H_
