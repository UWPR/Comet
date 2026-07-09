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

// Per-spectrum and index runtime data structs: Results, Query, QueryMS1, etc.
// Depends on: core/Constants.h, core/Params.h, CometData.h, Threading.h, AScore headers

#ifndef _COMETTYPES_H_
#define _COMETTYPES_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "core/Constants.h"
#include "core/Params.h"
#include "Threading.h"
#include "AScoreOptions.h"
#include "AScoreCentroid.h"
#include "AScoreAPI.h"
#include "AScoreFactory.h"
#include "AScoreDllInterface.h"

using std::string;
using std::vector;
using std::map;

class CometSearchManager;

struct Results
{
   double dPepMass;
   double dExpect;
   float  fScoreSp;
   float  fXcorr;
   float  fDeltaCn;
   float  fLastDeltaCn;
   float  fAScorePro;                         // AScorePro score
   unsigned short    usiRankXcorr;
   unsigned short    usiLenPeptide;
   unsigned short    usiRankSp;
   unsigned short    usiMatchedIons;
   unsigned short    usiTotalIons;  
   comet_fileoffset_t   lProteinFilePosition; // for indexdb, this is the entry in g_pvProteinsList
   long   lWhichProtein;                      // which entry in g_pvProteinsList[] contains the matched proteins
   int    piVarModSites[MAX_PEPTIDE_LEN_P2];  // store variable mods encoding, +2 to accomodate N/C-term
   double pdVarModSites[MAX_PEPTIDE_LEN_P2];  // store variable mods mass diffs, +2 to accomodate N/C-term
   char   pszMod[MAX_PEPTIDE_LEN][MAX_PEFFMOD_LEN];    // store PEFF mod string
   char   szPeptide[MAX_PEPTIDE_LEN];
   char   cPrevAA;                            // stores prev flanking AA
   char   cNextAA;                            // stores following flanking AA
   bool   bClippedM;                          // true if new N-term protein due to clipped methionine
   char   cHasVariableMod;                    // HasVariableModType enum: 0 = no variable mod, 1 = has variable mod, 2 = has AScorePro mod
   string sPeffOrigResidues;                  // original residue(s) of a PEFF variant
   string sAScoreProSiteScores;               // AScorePro site scores as comma-separated string
   int    iPeffOrigResiduePosition;           // position of PEFF variant substitution; -1 = n-term, iLenPeptide = c-term; -9=unused
   int    iPeffNewResidueCount;               // more than 0 new residues is a substitution (if iPeffOrigResidueCount=1) or insertion (if iPeffOrigResidueCount>1)
   vector<struct ProteinEntryStruct> pWhichProtein;       // file positions of matched protein entries
   vector<struct ProteinEntryStruct> pWhichDecoyProtein;  // keep separate decoy list (used for separate decoy matches and combined results)
};

struct SpecLibResults // MS2 spec lib
{
   unsigned int iWhichSpecLib;                // the matched spectral library entry
   float fSpecLibScore;
   float fXcorr;                              // use xcorr for now
   float fCn;                                 // speclib score
   float fRTtime;                             // retention time in seconds of the matched entry
};

struct SpecLibResultsMS1  // MS1 spec lib
{
   unsigned int iWhichSpecLib;                // the matched spectral library entry
   float fDotProduct;                         // unit vector dot product aka cosine similarity
   float fRTime;                              // retention time in seconds of the matched entry
};

struct PepMassInfo
{
   double dCalcPepMass;
   double dExpPepMass;                        // protonated MH+ experimental mass
   double dPeptideMassToleranceLow;           // mass tolerance low in amu from experimental mass
   double dPeptideMassToleranceHigh;          // mass tolerance high in amu from experimental mass
   double dPeptideMassToleranceMinus;         // low end of mass tolerance range including isotope offsets
   double dPeptideMassTolerancePlus;          // high end of mass tolerance range including isotope offsets
};

struct SpectrumInfoInternal
{
   int    iArraySize;         // m/z versus intensity array
   int    iHighestIon;
   int    iScanNumber;
   unsigned short    usiChargeState;
   unsigned short    usiMaxFragCharge;
   double dTotalIntensity;
   float  fRTime;
   char   szMango[32];                // Mango encoding
   char   szNativeID[SIZE_NATIVEID];  // nativeID string from mzML
};

