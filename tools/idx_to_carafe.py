#!/usr/bin/env python3
"""
Read a Comet unified .idx file (PI_DB/FI_DB, "Comet index database v1") and export every
peptide it encodes -- sequence plus static and variable modifications -- to a TSV that
Carafe's ai_pred.py can consume directly for in-silico MS2 (and RT/CCS) prediction:

    python ai_pred.py --model_dir generic --mode general \
        --in_file carafe_peptides.tsv --out_dir out/ --out_prefix mylib \
        --device cpu --instrument Lumos --nce 27 --tf_type ms2

Output columns: sequence, mods, mod_sites, charge -- the exact "sequence/mods/mod_sites/
charge" input ai_pred.py's predict_ms2()/predict_rt() expect. mods/mod_sites use Carafe's
own AlphaBase-name convention: "<UniModTitle>@<Site>" / 1-based residue position (0 = N-term,
len+1 = C-term), semicolon-separated, matching AIGear.load_mod_map()'s output and
get_modified_peptide()'s "pos - 1" indexing (both in Carafe's AIGear.java).

The .idx binary format has no C++/Java API of its own -- this script is a from-scratch
reimplementation of CometPeptideIndex::ReadPeptideIndex() / MaterializeOneEntry() (see
CometSearch/CometPeptideIndex.cpp) in Python, decoding the same four sections (raw peptide
table, protein list, mod-permutation tables, compact per-variant array) that the C++ reader
does. See the format notes above each _read_* function below for the exact byte layout each
one depends on -- keep those in sync with CometPeptideIndex.cpp if the .idx format ever
changes.

Known limitation: Comet's variable-mod terminal flag (protein-N-term vs. plain peptide-N-term,
etc.) is NOT persisted in the .idx header -- only whether a mod is N-term/C-term at all. Every
N-term/C-term variable mod is therefore exported using the generic "@N-term"/"@C-term" site
(matching what AIGear.load_mod_map() itself produces for non-protein-term unimod entries);
a mod that was configured in comet.params as specifically protein-terminal will still be
tagged "@N-term"/"@C-term" here rather than "@Protein_N-term"/"@Protein_C-term". Peptides that
happen to sit at a protein terminus (cPrevAA/cNextAA == '-') are still detected correctly for
applying *static* protein-terminal mods (add_Nterm_protein/add_Cterm_protein).

Modification NAMES (the "<UniModTitle>@<Site>" string) are resolved by matching each Comet
mod's mass + affected residue/terminus against Carafe's own top_modifications.tsv -- the
curated list of PTMs Carafe's AI models actually support (also what `carafe -printPTM` prints).
Point --carafe-mods-tsv at it if this script can't find it next to a sibling Carafe checkout.
Any Comet mod that doesn't match a row within --mass-tol is reported as unresolved and, by
default, causes that peptide to be skipped (see --unresolved-mod-name to emit a placeholder
name instead so unmapped peptides aren't silently dropped).
"""

import argparse
import os
import struct
import sys
from collections import namedtuple

WIDTH_REFERENCE = 256      # CometSearch/core/Constants.h -- protein name block size
FRAGINDEX_VMODS = 5        # CometSearch/core/Constants.h -- only the first 5 variable_modNN
                            # slots participate in index builds/searches (PermuteIndexPeptideMods,
                            # EnumerateIndexPeptideMods both loop 0..FRAGINDEX_VMODS-1)
VMODS = 15                 # CometSearch/core/Constants.h -- total variable_modNN slots the
                            # header's "VariableMod:" line always writes

# UniMod accession -> the short/PSI-MS title AlphaBase mod names use (the string before '@').
# Covers exactly the unimod_accession values appearing in Carafe's top_modifications.tsv.
ACCESSION_TO_ALPHABASE_TITLE = {
    "UNIMOD:4":    "Carbamidomethyl",
    "UNIMOD:35":   "Oxidation",
    "UNIMOD:7":    "Deamidated",
    "UNIMOD:1":    "Acetyl",
    "UNIMOD:21":   "Phospho",
    "UNIMOD:121":  "GG",
    "UNIMOD:737":  "TMT6plex",     # covers Carafe's "TMT 6/10/11-plex" rows -- same UniMod accession/mass
    "UNIMOD:738":  "TMT2plex",
    "UNIMOD:2016": "TMTpro",
    "UNIMOD:214":  "iTRAQ4plex",
    "UNIMOD:730":  "iTRAQ8plex",
    "UNIMOD:27":   "Glu->pyro-Glu",
    "UNIMOD:28":   "Gln->pyro-Glu",
}

