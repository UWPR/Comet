/*
   Copyright 2012 University of Washington

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

#ifndef _COMETINDEXDB_H_
#define _COMETINDEXDB_H_

#include "Common.h"
#include "CometDataInternal.h"

struct IndexThreadData
{
   sDBEntry dbEntry;

   IndexThreadData()
   {
   }

   IndexThreadData(sDBEntry &dbEntry_in)
   {
      dbEntry.strName = dbEntry_in.strName;
      dbEntry.strSeq = dbEntry_in.strSeq;
      dbEntry.lProteinFilePosition = dbEntry_in.lProteinFilePosition;
   }

   ~IndexThreadData()
   {
   }
};


class CometIndexDb
{
public:

   CometIndexDb();
   ~CometIndexDb();

   static bool CreateIndex(void);
   bool DoIndex(sDBEntry dbe,
                vector<struct DBIndex> &vIndex,
                int iProtNum);
   static bool SortByPeptide(const DBIndex &lhs,
                             const DBIndex &rhs);
   static bool SortByMass(const DBIndex &lhs,
                          const DBIndex &rhs);
   static bool SortByFP(const DBIndex &lhs,
                        const DBIndex &rhs);

private:

   bool DigestPeptides(char *szProteinSeq,
                       vector<struct DBIndex> &vIndex,
                       int iProtNum);
   bool WithinDigestRange(double dCalcPepMass,
                          char *szProteinSeq,
                          int iStartPos,
                          int iEndPos);
   bool CheckEnzymeTermini(char *szProteinSeq,
                           int iStartPos,
                           int iEndPos);

   // Cleaning up
   void CleanUp();

private:

   struct IndexProteinInfo
   {
       int  iProteinSeqLength;
       int  iAllocatedProtSeqLength;
       long lProteinFilePosition;
       char szProteinName[WIDTH_REFERENCE];
       char *pszProteinSeq;
   };

   IndexProteinInfo        _proteinInfo;
};

#endif // _COMETINDEXDB_H_