// PreprocessStruct stores information used in preprocessing
// each spectrum.  Information not kept around otherwise
struct PreprocessStruct
{
   int    iHighestIon;
   double dHighestIntensity;
};

struct OBOStruct           // stores info read from OBO file
{
   double dMassDiffAvg;    // this is looked up from strMod string from OBO
   double dMassDiffMono;
   string strMod;          // mod string, PSI-MOD, Unimod or custom

   bool operator<(const OBOStruct& a) const
   {
      return (strMod < a.strMod);
   }
};

struct ProteinEntryStruct
{
   comet_fileoffset_t   lWhichProtein;     // file pointer to protein
   int    iStartResidue;      // start residue position in protein (1-based)
   char   cPrevAA;
   char   cNextAA;

   bool operator<(const ProteinEntryStruct& a) const
   {
      return (lWhichProtein < a.lWhichProtein);
   }
};

struct PeffModStruct       // stores info read from PEFF header
{
   double dMassDiffAvg;    // this is looked up from strMod string from OBO
   double dMassDiffMono;
   int    iPosition;       // position of modification
   char   szMod[MAX_PEFFMOD_LEN];

   bool operator<(const PeffModStruct& a) const
   {
      return (iPosition < a.iPosition);
   }
};

struct PeffVariantSimpleStruct  // stores info read from PEFF header
{
   int    iPosition;       // position of variant
   char   cResidue;        // new variant

   bool operator<(const PeffVariantSimpleStruct& a) const
   {
      return (iPosition < a.iPosition);
   }
};

struct PeffVariantComplexStruct  // stores info read from PEFF header
{
  int    iPositionA;       // start position of variant
  int    iPositionB;       // end position of variant
  string sResidues;        // if !empty(), insertion replacing aa from pos A to B;
                           // if empty(), deletion of aa from pos A to B

  bool operator<(const PeffVariantComplexStruct& a) const
  {
    return (iPositionA < a.iPositionA);
  }
};

struct PeffProcessedStruct
{
   int iBeginResidue;
   int iEndResidue;
};

struct PeffPositionStruct  // collate PEFF mods by position in sequence
{
   int iPosition;  // position within the sequence
   vector<int>    vectorWhichPeff;  // which specific peff entry from PeffModStruct
   vector<double> vectorMassDiffAvg;
   vector<double> vectorMassDiffMono;
};

struct PeffSearchStruct  // variant info passed to SearchForPeptides
{
   int    iPosition;
   bool   bBeginCleavage;
   bool   bEndCleavage;
   char   cOrigResidue;
};

//-->MH
typedef struct sDBEntry
{
   string strName;           // might be able to delete this here
   string strSeq;
   comet_fileoffset_t lProteinFilePosition;
   vector<PeffModStruct> vectorPeffMod;
   vector<PeffVariantSimpleStruct> vectorPeffVariantSimple;
   vector<PeffVariantComplexStruct> vectorPeffVariantComplex;
   vector<PeffProcessedStruct> vectorPeffProcessed;
} sDBEntry;

struct DBIndex
{
   vector<char>          pcVarModSites;                         // empty = unmodified; else [iLen+2] encoding var mods
   comet_fileoffset_t    lIndexProteinFilePosition;             // points to entry in g_pvProteinsList
   double                dPepMass;                              // MH+ pep mass
   unsigned short        siVarModProteinFilter = 0;             // bitwise representation of mmapProtein
   char                  cPrevAA;
   char                  cNextAA;
   char                  sPeptide[MAX_PEPTIDE_LEN];             // peptide sequence, null-terminated

   bool operator==(const DBIndex& rhs) const
   {
      if (strcmp(sPeptide, rhs.sPeptide) != 0)
         return false;

      if (fabs(dPepMass - rhs.dPepMass) > FLOAT_ZERO)
         return false;

      int iLen = (int)strlen(sPeptide) + 2;
      for (int i = 0; i < iLen; ++i)
      {
         char l = pcVarModSites.empty()     ? 0 : pcVarModSites[i];
         char r = rhs.pcVarModSites.empty() ? 0 : rhs.pcVarModSites[i];
         if (l != r)
            return false;
      }

      return true;
   }