# Fallback copy of Carafe's top_modifications.tsv (its 28-row curated PTM list as of this
# writing) used only when --carafe-mods-tsv isn't given and no sibling Carafe checkout is
# found. Keep in sync with Carafe/src/main/resources/top_modifications.tsv if that file grows;
# passing --carafe-mods-tsv explicitly always wins over this and the auto-detected path.
BUILTIN_TOP_MODIFICATIONS_TSV = """mod_id\tmod_name\tmod_mass\tmod_type\tmod_category\tunimod_accession
1\tCarbamidomethyl of C\t57.02146372057\tParticular Amino Acid\tCommon\tUNIMOD:4
2\tOxidation of M\t15.99491461956\tParticular Amino Acid\tCommon\tUNIMOD:35
3\tDeamidated of N\t0.9840155826899988\tParticular Amino Acid\tCommon_Artifact\tUNIMOD:7
4\tDeamidated of Q\t0.9840155826899988\tParticular Amino Acid\tCommon_Artifact\tUNIMOD:7
5\tAcetyl of protein N-term\t42.0105646837\tPeptide N-term\tCommon_Biological\tUNIMOD:1
6\tAcetyl of K\t42.0105646837\tParticular Amino Acid\tCommon_Biological\tUNIMOD:1
7\tPhospho of S\t79.96633052074999\tParticular Amino Acid\tCommon_Biological\tUNIMOD:21
8\tPhospho of T\t79.96633052074999\tParticular Amino Acid\tCommon_Biological\tUNIMOD:21
9\tPhospho of Y\t79.96633052074999\tParticular Amino Acid\tCommon_Biological\tUNIMOD:21
10\tGG of K\t114.04292744114\tParticular Amino Acid\tLess_Common\tUNIMOD:121
11\tTMT 10-plex of K\t229.16293213472\tParticular Amino Acid\tLabeling\tUNIMOD:737
12\tTMT 10-plex of peptide N-term\t229.16293213472\tPeptide N-term\tLabeling\tUNIMOD:737
13\tTMT 11-plex of K\t229.16293213472\tParticular Amino Acid\tLabeling\tUNIMOD:737
14\tTMT 11-plex of peptide N-term\t229.16293213472\tPeptide N-term\tLabeling\tUNIMOD:737
17\tTMT 6-plex of K\t229.16293213472\tParticular Amino Acid\tLabeling\tUNIMOD:737
18\tTMT 6-plex of peptide N-term\t229.16293213472\tPeptide N-term\tLabeling\tUNIMOD:737
15\tTMT 2-plex of K\t225.15583272792\tParticular Amino Acid\tLabeling\tUNIMOD:738
16\tTMT 2-plex of peptide N-term\t225.15583272792\tPeptide N-term\tLabeling\tUNIMOD:738
19\tTMTpro of K\t304.20714532623\tParticular Amino Acid\tLabeling\tUNIMOD:2016
20\tTMTpro of peptide N-term\t304.20714532623\tPeptide N-term\tLabeling\tUNIMOD:2016
21\tiTRAQ 4-plex of K\t144.1020624208\tParticular Amino Acid\tLabeling\tUNIMOD:214
22\tiTRAQ 4-plex of peptide N-term\t144.1020624208\tPeptide N-term\tLabeling\tUNIMOD:214
23\tiTRAQ 4-plex of Y\t144.1020624208\tParticular Amino Acid\tLabeling\tUNIMOD:214
24\tiTRAQ 8-plex of K\t304.19903946116\tParticular Amino Acid\tLabeling\tUNIMOD:730
25\tiTRAQ 8-plex of peptide N-term\t304.19903946116\tPeptide N-term\tLabeling\tUNIMOD:730
26\tiTRAQ 8-plex of Y\t304.19903946116\tParticular Amino Acid\tLabeling\tUNIMOD:730
27\tGlu->pyro-Glu of E\t-18.0105646837\tPeptide N-term - Particular Amino Acid(s)\tCommon_Artifact\tUNIMOD:27
28\tGln->pyro-Glu of Q\t-17.02654910101\tPeptide N-term - Particular Amino Acid(s)\tCommon_Artifact\tUNIMOD:28
"""


