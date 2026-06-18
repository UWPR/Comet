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

#ifndef _IRESULTWRITER_H_
#define _IRESULTWRITER_H_

#include <cstdio>
#include <string>
#include <vector>

class CometSearchManager;
class CometStatus;
struct Query;

// Parameters passed to each writer's open() method.
struct WriterOpenCtx
{
   // pStatus is required: every writer's open() dereferences it unconditionally on
   // the file-open-failure path with no null check, so making it constructor-only
   // (rather than a default-nullptr field set later like the rest of this struct)
   // turns "forgot to set pStatus" from a runtime null-pointer crash into a compile
   // error at the construction site.
   explicit WriterOpenCtx(CometStatus& status) : pStatus(&status) {}

   const char* szBaseName       = nullptr;
   const char* szOutputSuffix   = nullptr;
   const char* szTxtFileExt     = nullptr;     // TxtWriter only
   bool        bEntireFile      = false;       // true => no scan-range suffix on output name
   int         iFirstScan       = 0;
   int         iLastScan        = 0;
   int         iDecoySearch     = 0;            // 0=off, 1=concat, 2=separate
   bool        bIdxNoFasta      = false;        // .idx DB with no companion .fasta (mzIdentML)
   CometSearchManager* pMgr     = nullptr;       // for format headers that need ICometSearchManager
   CometStatus* const  pStatus;                  // session error/cancel state; never null, set once above
};

// Parameters passed to each writer's write() method (per-batch).
struct WriterWriteCtx
{
   FILE* fpdb;
   int   iScanOffset;    // iTotalSpectraSearched - queries.size(); pepXML only
   int   iBatchNum;      // mzIdentML only
   const std::vector<Query*>* pQueries;  // batch query results for this write call
};

class IResultWriter
{
public:
   virtual ~IResultWriter() = default;

   // Open output file(s) and write format header.
   // Returns false on error.
   virtual bool open(const WriterOpenCtx& ctx) = 0;

   // Write all results in ctx.pQueries (the current batch's session.queries) for one batch.
   // Returns false on error.
   virtual bool write(const WriterWriteCtx& ctx) = 0;

   // Write format footer (if any), close file(s), and optionally remove
   // them (bEmpty = iTotalSpectraSearched == 0).
   //
   // Contract: must be safe to call even if open() was never called, or returned
   // false partway through. Pipeline::run() calls close(false, false) on every
   // writer in the vector -- including ones after the one whose open() failed --
   // when any writer's open() fails, so implementations must null-check their file
   // handles before touching them.
   virtual void close(bool bSucceeded, bool bEmpty) = 0;

protected:
   // Shared output-filename builder used by all format writers.
   static void BuildNames(const WriterOpenCtx& ctx,
                          const char* ext,
                          const char* extDecoy,
                          std::string& sTarget,
                          std::string& sDecoy,
                          const char* extTargetCrux = nullptr)
   {
      std::string base = std::string(ctx.szBaseName) + ctx.szOutputSuffix;
      std::string range;
      if (!ctx.bEntireFile)
         range = "." + std::to_string(ctx.iFirstScan) + "-" + std::to_string(ctx.iLastScan);
#ifdef CRUX
      if (ctx.iDecoySearch == 2)
         { sTarget = base + range + (extTargetCrux ? extTargetCrux : ext); sDecoy = base + range + extDecoy; }
      else
         sTarget = base + range + ext;
#else
      (void)extTargetCrux;
      sTarget = base + range + ext;
      if (ctx.iDecoySearch == 2) sDecoy = base + range + extDecoy;
#endif
   }
};

#endif // _IRESULTWRITER_H_