   bool operator<(const DBIndex& rhs) const
   {
      int cmp = strcmp(sPeptide, rhs.sPeptide);
      if (cmp != 0)
         return cmp < 0;

      if (fabs(dPepMass - rhs.dPepMass) > FLOAT_ZERO)
         return dPepMass < rhs.dPepMass;

      int iLen = (int)strlen(sPeptide) + 2;
      for (int i = 0; i < iLen; ++i)
      {
         char l = pcVarModSites.empty()     ? 0 : pcVarModSites[i];
         char r = rhs.pcVarModSites.empty() ? 0 : rhs.pcVarModSites[i];
         if (l != r)
            return l < r;
      }

      // FINAL tie-breaker: lowest protein index first in order
      // to grab flanking residues from the first protein
      return lIndexProteinFilePosition < rhs.lIndexProteinFilePosition;
   }
};

// Compact fixed-size tuple used during plain-peptide index generation.
// Replaces heap-heavy DBIndex entries during the per-thread collection phase.
struct PepGenTuple
{
   char     sPeptide[MAX_PEPTIDE_LEN];   // original AA letters (or L->I canonical), null-terminated
   double   dPepMass;                    // MH+ mass
   comet_fileoffset_t lProteinFileOffset;// FASTA byte offset of the source protein
   uint16_t siVarModProteinFilter;
   char     cPrevAA;
   char     cNextAA;
};

// ---------------------------------------------------------------------------
// 5-bit amino acid encoding for per-length short-peptide key packing.
// The 20 standard AAs are mapped in ASCII sort order (A=1, C=2, ..., Y=20)
// so that sorting packed uint64 keys is equivalent to lexicographic sort of
// sequences within a given peptide length. Non-standard/ambiguous one-letter
// codes (B, J, O, U, X, Z) get their own distinct codes 21-26, appended after
// the standard set rather than interleaved in ASCII order -- ordering is only
// used to group identical sequences together before dedup (final index order
// is by mass, not by this key), so exact lexicographic placement of these six
// doesn't matter. Code 0 is reserved as the "unset/pad" value read for slots
// beyond a short peptide's length; every mapped residue below MUST be nonzero,
// or UnpackPeptide() will decode it as the pad sentinel ('\0') and silently
// truncate the reconstructed string at that position (this happened for 'U',
// see docs/20260709_sprankjitter.md).
// ---------------------------------------------------------------------------
static constexpr uint8_t kAA5bit[256] = {
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   //   0-15
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   //  16-31
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   //  32-47
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   //  48-63
   0,                                   //  64 '@'
   1,                                   //  65 'A'
  21,                                   //  66 'B'
   2,                                   //  67 'C'
   3,                                   //  68 'D'
   4,                                   //  69 'E'
   5,                                   //  70 'F'
   6,                                   //  71 'G'
   7,                                   //  72 'H'
   8,                                   //  73 'I'  (canonical for I/L when bTreatSameIL)
  22,                                   //  74 'J'
   9,                                   //  75 'K'
  10,                                   //  76 'L'  (remapped to 8 when bTreatSameIL)
  11,                                   //  77 'M'
  12,                                   //  78 'N'
  23,                                   //  79 'O'
  13,                                   //  80 'P'
  14,                                   //  81 'Q'
  15,                                   //  82 'R'
  16,                                   //  83 'S'
  17,                                   //  84 'T'
  24,                                   //  85 'U'
  18,                                   //  86 'V'
  19,                                   //  87 'W'
  25,                                   //  88 'X'
  20,                                   //  89 'Y'
  26,                                   //  90 'Z'
   // 91-255: all zeros
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0
};

// Reverse map: 5-bit code -> amino acid character.
// Code 8 always decodes to 'I' (canonical; L maps to code 8 when bTreatSameIL).
// Codes 21-26 decode the non-standard/ambiguous residues B/J/O/U/X/Z.
static constexpr char k5bitAA[32] = {
   '\0','A','C','D','E','F','G','H','I','K','L','M','N','P','Q','R',
   'S', 'T','V','W','Y','B','J','O','U','X','Z','\0','\0','\0','\0','\0'
};