RawPeptide = namedtuple("RawPeptide",
    "seq prevAA nextAA mass sivar protein_row")

VarModSlot = namedtuple("VarModSlot",
    "chars mass nl nl2 is_nterm is_cterm active")


# ---------------------------------------------------------------------------
# Carafe mod-name table
# ---------------------------------------------------------------------------

class CarafeModTable:
    """(mass, site) -> AlphaBase mod name, resolved from Carafe's top_modifications.tsv."""

    def __init__(self, tsv_path, mass_tol):
        self.mass_tol = mass_tol
        # entries: list of (mass, site_kind, residue_or_None, alphabase_name, unimod_acc)
        self.entries = []
        self._load(tsv_path)

    def _load(self, tsv_path):
        if tsv_path:
            with open(tsv_path, "r", encoding="utf-8") as f:
                text = f.read()
        else:
            text = BUILTIN_TOP_MODIFICATIONS_TSV

        lines = text.splitlines()
        header = lines[0].split("\t")
        col = {name: i for i, name in enumerate(header)}
        for line in lines[1:]:
            if not line.strip():
                continue
            d = line.split("\t")
            mod_name = d[col["mod_name"]]
            mass = float(d[col["mod_mass"]])
            accession = d[col["unimod_accession"]]

            title = ACCESSION_TO_ALPHABASE_TITLE.get(accession)
            if title is None:
                # Fall back to the text before " of " in mod_name -- not guaranteed to be
                # the exact UniMod PSI-MS title, but better than dropping the row outright.
                title = mod_name.split(" of ")[0]

            site_text = mod_name.split(" of ", 1)[1] if " of " in mod_name else ""
            if site_text == "protein N-term":
                self.entries.append((mass, "nterm", None, title + "@Protein_N-term", accession))
            elif site_text == "protein C-term":
                self.entries.append((mass, "cterm", None, title + "@Protein_C-term", accession))
            elif site_text == "peptide N-term" or site_text == "N-term":
                self.entries.append((mass, "nterm", None, title + "@N-term", accession))
            elif site_text == "peptide C-term" or site_text == "C-term":
                self.entries.append((mass, "cterm", None, title + "@C-term", accession))
            elif len(site_text) == 1 and site_text.isalpha():
                self.entries.append((mass, "residue", site_text, title + "@" + site_text, accession))
            # else: unrecognized site format (e.g. the pyro-Glu "Peptide N-term - Particular
            # Amino Acid(s)" category) -- already captured above via its residue-letter site
            # ("of E" / "of Q"), so nothing further to add here.

    def resolve(self, mass, site_kind, residue=None, is_protein_terminal=False):
        """site_kind: 'residue' (with residue=letter), 'nterm', or 'cterm'.

        A "Protein_N-term"/"Protein_C-term" table entry is only ever returned when
        is_protein_terminal is True (i.e. the peptide's flanking residue is '-' at that
        end) -- Carafe's top_modifications.tsv has no generic, non-protein-specific
        N-term entry for some mods (e.g. Acetyl), and applying that name to a peptide
        that isn't actually protein-terminal would claim a biologically-impossible
        modification. See the module docstring's note on why the .idx can't always tell
        us whether a *variable* term mod was configured as protein-specific."""
        generic = None
        protein_variant = None
        for entry_mass, entry_kind, entry_residue, name, accession in self.entries:
            if entry_kind != site_kind:
                continue
            if site_kind == "residue" and entry_residue != residue:
                continue
            if abs(entry_mass - mass) > self.mass_tol:
                continue
            if "Protein_" in name:
                protein_variant = name
            else:
                generic = name
        if generic is not None:
            return generic
        if protein_variant is not None and is_protein_terminal:
            return protein_variant
        return None


# ---------------------------------------------------------------------------
# .idx reader
# ---------------------------------------------------------------------------

