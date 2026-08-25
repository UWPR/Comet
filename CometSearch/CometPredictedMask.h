// Copyright 2012-2026 Jimmy Eng
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


#ifndef _COMETPREDICTEDMASK_H_
#define _COMETPREDICTEDMASK_H_

#include "Common.h"
#include "CometDataInternal.h"
#include "CometStatus.h"

#include <cstdint>
#include <string>
#include <vector>

// Phase 3 (docs/20260805_carafe.md Section 4.4/6.9/6.10, 9): loads and looks up a Carafe
// predicted-fragment keep/drop mask (tools/carafe_ms2_to_fi_mask.py's write_mask_file()
// output) for CometFragmentIndex::AddFragments() to gate individual b/y/modloss FI entry
// insertions against, without changing which peptide variants exist in the index at all --
// only which of a kept variant's fragment-ion positions get written.
class CometPredictedMask
{
public:
   // Loads strMaskFile. A no-op (returns true, IsEnabled() stays false) if strMaskFile is
   // empty -- masking is opt-in via the fragment_index_predicted_mask_file comet.params key.
   // Verifies the mask's magic/version string and its SourceIdxFingerprint/
   // SourceIdxNumRawPeptides header fields against the CURRENTLY-LOADED .idx
   // (g_staticParams.databaseInfo.szDatabase, which must already be a valid PI_DB the .idx was
   // built from before this is called) -- returns false (sets g_cometStatus, logs a clear
   // error) on any mismatch rather than silently applying a stale/wrong mask to the wrong
   // peptide universe. Must be called once, after g_vRawPeptides is populated (i.e. after
   // CometPeptideIndex::ReadPeptideIndex()) and before CometFragmentIndex::
   // GenerateFragmentIndex() runs -- see CometFragmentIndex::CreateFragmentIndex().
   static bool Load(const std::string& strMaskFile);

   static bool IsEnabled();

   // Returns true if variant (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod) has a mask
   // entry, filling in the four masks (bMask/yMask for unshifted b/y ions,
   // bModlossMask/yModlossMask for Phase 2b's NL-shifted ions -- see
   // CometFragmentIndex::AddFragments()). Returns false, leaving the four out-params
   // untouched, if no entry exists for this exact tuple -- callers MUST treat "not found" as
   // fully-unfiltered (docs/20260805_carafe.md Section 8 item 2: a variant Carafe couldn't
   // score, or wasn't included in this particular mask build, keeps its full fragment set),
   // never as "everything masked out".
   //
   // Bit convention (tools/carafe_ms2_to_fi_mask.py's module docstring, unchanged here): bit
   // (i-2) of the relevant mask corresponds to Comet's AddFragments() ladder index i (0-based,
   // only ever queried for i > 1 i.e. fragment length >= 3) -- bit 0 = i=2 (length 3), packed
   // from the bottom. bModlossMask/yModlossMask use the identical convention, sourced from the
   // SAME ladder index the NL-shifted insertion loop is already at (CometFragmentIndex.cpp's
   // neutral-loss block runs inside the very same `for (int i ...)` iteration as the unshifted
   // insertion, immediately above it -- no separate remap needed).
   static bool Lookup(unsigned int iWhichPeptide, int modNumIdx, signed char cNtermMod, signed char cCtermMod,
                      uint64_t& bMask, uint64_t& yMask, uint64_t& bModlossMask, uint64_t& yModlossMask);

   // Closes the gap the .idx fingerprint alone leaves open (docs/20260805_carafe.md Section
   // 6.10's closing note / Section 8 items 12-14): the .idx fingerprint proves iWhichPeptide numbering is
   // safe, but modNumIdx numbering also depends on whichever variable mods were live in
   // comet.params when the mask's tuples were generated (comet.exe -x), which the .idx itself
   // no longer records at all (Phase 0.5). Serializes the first FRAGINDEX_VMODS varModList[]
   // slots' (mass, chars, n/c-term flags, neutral loss) into one comparable string --
   // identical serialization used by comet.exe -x (CometPeptideIndex::ExportVariants(), which
   // writes it as a "# VarModConfig: <string>" comment line propagated through
   // tools/idx_to_carafe.py's variant map into tools/carafe_ms2_to_fi_mask.py's mask file
   // header) and here at mask-load time, so Load() can reject a mask built against different
   // variable mods than are live right now, rather than silently trusting stale modNumIdx keys.
   static std::string ComputeVarModConfigString();