// Pack up to 12 amino acids into a uint64 key (5 bits each, 60 bits total).
// When bTreatSameIL is true, L encodes identically to I.
inline uint64_t PackPeptide(const char* seq, int iLen, bool bTreatSameIL)
{
   uint64_t key = 0;
   for (int i = 0; i < iLen; ++i)
   {
      char c = seq[i];
      if (bTreatSameIL && c == 'L') c = 'I';
      key |= ((uint64_t)kAA5bit[(unsigned char)c] << (55 - i * 5));
   }
   return key;
}

// Decode a packed key back to a null-terminated sequence of iLen characters.
inline void UnpackPeptide(uint64_t key, int iLen, char* seq)
{
   for (int i = 0; i < iLen; ++i)
      seq[i] = k5bitAA[(key >> (55 - i * 5)) & 0x1F];
   seq[iLen] = '\0';
}

// Compact per-thread tuple for short peptides (len <= 12) during index generation.
// 32 bytes on 64-bit (8-byte alignment); uILMask occupies 2 of the 4 trailing pad bytes.
struct PepGenTupleShort
{
   uint64_t           uPackedPep;            // canonical 5-bit-encoded sequence (L treated as I when bTreatSameIL)
   double             dPepMass;
   comet_fileoffset_t lProteinFileOffset;
   uint16_t           siVarModProteinFilter;
   char               cPrevAA;
   char               cNextAA;
   uint16_t           uILMask;              // bitmask: bit k = 1 means position k was 'L' in FASTA original
};

// This is used for fragment indexing; plain peptides are stored in index
// file and read in to this data struct.  Same as DBIndex w/o pcVarModSites[]
struct PlainPeptideIndexStruct
{
   comet_fileoffset_t   lIndexProteinFilePosition;  // points to entry in g_pvProteinsList
   double               dPepMass;                   // MH+ pep mass, unmodified mass; modified mass in FragmentPeptidesStruct
   unsigned short       siVarModProteinFilter;      // bitwise representation of mmapProtein
   char                 cPrevAA;
   char                 cNextAA;
   char                 szPeptide[MAX_PEPTIDE_LEN]; // peptide sequence, null-terminated

   bool operator==(const PlainPeptideIndexStruct &rhs) const
   {
      return strcmp(szPeptide, rhs.szPeptide) == 0;
   }
};

struct FragmentPeptidesStruct
{
   size_t iWhichPeptide;   // reference to raw peptide (sequence, proteins, etc.) in PlainPeptideIndexStruct
   int modNumIdx;
   double dPepMass;     // peptide mass (modified or unmodified) after permuting mods
   char cNtermMod;
   char cCtermMod;

   bool operator<(const FragmentPeptidesStruct& a) const
   {
      return dPepMass < a.dPepMass;
   }
};

struct SpecLibStruct
{
   string strName;                   // any string associated with speclib entry
   unsigned int iLibEntry;           // a reference number associated with speclib entry
   unsigned int iNumPeaks;
   int iSpecLibCharge;               // precursor charge; not relevant for MS1 speclib
   double dSpecLibMW;                // if a peptide, store neutral mass
   float fRTime;
   float fScaleMinInten;             // min intensity of data prior to encoding to pccSparseFastXcorrData; 0.0 for unit vector
   float fScaleMaxInten;             // max intensity of data prior to encoding to ppcSparseFastXcorrData
   vector<std::pair<double, float>> vSpecLibPeaks;
   float* pfUnitVector;
   unsigned int uiArraySizeMS1;
};

// for MS1 alignment
struct RetentionMatch
{
   double dQueryTime;
   double dReferenceTime;
   int iSpectrumIndex;

   RetentionMatch(double dQueryTime, double dReferenceTime, int iSpectrumIndex);
};
extern std::deque<RetentionMatch> RetentionMatchHistory;