class IdxReader:
    def __init__(self, path):
        self.path = path
        self.f = open(path, "rb")
        self.header = {}
        self.static_mods = {}      # 'A'..'Z' -> mass delta (only nonzero entries)
        self.static_nterm_pep = 0.0
        self.static_cterm_pep = 0.0
        self.static_nterm_prot = 0.0
        self.static_cterm_prot = 0.0
        self.var_mods = []         # VMODS entries, index 0..VMODS-1
        self.slot_map = []         # vModSlotForAllModsIdx: compacted list of active slot indices, FRAGINDEX_VMODS scope only

        self._read_header()
        self._read_footer()

    # -- header: text lines up to and including the first blank line. Field formats mirror
    # CometPeptideIndex::WritePeptideIndex()'s fprintf calls exactly. --
    def _read_header(self):
        self.f.seek(0)
        magic = self.f.readline()
        if not magic.startswith(b"Comet index database v1"):
            raise ValueError(
                f"{self.path!r} is not a 'Comet index database v1' unified .idx file "
                "(old-format PI_DB/FI_DB files aren't supported -- rebuild with a current Comet).")

        while True:
            line = self.f.readline()
            if not line or line in (b"\n", b"\r\n"):
                break
            text = line.decode("ascii", errors="replace").rstrip("\r\n")
            if text.startswith("MassRange:"):
                lo, hi = text[len("MassRange:"):].split()
                self.header["mass_range"] = (float(lo), float(hi))
            elif text.startswith("LengthRange:"):
                lo, hi = text[len("LengthRange:"):].split()
                self.header["length_range"] = (int(lo), int(hi))
            elif text.startswith("DecoySearch:"):
                self.header["decoy_search"] = int(text.split(":")[1].strip())
            elif text.startswith("StaticMod:"):
                vals = [float(x) for x in text[len("StaticMod:"):].split()]
                # 65..90 = 'A'..'Z', then Nterm-peptide, Cterm-peptide, Nterm-protein, Cterm-protein
                for i, aa in enumerate("ABCDEFGHIJKLMNOPQRSTUVWXYZ"):
                    if vals[i] != 0.0:
                        self.static_mods[aa] = vals[i]
                self.static_nterm_pep = vals[26]
                self.static_cterm_pep = vals[27]
                self.static_nterm_prot = vals[28]
                self.static_cterm_prot = vals[29]
            elif text.startswith("VariableMod:"):
                toks = text[len("VariableMod:"):].split()
                for i, tok in enumerate(toks[:VMODS]):
                    chars, mass, nl, nl2 = tok.split(":")
                    mass = float(mass)
                    active = (mass != 0.0) and (chars[:1] != "-")
                    self.var_mods.append(VarModSlot(
                        chars=chars, mass=mass, nl=float(nl), nl2=float(nl2),
                        is_nterm=("n" in chars), is_cterm=("c" in chars), active=active))
                while len(self.var_mods) < VMODS:
                    self.var_mods.append(VarModSlot("-", 0.0, 0.0, 0.0, False, False, False))

        # CometFragmentIndex::PermuteIndexPeptideMods()/EnumerateIndexPeptideMods() only ever
        # consider the first FRAGINDEX_VMODS slots -- MOD_NUMBERS[].modifications[] values are
        # indices into this compacted list, and FragmentPeptidesStruct.cNtermMod/cCtermMod are
        # raw slot indices already restricted to this same range.
        self.slot_map = [i for i in range(FRAGINDEX_VMODS) if i < len(self.var_mods) and self.var_mods[i].active]

    def _read_footer(self):
        self.f.seek(0, os.SEEK_END)
        file_size = self.f.tell()
        self.f.seek(-4 * 8, os.SEEK_END)
        (self.pep_pos, self.prot_pos, self.perm_pos, self.var_pos) = struct.unpack("<qqqq", self.f.read(32))
        self.footer_pos = file_size - 4 * 8

    # -- Raw peptide table @ pep_pos: count(u64), then per-entry iLen(i32) szPeptide(iLen)
    # cPrevAA(c) cNextAA(c) dPepMass(d) siVarModProteinFilter(u16) lIndexProteinFilePosition(i64) --
    def read_raw_peptides(self):
        f = self.f
        f.seek(self.pep_pos)
        (num_raw,) = struct.unpack("<Q", f.read(8))
        section_size = self.prot_pos - self.pep_pos - 8
        buf = f.read(section_size)
        raw = []
        p = 0
        for _ in range(num_raw):
            (ilen,) = struct.unpack_from("<i", buf, p); p += 4
            seq = buf[p:p + ilen].decode("ascii"); p += ilen
            prevAA = chr(buf[p]); p += 1
            nextAA = chr(buf[p]); p += 1
            (mass,) = struct.unpack_from("<d", buf, p); p += 8
            (sivar,) = struct.unpack_from("<H", buf, p); p += 2
            (protrow,) = struct.unpack_from("<q", buf, p); p += 8
            raw.append(RawPeptide(seq, prevAA, nextAA, mass, sivar, protrow))
        return raw

    # -- Protein list (CSR) @ prot_pos: count(i64) then per-row count(i64) + offsets(i64 x count) --
    def read_protein_list(self):
        f = self.f
        f.seek(self.prot_pos)
        (num_rows,) = struct.unpack("<q", f.read(8))
        section_size = self.perm_pos - self.prot_pos - 8
        buf = f.read(section_size)
        rows = []
        p = 0
        for _ in range(num_rows):
            (cnt,) = struct.unpack_from("<q", buf, p); p += 8
            offsets = struct.unpack_from(f"<{cnt}q", buf, p); p += 8 * cnt
            rows.append(offsets)
        return rows

    def read_protein_name(self, file_offset, cache):
        if file_offset in cache:
            return cache[file_offset]
        self.f.seek(file_offset)
        buf = self.f.read(WIDTH_REFERENCE)
        name = buf.split(b"\x00", 1)[0].decode("ascii", errors="replace")
        cache[file_offset] = name
        return name

    # -- Permutation tables @ perm_pos: ulSizeModSeqs/ulSizeRaw/ulModNumSize(u64 x3), then
    # MOD_SEQ_MOD_NUM_START[ulSizeModSeqs](i32), MOD_SEQ_MOD_NUM_CNT[ulSizeModSeqs](i32),
    # PEPTIDE_MOD_SEQ_IDXS[ulSizeRaw](i32), then MOD_SEQS as (len(i32), bytes) x ulSizeModSeqs,
    # then MOD_NUMBERS as (modStringLen(i32), signed-char bytes) x ulModNumSize --
    def read_permutation_tables(self):
        f = self.f
        f.seek(self.perm_pos)
        n_modseqs, n_raw, n_modnums = struct.unpack("<QQQ", f.read(24))
        mod_seq_start = struct.unpack(f"<{n_modseqs}i", f.read(4 * n_modseqs))
        mod_seq_cnt = struct.unpack(f"<{n_modseqs}i", f.read(4 * n_modseqs))
        peptide_mod_seq_idxs = struct.unpack(f"<{n_raw}i", f.read(4 * n_raw))

        remaining = self.var_pos - f.tell()
        buf = f.read(remaining)
        p = 0
        mod_seqs = []
        for _ in range(n_modseqs):
            (l,) = struct.unpack_from("<i", buf, p); p += 4
            mod_seqs.append(buf[p:p + l].decode("ascii")); p += l
        mod_numbers = []
        for _ in range(n_modnums):
            (l,) = struct.unpack_from("<i", buf, p); p += 4
            mod_numbers.append(struct.unpack_from(f"<{l}b", buf, p)); p += l

        return mod_seq_start, mod_seq_cnt, peptide_mod_seq_idxs, mod_seqs, mod_numbers

    # -- Compact variant array @ var_pos: count(u64), then per-entry dPepMass(d)
    # iWhichPeptide(u32) modNumIdx(i32) cNtermMod(signed char) cCtermMod(signed char) --
    def iter_variants(self, chunk_size=1 << 20):
        f = self.f
        f.seek(self.var_pos)
        (num_variants,) = struct.unpack("<Q", f.read(8))
        entry_fmt = "<dIibb"
        entry_size = struct.calcsize(entry_fmt)
        remaining = num_variants
        while remaining > 0:
            n = min(chunk_size, remaining)
            buf = f.read(entry_size * n)
            for i in range(n):
                yield struct.unpack_from(entry_fmt, buf, i * entry_size)
            remaining -= n
        return num_variants

    def num_variants(self):
        f = self.f
        f.seek(self.var_pos)
        (num_variants,) = struct.unpack("<Q", f.read(8))
        return num_variants