   // Releases s_entries' backing storage once CometFragmentIndex::GenerateFragmentIndex() has
   // finished -- Lookup() is only ever called from AddFragments() during that one build pass
   // (CometFragmentIndex.cpp:854), so the mask's 39M+-entry lookup table (42 bytes/entry, e.g.
   // ~1.66GB for the phospho run in docs/20260824_carafe_phoshoresults.md) is otherwise dead
   // weight for the rest of the search. Deliberately leaves s_bEnabled untouched -- nothing
   // outside this class currently queries IsEnabled() after the FI build completes, and even if
   // something did, Lookup() against an emptied s_entries still hits the documented "not found"
   // path (lower_bound on an empty vector) and correctly falls back to fully-unfiltered rather
   // than misbehaving.
   //
   // Safety precondition (shared with Load()'s own s_bLoadAttempted one-shot guard just below):
   // this assumes CreateFragmentIndex() runs at most once per process, as already documented
   // there -- if that ever changes, a second build would find s_entries empty (Load() no-ops
   // after its first call, so it won't reload) and would silently run unmasked instead of
   // failing loudly. Revisit both guards together if a multi-build-per-process caller appears.
   static void FreeAfterIndexBuild();

private:
   // Byte-for-byte the on-disk layout too (tools/carafe_ms2_to_fi_mask.py's ENTRY_FMT =
   // "<IibbQQQQ", little-endian, unpadded): iWhichPeptide(u32) modNumIdx(i32) cNtermMod(i8)
   // cCtermMod(i8) bMask(u64) yMask(u64) bModlossMask(u64) yModlossMask(u64). Packed
   // (no alignment padding) so Load() can fread() the file directly into s_entries with no
   // separate staging buffer/conversion pass -- this used to be two types (a padded 48-byte
   // "Entry" for lookup plus an unpadded 42-byte "PackedEntry" for I/O, copied field-by-field
   // right after loading) purely so Entry's uint64_t members would land on natural alignment;
   // merged into one packed type instead, since the transient staging copy cost ~1.66GB extra
   // (a second full-size buffer, momentarily resident alongside this one) for no benefit --
   // Lookup()/EntryKeyLess() only run during the one-time FI-build pass, so the minor unaligned-
   // load cost of reading uint64_t fields at non-8-byte-aligned offsets here is immaterial next
   // to the memory saved, and every target this builds for (x86/x64/ARM64, all little-endian in
   // practice per the byte-order note above) handles unaligned loads correctly.
#pragma pack(push, 1)
   struct Entry
   {
      unsigned int iWhichPeptide;
      int modNumIdx;
      signed char cNtermMod;
      signed char cCtermMod;
      uint64_t bMask;
      uint64_t yMask;
      uint64_t bModlossMask;
      uint64_t yModlossMask;
   };
#pragma pack(pop)

   static_assert(sizeof(Entry) == 4 + 4 + 1 + 1 + 8 + 8 + 8 + 8,
      "Entry must match tools/carafe_ms2_to_fi_mask.py's ENTRY_FMT exactly (42 bytes, unpadded)");

   static std::vector<Entry> s_entries;   // kept sorted by (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod)
   static bool s_bEnabled;

   // (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod) tuple ordering, shared by the sort in
   // Load() and the binary search in Lookup(). A private static member (not a free function in
   // an anonymous namespace in the .cpp) because Entry itself is private -- a free function
   // outside the class can't name a private nested type even from the same translation unit.
   static bool EntryKeyLess(const Entry& a, const Entry& b);

   // CRC-32 (zlib's crc32(), matching tools/carafe_ms2_to_fi_mask.py's idx_fingerprint() --
   // see that function's docstring for why CRC-32 rather than a cryptographic hash: this is a
   // "did I point the mask at the wrong .idx" sanity check, not a security boundary, and zlib
   // is already a linked dependency on both sides -- Python's zlib module wraps the identical
   // C library -- whereas matching a from-scratch/vendored SHA-256 bit-for-bit across two
   // independent language implementations would be new correctness-critical code for a check
   // that doesn't need cryptographic strength) over the currently-loaded .idx's
   // [pep_pos, footer_pos) byte range -- the same range tools/carafe_ms2_to_fi_mask.py's
   // idx_fingerprint() hashes (raw peptide table + protein list; Phase 0.5 stopped persisting
   // anything past that, so footer_pos IS the end of both, unlike the old v1 format's
   // narrower [pep_pos, var_pos) -- see docs/20260805_carafe.md Section 6.10). Returns false
   // (sets g_cometStatus) if the .idx can't be opened or isn't a valid v2 file.
   static bool ComputeIdxFingerprint(unsigned long& fingerprint, uint64_t& numRawPeptides);
};

#endif // _COMETPREDICTEDMASK_H_