extern unsigned int* g_iFragmentIndex;            // CSR flat data: all posting lists concatenated [g_iFragmentIndexOffset[bin]..g_iFragmentIndexOffset[bin+1])
extern uint64_t*     g_iFragmentIndexOffset;      // CSR offsets [uiMaxFragmentArrayIndex+1]: cumulative entry counts, can exceed UINT_MAX for large non-enzymatic searches
extern vector<struct FragmentPeptidesStruct> g_vFragmentPeptides;
extern vector<PlainPeptideIndexStruct> g_vRawPeptides;
extern bool* g_bIndexPrecursors;     // allocate an array of BIN(max_precursor, protonated) and use a bool to indicate if that precursor is present in input file(s)
extern vector<SpecLibStruct> g_vSpecLib;
extern vector<vector<unsigned int>> g_vulSpecLibPrecursorIndex;  // this will be an vector of vectors<unsigned int>

struct IndexProteinStruct  // for indexed database
{
   char szProt[WIDTH_REFERENCE];
   comet_fileoffset_t lProteinFilePosition;
   int  iWhichProtein;
};

// Flat CSR-style storage for the per-peptide protein list.
// Replaces vector<vector<comet_fileoffset_t>> to eliminate the ~190M
// individual heap allocations (one per inner vector) that caused a
// ~6-minute free-time tail when building an MHC .idx file.
// External interface mirrors vector<vector<comet_fileoffset_t>> so
// existing call sites need no changes.
class ProteinsListCSR
{
public:
   // Read-only proxy for a single row (one peptide's protein offsets).
   struct Row
   {
      const comet_fileoffset_t* ptr;
      size_t                    n;

      size_t size()  const { return n; }
      bool   empty() const { return n == 0; }

      const comet_fileoffset_t& operator[](size_t j) const { return ptr[j]; }
      comet_fileoffset_t        at(size_t j)          const { return ptr[j]; }

      const comet_fileoffset_t* begin() const { return ptr; }
      const comet_fileoffset_t* end()   const { return ptr + n; }
   };

   // Size / state
   size_t size()  const { return m_off.empty() ? 0 : m_off.size() - 1; }
   bool   empty() const { return size() == 0; }

   // Modifiers
   void clear()
   {
      vector<comet_fileoffset_t>().swap(m_flat);
      vector<uint64_t>().swap(m_off);
   }

   void reserve(size_t n) { m_off.reserve(n + 1); }

   void push_back(const vector<comet_fileoffset_t>& v)
   {
      if (m_off.empty()) m_off.push_back(0);
      m_flat.insert(m_flat.end(), v.begin(), v.end());
      m_off.push_back(m_flat.size());
   }

   void push_back(vector<comet_fileoffset_t>&& v)
   {
      if (m_off.empty()) m_off.push_back(0);
      m_flat.insert(m_flat.end(), v.begin(), v.end());
      m_off.push_back(m_flat.size());
      vector<comet_fileoffset_t>().swap(v);  // release source buffer immediately
   }

   // Batch-append from pre-built flat storage.
   // flat: all protein file offsets for this block, concatenated in row order
   // cnt:  number of offsets per row (max value bounded by iMaxDuplicateProteins)
   // Bulk-copies both arrays into m_flat/m_off with two insert() calls, then
   // releases the source buffers.  Replaces N individual push_back(vector&&)
   // calls, each of which required one heap free() -- this reduces N free()s
   // to 2 (one for flat, one for cnt) regardless of how many rows are in the block.
   void append_flat(vector<comet_fileoffset_t>& flat, vector<uint32_t>& cnt)
   {
      if (flat.empty())
         return;
      if (m_off.empty())
         m_off.push_back(0);
      m_flat.insert(m_flat.end(), flat.begin(), flat.end());
      for (uint32_t n : cnt)
         m_off.push_back(m_off.back() + n);
      vector<comet_fileoffset_t>().swap(flat);
      vector<uint32_t>().swap(cnt);
   }

   // Element access
   Row operator[](size_t i) const
   {
      return {m_flat.data() + m_off[i],
              static_cast<size_t>(m_off[i + 1] - m_off[i])};
   }

   Row at(size_t i) const { return (*this)[i]; }

   // Range-based for -- yields Row values
   struct Iterator
   {
      const ProteinsListCSR* self;
      size_t                 i;