# ---------------------------------------------------------------------------
# Peptide -> (mods, mod_sites) reconstruction
# ---------------------------------------------------------------------------

class ModResolutionError(Exception):
    pass


def decode_variable_mods(raw, mod_num_idx, cn_term_mod, cc_term_mod,
                          peptide_mod_seq_idxs, mod_seqs, mod_numbers, slot_map, var_mods,
                          which_peptide):
    """Returns list of (0-based position OR 'nterm'/'cterm', VarModSlot) -- mirrors
    CometPeptideIndex::MaterializeOneEntry()'s reconstruction exactly."""
    sites = []
    seq = raw.seq
    if mod_num_idx >= 0:
        mods = mod_numbers[mod_num_idx]
        mod_seq_idx = peptide_mod_seq_idxs[which_peptide]
        mod_seq = mod_seqs[mod_seq_idx]
        j = 0
        for i, c in enumerate(seq):
            if j < len(mod_seq) and c == mod_seq[j]:
                if mods[j] != -1:
                    slot = slot_map[mods[j]]
                    sites.append((i, var_mods[slot]))
                j += 1
    if cn_term_mod >= 0:
        sites.append(("nterm", var_mods[cn_term_mod]))
    if cc_term_mod >= 0:
        sites.append(("cterm", var_mods[cc_term_mod]))
    return sites


