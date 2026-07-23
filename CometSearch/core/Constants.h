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

#ifndef _COMETCONSTANTS_H_
#define _COMETCONSTANTS_H_

#define PROTON_MASS                 1.00727646688
#define C13_DIFF                    1.00335483

#define FLOAT_ZERO                  1e-6     // 0.000001

#define MIN_PEPTIDE_LEN             1        // min # of AA for a petpide
#define MAX_PEPTIDE_LEN             51       // max # of AA for a peptide; one more than actual # to account for terminating char
#define MAX_PEPTIDE_LEN_P2          53       // max # of AA for a peptide plus 2 for N/C-term

#define FRAGINDEX_MIN_IONS_SCORE    3        // min # of matched ions for peptide to register for E-value xcorr histogram
#define FRAGINDEX_MIN_IONS_REPORT   3        // min # of matched ions for peptide to be reported
#define FRAGINDEX_MIN_MASS          200.0    // minimum fragment ion mass used to generate fragment index
#define FRAGINDEX_MAX_MASS          2000.0   // maximum fragment ion mass used to generate fragment index
#define FRAGINDEX_MAX_BATCHSIZE     1000     // maximum number of spectra loaded when querying fragment index; legacy (non-fused) batch path only -- see FUSED_FLUSH_PER_THREAD

// Fused FI_DB/PI_DB batch path (CometPreprocess::FusedLoadAndSearchSpectra): number of
// fully-searched-and-post-analysed spectra to accumulate before returning to
// Pipeline::run() for a write+free cycle. Decoupled from FRAGINDEX_MAX_BATCHSIZE/
// spectrum_batch_size (which govern the legacy load-all-then-search-all path) --
// the fused path already bounds spectra-read-but-not-searched via its own
// BoundedSpectrumQueue (depth = 4 * iNumThreads).
//
// Scaled by thread count (actual threshold = max(FUSED_FLUSH_MIN_BATCH_SIZE,
// iNumThreads * FUSED_FLUSH_PER_THREAD), computed in FusedLoadAndSearchSpectra) --
// a flat constant was tried first and found to under-flush on high-thread-count
// machines: each flush round requires every worker thread to fully drain before
// the next round's jobs are dispatched (and, for readahead, its background
// reader thread torn down and recreated), so the *per-thread* share of a round
// is what needs to stay above the amortization floor, not the round's total
// size.
#define FUSED_FLUSH_PER_THREAD      50
#define FUSED_FLUSH_MIN_BATCH_SIZE  5000
#define FRAGINDEX_MAX_NUMPEAKS      150      // number of spectrum peaks used to query fragment index
#define FRAGINDEX_MAX_NUMSCORED     100      // for each fragment index spectrum query, score up to this many peptides
#define FRAGINDEX_MAX_COMBINATIONS  10000    // raised from 2000; see docs/20260714_modifications.md
#define FRAGINDEX_MAX_MODS_PER_MOD  5
#define FRAGINDEX_KEEP_ALL_PEPTIDES 1        // 1 = consider up to FRAGINDEX_MAX_COMBINATIONS of peptides; 0 = ignore all mods for peptide that exceed FRAGINDEX_MAX_COMBINATIONS

#define MS1_MIN_MASS                0.0      // only parse up to this mass in MS1 scans for MS1 library searches
#define MS1_MAX_MASS                3000.0   // only parse up to this mass in MS1 scans for MS1 library searches
#define MS1_RT_HISTORY_SIZE         250      // size of MS1 RT history kept for recent history linear regression
#define MS1_RT_OUTLIER_THRESHOLD    2.0      // # stdev outlier threshold for MS1 RT history

#define MAX_PEFFMOD_LEN             16
#define SIZE_MASS                   128      // ascii value size
#define SIZE_NATIVEID               256      // max length of nativeID string
#define NUM_SP_IONS                 1000     // num ions for preliminary scoring
#define NUM_ION_SERIES              7        // a,b,c,x,y,z,z1
#define EXPECT_DECOY_SIZE           3000     // number of decoy entries in CometDecoys.h

#define WIDTH_REFERENCE             256      // length of the protein accession field to store
#define MAX_PROTEINS                50       // maximum number of proteins to return for each query; for index search only right now

#define HISTO_SIZE                  152      // some number greater than 150

#define NO_PEFF_VARIANT             -127

#define ASCORE_CUTOFF_TO_ACCEPT     13.0     // minimum AScore value to accept localization

#define FRAGINDEX_VMODS             5        // only parse first five variable mods for fragment ion index searches
                                             // if this is ever larger than 16, need to extend range of siVarModProteinFilter

#define VMODS                       15       // also "VMODS+1" is 4th dimension of uiBinnedIonMasses to cover unmodified ions (0), mod NL (1-15)
#define COMPOUNDMODS_OFFSET         100      // piVarModSites values >= 100 encode compound mods; index = value - 100
#define VMOD_1_INDEX                0
#define VMOD_2_INDEX                1
#define VMOD_3_INDEX                2
#define VMOD_4_INDEX                3
#define VMOD_5_INDEX                4
#define VMOD_6_INDEX                5
#define VMOD_7_INDEX                6
#define VMOD_8_INDEX                7
#define VMOD_9_INDEX                8
#define VMOD_10_INDEX               9
#define VMOD_11_INDEX               10
#define VMOD_12_INDEX               11
#define VMOD_13_INDEX               12
#define VMOD_14_INDEX               13
#define VMOD_15_INDEX               14

#define ENZYME_SINGLE_TERMINI       1
#define ENZYME_DOUBLE_TERMINI       2
#define ENZYME_N_TERMINI            8
#define ENZYME_C_TERMINI            9

#define ION_SERIES_A                0
#define ION_SERIES_B                1
#define ION_SERIES_C                2
#define ION_SERIES_X                3
#define ION_SERIES_Y                4
#define ION_SERIES_Z                5
#define ION_SERIES_Z1               6  //z+1

#ifdef CRUX
#define XCORR_CUTOFF                -999.0
#else
#define XCORR_CUTOFF                1E-8   // some near-zero cutoff
#endif

#define SPECLIB_CUTOFF              -999.9

// Identifies which type of database is being searched.
enum class DbType
{
   FASTA_DB = 0,  // normal FASTA sequence database
   FI_DB = 1,     // fragment ion index (.idx)
   PI_DB = 2      // peptide index (.idx)
};

#endif // _COMETCONSTANTS_H_
