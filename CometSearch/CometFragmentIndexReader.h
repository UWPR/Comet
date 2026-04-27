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


#ifndef _COMETFRAGMENTINDEXREADER_H_
#define _COMETFRAGMENTINDEXREADER_H_

#include "Common.h"
#include "CometSearch.h"

// Read-only wrapper for fragment index globals
class FragmentIndexReader
{
public:
   FragmentIndexReader()
      : m_iFragmentIndex(g_iFragmentIndex)
      , m_iFragmentIndexOffset(g_iFragmentIndexOffset)
      , m_vFragmentPeptides(g_vFragmentPeptides)
      , m_vRawPeptides(g_vRawPeptides)
      , m_pvProteinsList(g_pvProteinsList)
   {
   }

   // Const-only access methods
   inline unsigned int GetFragmentIndexEntry(unsigned int bin, size_t idx) const
   {
      return m_iFragmentIndex[m_iFragmentIndexOffset[bin] + idx];
   }

   inline unsigned int GetFragmentCount(unsigned int bin) const
   {
      return m_iFragmentIndexOffset[bin + 1] - m_iFragmentIndexOffset[bin];
   }

   inline const FragmentPeptidesStruct& GetFragmentPeptide(size_t idx) const
   {
      return m_vFragmentPeptides[idx];
   }

   inline const PlainPeptideIndexStruct& GetRawPeptide(size_t idx) const
   {
      return m_vRawPeptides[idx];
   }

   inline const vector<comet_fileoffset_t>& GetProteinList(size_t idx) const
   {
      return m_pvProteinsList[idx];
   }

private:
   // Const pointers prevent modification
   const unsigned int* m_iFragmentIndex;
   const unsigned int* m_iFragmentIndexOffset;
   const vector<struct FragmentPeptidesStruct>& m_vFragmentPeptides;
   const vector<PlainPeptideIndexStruct>& m_vRawPeptides;
   const vector<vector<comet_fileoffset_t>>& m_pvProteinsList;

   // Delete copy/assignment to prevent misuse
   FragmentIndexReader(const FragmentIndexReader&) = delete;
   FragmentIndexReader& operator=(const FragmentIndexReader&) = delete;
};

#endif // _COMETFRAGMENTINDEXREADER_H_