def decode_static_mods(raw, static_mods, static_nterm_pep, static_cterm_pep,
                        static_nterm_prot, static_cterm_prot):
    """Returns list of (0-based position OR 'nterm'/'cterm', mass, is_protein_term)."""
    sites = []
    for i, c in enumerate(raw.seq):
        m = static_mods.get(c)
        if m:
            sites.append((i, m, False))
    if raw.prevAA == "-" and static_nterm_prot:
        sites.append(("nterm", static_nterm_prot, True))
    elif static_nterm_pep:
        sites.append(("nterm", static_nterm_pep, False))
    if raw.nextAA == "-" and static_cterm_prot:
        sites.append(("cterm", static_cterm_prot, True))
    elif static_cterm_pep:
        sites.append(("cterm", static_cterm_pep, False))
    return sites


def build_mods_mod_sites(raw, static_sites, var_sites, mod_table, mass_tol, unresolved_name_fmt):
    """Turns decoded static+variable mod sites into Carafe's semicolon-separated
    "mods"/"mod_sites" strings. Returns (mods_str, mod_sites_str, unresolved list)."""
    tokens = []   # (sort_key, mod_sites_value, name)
    unresolved = []

    def sort_key(pos):
        if pos == "nterm":
            return -1
        if pos == "cterm":
            return len(raw.seq) + 1
        return pos

    for pos, mass, is_protein_term in static_sites:
        if pos == "nterm":
            name = mod_table.resolve(mass, "nterm", is_protein_terminal=is_protein_term)
            site_val = 0
        elif pos == "cterm":
            name = mod_table.resolve(mass, "cterm", is_protein_terminal=is_protein_term)
            site_val = len(raw.seq) + 1
        else:
            name = mod_table.resolve(mass, "residue", residue=raw.seq[pos])
            site_val = pos + 1
        if name is None:
            if unresolved_name_fmt:
                name = unresolved_name_fmt.format(mass=mass, residue=raw.seq[pos] if isinstance(pos, int) else pos)
            else:
                unresolved.append((pos, mass))
                continue
        tokens.append((sort_key(pos), site_val, name))

    # Comet's index-mode variable-mod enumeration applies an N-term/C-term variable mod to
    # every peptide's terminus regardless of whether it was configured as protein-specific
    # (EnumerateIndexPeptideMods() has no protein-terminal check -- see module docstring), so
    # a variant carrying one of these doesn't by itself mean the peptide IS protein-terminal.
    # Gate the name resolution on the peptide's *actual* flanking residue instead, exactly as
    # for static sites above -- CarafeModTable.resolve() then only hands back a "Protein_"
    # name when that's actually true, and reports unresolved (rather than a wrong name)
    # otherwise for mods (like Acetyl) with no generic non-protein-specific table entry.
    for pos, slot in var_sites:
        if pos == "nterm":
            name = mod_table.resolve(slot.mass, "nterm", is_protein_terminal=(raw.prevAA == "-"))
            site_val = 0
        elif pos == "cterm":
            name = mod_table.resolve(slot.mass, "cterm", is_protein_terminal=(raw.nextAA == "-"))
            site_val = len(raw.seq) + 1
        else:
            name = mod_table.resolve(slot.mass, "residue", residue=raw.seq[pos])
            site_val = pos + 1
        if name is None:
            if unresolved_name_fmt:
                name = unresolved_name_fmt.format(mass=slot.mass, residue=raw.seq[pos] if isinstance(pos, int) else pos)
            else:
                unresolved.append((pos, slot.mass))
                continue
        tokens.append((sort_key(pos), site_val, name))

    tokens.sort(key=lambda t: t[0])
    mods_str = ";".join(t[2] for t in tokens)
    mod_sites_str = ";".join(str(t[1]) for t in tokens)
    return mods_str, mod_sites_str, unresolved