      Row       operator*()  const { return (*self)[i]; }
      Iterator& operator++()       { ++i; return *this; }
      bool      operator!=(const Iterator& o) const { return i != o.i; }
   };

   Iterator begin() const { return {this, 0}; }
   Iterator end()   const { return {this, size()}; }

private:
   vector<comet_fileoffset_t> m_flat;   // all protein offsets concatenated
   vector<uint64_t>           m_off;    // [N+1] CSR offsets; row i spans [m_off[i], m_off[i+1])
};

extern ProteinsListCSR g_pvProteinsList;
extern std::unordered_map<comet_fileoffset_t, string> g_pvProteinNameCache;  // file offset -> protein name string; populated at index load

extern AScoreProCpp::AScoreOptions g_AScoreOptions;  // AScore options
extern AScoreProCpp::AScoreDllInterface* g_AScoreInterface;

struct ModificationNumber
{
//   int modificationNumber;
   int modStringLen;             // FIX: need to confirm if not needed  (MOD_SEQS.at(modSeqIdx)).size();
   char* modifications;
};

extern vector<ModificationNumber> MOD_NUMBERS;
extern vector<string> MOD_SEQS;    // Unique modifiable sequences.
extern int* MOD_SEQ_MOD_NUM_START; // Start index in the MOD_NUMBERS vector for a modifiable sequence; -1 if no modification numbers were generated
extern int* MOD_SEQ_MOD_NUM_CNT;   // Total modifications numbers for a modifiable sequence.

// Index into the MOD_SEQS vector
// -1 for peptides that have no modifiable amino acids
// -2 for peptides with no modifiable amino acids but contain n/c-term mods
extern int* PEPTIDE_MOD_SEQ_IDXS;

extern int MOD_NUM;
extern bool g_bPlainPeptideIndexRead;   // set to true if plain peptide index file is read (and fragment index generated)
                                        // poor choice of name for the fragment index .idx given peptide index is back
extern  std::atomic<bool>  g_bPeptideIndexRead;        // set to true if peptide index file is read
extern bool g_bSpecLibRead;             // set to true if spectral library file is read

// g_bPerformSpecLibSearch, g_bPerformDatabaseSearch, g_bIdxNoFasta moved to SearchSession
// (Phase 4: batch path only -- see search/SearchSession.h)

extern bool g_bCometPreprocessMemoryAllocated;    // set to true when memory has been allocated
extern bool g_bCometSearchMemoryAllocated;        // set to true when memory has been allocated

// Query stores information for peptide scoring and results
// This struct is allocated for each spectrum/charge combination
struct Query
{
   int   iXcorrHistogram[HISTO_SIZE];
   unsigned int   uiHistogramCount;   // # of entries in histogram
   float fPar[4];           // parameters of LMA regression

   int iMatchPeptideCount;        // # of peptides that get stored (i.e. are greater than lowest score)
   int iDecoyMatchPeptideCount;   // # of decoy peptides that get stored (i.e. are greater than lowest score)

   short siMaxXcorr;        // index of maximum correlation score in iXcorrHistogram

   short siLowestXcorrScoreIndex;
   short siLowestDecoyXcorrScoreIndex;

   double dLowestXcorrScore;
   double dLowestDecoyXcorrScore;

   float fLowestSpecLibScore;

   int iMinXcorrHisto;    // min xcorr score for xcorr histogram to address good E-values for poor/sparse spectra

   double dMangoIndex;      // scan number decimal precursor value i.e. 2401.001 for scan 2401, first precursor/z pair

   unsigned long int  _uliNumMatchedPeptides;  // # of peptides that get scored
   unsigned long int  _uliNumMatchedDecoyPeptides;

   // When true, sparse child arrays (float[SPARSE_MATRIX_SIZE]) belong to the
   // thread-local RtsScratch pool and must NOT be delete[]'d by the destructor.
   // Set only by PreprocessSingleSpectrumThreadLocal via PreprocessSingleSpectrumCore.
   bool bSparseFromPool;

   // Sparse matrix representation of data
   int iSpScoreData;    //size of sparse matrix
   int iFastXcorrDataSize;
   float **ppfSparseSpScoreData;
   float **ppfSparseFastXcorrData;
   float **ppfSparseFastXcorrDataNL;  // ppfSparseFastXcorrData with NH3, H2O contributions

