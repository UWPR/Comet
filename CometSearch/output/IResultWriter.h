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
struct Query;

// Parameters passed to each writer's open() method.
struct WriterOpenCtx
{
   const char* szBaseName;
   const char* szOutputSuffix;
   const char* szTxtFileExt;     // TxtWriter only
   bool        bEntireFile;      // true => no scan-range suffix on output name
   int         iFirstScan;
   int         iLastScan;
   int         iDecoySearch;     // 0=off, 1=concat, 2=separate
   bool        bIdxNoFasta;      // .idx DB with no companion .fasta (mzIdentML)
   CometSearchManager* pMgr;     // for format headers that need ICometSearchManager
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

   // Write all results in g_pvQuery for one batch.
   // Returns false on error.
   virtual bool write(const WriterWriteCtx& ctx) = 0;

   // Write format footer (if any), close file(s), and optionally remove
   // them (bEmpty = iTotalSpectraSearched == 0).
   virtual void close(bool bSucceeded, bool bEmpty) = 0;
};

#endif // _IRESULTWRITER_H_