def is_decoy(protein_names, decoy_prefixes):
    for name in protein_names:
        upper = name.upper()
        if any(upper.startswith(p) for p in decoy_prefixes):
            return True
    return False


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def find_default_carafe_tsv():
    candidates = [
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Carafe",
                     "src", "main", "resources", "top_modifications.tsv"),
        "/mnt/c/Work/Carafe/src/main/resources/top_modifications.tsv",
    ]
    for c in candidates:
        if os.path.isfile(c):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(
        description="Export peptides + modifications from a Comet .idx to a Carafe ai_pred.py input TSV.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__)
    ap.add_argument("idx_file", help="Comet unified .idx file (PI_DB or FI_DB)")
    ap.add_argument("out_tsv", help="Output TSV path (sequence/mods/mod_sites/charge)")
    ap.add_argument("--charges", default="2,3",
                     help="Comma-separated charge states to emit per peptide (default: 2,3)")
    ap.add_argument("--carafe-mods-tsv", default=None,
                     help="Path to Carafe's top_modifications.tsv (default: auto-detect a sibling "
                          "Carafe checkout, else a bundled copy of its 28-row table)")
    ap.add_argument("--mass-tol", type=float, default=0.001,
                     help="Da tolerance matching Comet mod masses to Carafe's mod table (default: 0.001)")
    ap.add_argument("--include-decoys", action="store_true",
                     help="Include peptides whose only proteins are decoys (default: excluded)")
    ap.add_argument("--decoy-prefix", action="append", default=None,
                     help="Protein-name prefix marking a decoy, case-insensitive; repeatable "
                          "(default: DECOY_ and REV_, matching tools/qvalue.py's convention)")
    ap.add_argument("--unmodified-only", action="store_true",
                     help="Skip variable-mod variants; export only each peptide's static-mod-only form")
    ap.add_argument("--unresolved-mod-name",
                     help="Format string (e.g. 'Delta{mass:+.4f}@{residue}') to use for a mod mass "
                          "that doesn't match Carafe's mod table, instead of skipping that peptide")
    ap.add_argument("--max-peptides", type=int, default=None, help="Stop after N output rows (debugging)")
    ap.add_argument("--no-dedup", action="store_true",
                     help="Don't collapse identical (sequence, mods, mod_sites) rows arising from "
                          "different proteins/mod-combinations that produce the same peptide")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    charges = [int(c) for c in args.charges.split(",")]

    tsv_path = args.carafe_mods_tsv or find_default_carafe_tsv()
    mod_table = CarafeModTable(tsv_path, args.mass_tol)
    if args.verbose:
        src = tsv_path if tsv_path else "(bundled fallback copy)"
        print(f"Using Carafe mod table: {src}", file=sys.stderr)

    reader = IdxReader(args.idx_file)
    if args.verbose:
        print(f"Header: {reader.header}", file=sys.stderr)
        active = [(i, m) for i, m in enumerate(reader.var_mods) if m.active]
        print(f"Active variable mods: {active}", file=sys.stderr)
        print(f"Static AA mods: {reader.static_mods}", file=sys.stderr)

    raw_peptides = reader.read_raw_peptides()

    decoy_prefixes = [p.upper() for p in (args.decoy_prefix or ["DECOY_", "REV_"])]
    protein_rows = None
    name_cache = {}
    if not args.include_decoys:
        protein_rows = reader.read_protein_list()

    mod_seq_start, mod_seq_cnt, peptide_mod_seq_idxs, mod_seqs, mod_numbers = \
        reader.read_permutation_tables()

    n_written = 0
    n_skipped_decoy = 0
    n_skipped_unresolved = 0
    n_mass_mismatch = 0
    seen = set()

    with open(args.out_tsv, "wb") as out:
        out.write(b"sequence\tmods\tmod_sites\tcharge\r\n")

        for variant in reader.iter_variants():
            dmass, which_peptide, mod_num_idx, cn_term_mod, cc_term_mod = variant

            if args.unmodified_only and (mod_num_idx >= 0 or cn_term_mod >= 0 or cc_term_mod >= 0):
                continue

            raw = raw_peptides[which_peptide]

            if not args.include_decoys:
                offsets = protein_rows[raw.protein_row] if 0 <= raw.protein_row < len(protein_rows) else ()
                names = [reader.read_protein_name(o, name_cache) for o in offsets]
                if is_decoy(names, decoy_prefixes):
                    n_skipped_decoy += 1
                    continue

            static_sites = decode_static_mods(
                raw, reader.static_mods, reader.static_nterm_pep, reader.static_cterm_pep,
                reader.static_nterm_prot, reader.static_cterm_prot)
            var_sites = decode_variable_mods(
                raw, mod_num_idx, cn_term_mod, cc_term_mod,
                peptide_mod_seq_idxs, mod_seqs, mod_numbers, reader.slot_map, reader.var_mods,
                which_peptide)

            # Self-consistency check: raw.mass (the unmodified-variant baseline read from the
            # raw-peptide table) already has every static mod baked in -- pdAAMassFragment/
            # pdAAMassParent are static-mod-adjusted per-residue tables applied once at
            # digestion time (ParsePeptideIndexHeader()'s "pdAAMassFragment[x] += pdStaticMods[x]"),
            # and EnumerateIndexPeptideMods()/MaterializeOneEntry() both start dCalcPepMass from
            # raw.dPepMass and add ONLY variable-mod deltas on top (CometPeptideIndex.cpp). So the
            # check below adds variable-mod deltas only; adding static_sites' masses again here
            # would double-count them even though static_sites is exactly what's needed for the
            # mods/mod_sites annotation below.
            expected_mass = raw.mass + sum(s.mass for _, s in var_sites)
            if abs(expected_mass - dmass) > 0.01:
                n_mass_mismatch += 1
                if args.verbose:
                    print(f"WARNING: mass mismatch for {raw.seq}: reconstructed {expected_mass:.5f} "
                          f"vs stored {dmass:.5f} (delta {expected_mass - dmass:+.5f})", file=sys.stderr)

            mods_str, mod_sites_str, unresolved = build_mods_mod_sites(
                raw, static_sites, var_sites, mod_table, args.mass_tol, args.unresolved_mod_name)

            if unresolved:
                n_skipped_unresolved += 1
                if args.verbose:
                    print(f"SKIP {raw.seq}: unresolved mod mass(es) {unresolved}", file=sys.stderr)
                continue

            for charge in charges:
                key = (raw.seq, mods_str, mod_sites_str, charge)
                if not args.no_dedup:
                    if key in seen:
                        continue
                    seen.add(key)

                out.write(f"{raw.seq}\t{mods_str}\t{mod_sites_str}\t{charge}\r\n".encode("ascii"))
                n_written += 1

            if args.max_peptides is not None and n_written >= args.max_peptides:
                break

    print(f"Wrote {n_written} rows to {args.out_tsv}", file=sys.stderr)
    if n_skipped_decoy:
        print(f"Skipped {n_skipped_decoy} decoy-only peptide variant(s)", file=sys.stderr)
    if n_skipped_unresolved:
        print(f"Skipped {n_skipped_unresolved} peptide variant(s) with an unresolved mod name "
              "(pass --unresolved-mod-name to keep them with a placeholder, or -v for details)",
              file=sys.stderr)
    if n_mass_mismatch:
        print(f"WARNING: {n_mass_mismatch} peptide variant(s) had a mass mismatch between the "
              "reconstructed static+variable mod set and the .idx's stored mass -- rerun with "
              "-v to see which ones; static protein-terminal mod placement is the likely cause "
              "(see module docstring)", file=sys.stderr)


if __name__ == "__main__":
    main()