   // Store raw peaks for AScorePro

   // List of ms/ms masses for fragment index search; intensity not important at this stage
   vector<float> vfRawFragmentPeakMass;
   // Consider replacing vfRawFragmentPeakMass with a vector<pair<double, double>> to store
   // both mass and intensity if AScorePro is used
   vector<AScoreProCpp::Centroid> vRawFragmentPeakMassIntensity;


   PepMassInfo          _pepMassInfo;
   SpectrumInfoInternal _spectrumInfoInternal;
   Results*             _pResults;
   Results*             _pDecoys;
   SpecLibResults*      _pSpecLibResults;

   std::chrono::high_resolution_clock::time_point tSearchStart;  // per-query search start time for iMaxIndexRunTime timeout

   Mutex accessMutex;

   Query()
   {
      memset(iXcorrHistogram, 0, sizeof(iXcorrHistogram));

      iMatchPeptideCount = 0;
      iDecoyMatchPeptideCount = 0;
      uiHistogramCount = 0;
      iMinXcorrHisto = 0;

      fPar[0]=0.0;
      fPar[1]=0.0;
      fPar[2]=0.0;
      fPar[3]=0.0;

      siMaxXcorr = 0;                        // index of maximum correlation score in iXcorrHistogram
      siLowestXcorrScoreIndex = 0;
      siLowestDecoyXcorrScoreIndex = 0;

      dLowestXcorrScore = XCORR_CUTOFF;
      dLowestDecoyXcorrScore = XCORR_CUTOFF;

      fLowestSpecLibScore = SPECLIB_CUTOFF;

      dMangoIndex = 0.0;

      _uliNumMatchedPeptides = 0;
      _uliNumMatchedDecoyPeptides = 0;

      bSparseFromPool = false;

      // Set by CometPreprocess::Preprocess (or its long/MS1-path siblings)
      // once the spectrum's array size is known; must start at 0 here so the
      // destructor's delete loops below are no-ops if a Query is destroyed
      // before preprocessing ever runs (early-exit/error paths) -- otherwise
      // they read these as garbage and walk off the end of a NULL array.
      iSpScoreData = 0;
      iFastXcorrDataSize = 0;

      ppfSparseSpScoreData = NULL;
      ppfSparseFastXcorrData = NULL;
      ppfSparseFastXcorrDataNL = NULL;          // ppfSparseFastXcorrData with NH3, H2O contributions

      vfRawFragmentPeakMass.clear();
      vRawFragmentPeakMassIntensity.clear();

      _pepMassInfo.dCalcPepMass = 0.0;
      _pepMassInfo.dExpPepMass = 0.0;
      _pepMassInfo.dPeptideMassToleranceLow = 0.0;
      _pepMassInfo.dPeptideMassToleranceHigh = 0.0;
      _pepMassInfo.dPeptideMassToleranceMinus = 0.0;
      _pepMassInfo.dPeptideMassTolerancePlus = 0.0;

      _spectrumInfoInternal.dTotalIntensity = 0.0;
      _spectrumInfoInternal.iArraySize = 0;
      _spectrumInfoInternal.iHighestIon = 0;
      _spectrumInfoInternal.iScanNumber = 0;
      _spectrumInfoInternal.dTotalIntensity = 0.0;

      _pResults = NULL;
      _pDecoys = NULL;
      _pSpecLibResults = NULL;

      Threading::InitMutex(&accessMutex);
   }

   ~Query()
   {
      int i;
      if (!bSparseFromPool)
      {
         for (i = 0; i < iSpScoreData; ++i)
         {
            if (ppfSparseSpScoreData[i] != NULL)
               delete[] ppfSparseSpScoreData[i];
         }
      }
      delete[] ppfSparseSpScoreData;
      ppfSparseSpScoreData = NULL;

      if (g_staticParams.ionInformation.bUseWaterAmmoniaLoss
            && (g_staticParams.ionInformation.iIonVal[ION_SERIES_A]
               || g_staticParams.ionInformation.iIonVal[ION_SERIES_B]
               || g_staticParams.ionInformation.iIonVal[ION_SERIES_Y]))
      {
         if (!bSparseFromPool)
         {
            for (i = 0; i < iFastXcorrDataSize; ++i)
            {
               if (ppfSparseFastXcorrData[i] != NULL)
                  delete[] ppfSparseFastXcorrData[i];
               if (ppfSparseFastXcorrDataNL[i]!=NULL)
                  delete[] ppfSparseFastXcorrDataNL[i];
            }
         }
         delete[] ppfSparseFastXcorrDataNL;
         ppfSparseFastXcorrDataNL = NULL;
      }
      else
      {
         if (!bSparseFromPool)
         {
            for (i = 0; i < iFastXcorrDataSize; ++i)
            {
               if (ppfSparseFastXcorrData[i] != NULL)
                  delete[] ppfSparseFastXcorrData[i];
            }
         }
      }
      delete[] ppfSparseFastXcorrData;
      ppfSparseFastXcorrData = NULL;

      if (_pResults != NULL)
      {
         _pResults->pWhichProtein.clear();
         if (g_staticParams.options.iDecoySearch == 1)
            _pResults->pWhichDecoyProtein.clear();
         delete[] _pResults;
         _pResults = NULL;
      }

      if (g_staticParams.options.iDecoySearch == 2 && _pDecoys != NULL)
      {
         _pDecoys->pWhichDecoyProtein.clear();
         delete[] _pDecoys;
         _pDecoys = NULL;
      }

      Threading::DestroyMutex(accessMutex);
   }
};

struct QueryMS1
{
   //   short siLowestSpecLibIndex;
   //   float fLowestXcorr;
   unsigned int uiMatchMS1Count;        // # of peptides that get stored (i.e. are greater than lowest score)
   unsigned int iArraySizeMS1;          // dimension of pcFastXcorrData

   // Standard array representation of data
   // Library spectra are fast xcorr manipulated so non need to do so with query MS1
   float* pfFastXcorrData;

   SpecLibResultsMS1 _pSpecLibResultsMS1;

   Mutex accessMutex;

   QueryMS1()
   {
      //      siLowestSpecLibIndex = 0;
      //      fLowestXcorr = SPECLIB_CUTOFF;
      uiMatchMS1Count = 0;
      pfFastXcorrData = NULL;
      _pSpecLibResultsMS1.fDotProduct = 0.0;
      _pSpecLibResultsMS1.fRTime = 0.0;

      Threading::InitMutex(&accessMutex);
   }

   ~QueryMS1()
   {
      //FIX delete _pSepcLibResults

      Threading::DestroyMutex(accessMutex);
   }
};

// g_pvQuery and g_pvQueryMS1 moved to SearchSession.queries / SearchSession.ms1Queries
// (Phase 4: batch path only -- see search/SearchSession.h)
extern vector<InputFileInfo*>  g_pvInputFiles;
extern Mutex                   g_pvQueryMutex;
extern Mutex                   g_pvDBIndexMutex;
extern Mutex                   g_preprocessMemoryPoolMutex;
extern Mutex                   g_dbIndexMutex;
extern Mutex                   g_vSpecLibMutex;

extern vector<DBIndex> g_pvDBIndex;       // used in both peptide index and fragment ion index; latter to store plain peptides
// Per-length, per-thread generation buffers.  Outer index = (iLen - iMinLen) for short,
// (iLen - 13) for long.  Inner index = thread slot.
extern vector<vector<vector<PepGenTupleShort>>> g_vvvPepGenShort;  // lengths <= 12
extern vector<vector<vector<PepGenTuple>>>      g_vvvPepGenLong;   // lengths > 12
extern std::map<long long, IndexProteinStruct>  g_pvProteinNames;   // indexed database protein names and file positions

struct IonSeriesStruct         // defines which fragment ion series are considered
{
   int bPreviousMatch[8];
};


struct MatchedIonsStruct  // for SingleSpectrumSearch
{
   double dMass;
   double dInten;

   bool operator<(const MatchedIonsStruct& a) const
   {
      return dInten > a.dInten;
   }
};

#endif // _COMETTYPES_H_
