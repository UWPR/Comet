#!/usr/bin/env python3
"""
Structural comparison of two Comet fragment-ion plain-peptide .idx files.

Usage:
    python compare_idx.py <old.idx> <new.idx>       # compare two index files
    python compare_idx.py --dump <file.idx>         # dump contents to text

Exit code 0 = equivalent (no failures); 1 = differences found.

Memory-safe design
------------------
The original implementation built full Python dicts for 190M-peptide files
(~100+ GB peak RSS) causing WSL2 OOM / session kill.

This version uses two strategies in order:
  1. Binary-chunk comparison of the peptide and protein sections (O(16 MB)
     memory, fastest — catches identical files in one pass).
  2. Streaming semantic comparison if binary differs: records are parsed one
     at a time (O(buffer)); protein-list counts stored in array.array('I')
     (4 bytes × N, ~760 MB for 190M lists vs ~5 GB for Python list).
"""

import array
import struct
import sys
import os

MASS_TOL       = 1e-6   # Da
CHUNK          = 16 << 20   # 16 MB read chunk
MAX_PRINT      = 10
PROGRESS_EVERY = 5_000_000


# ---------------------------------------------------------------------------
# File header / offset helpers
# ---------------------------------------------------------------------------

def _skip_header(f):
    while True:
        line = f.readline()
        if not line or line.startswith(b"RequireVariableMod:"):
            f.readline()
            break


def _section_offsets(f):
    """Return (pep_pos, prot_pos, perm_pos) from last 24 bytes."""
    f.seek(-24, 2)
    return struct.unpack("<qqq", f.read(24))


def _open_idx(path):
    f = open(path, "rb")
    _skip_header(f)
    pep_pos, prot_pos, perm_pos = _section_offsets(f)
    f.seek(pep_pos)
    (num_pep,)   = struct.unpack("<Q", f.read(8))
    f.seek(prot_pos)
    (num_lists,) = struct.unpack("<q", f.read(8))
    return f, num_pep, num_lists, pep_pos, prot_pos, perm_pos


# ---------------------------------------------------------------------------
# Strategy 1 — binary section comparison (O(16 MB) memory)
# ---------------------------------------------------------------------------

def _sections_identical(fa, fb, start, end):
    """Return True if bytes [start, end) are identical in both files."""
    fa.seek(start)
    fb.seek(start)
    pos = start
    while pos < end:
        n = min(CHUNK, end - pos)
        ba = fa.read(n)
        bb = fb.read(n)
        if ba != bb:
            return False
        pos += n
    return True


# ---------------------------------------------------------------------------
# Strategy 2 — streaming semantic comparison
# ---------------------------------------------------------------------------

class _PepReader:
    _TAIL = 1 + 1 + 8 + 2 + 8   # prevAA nextAA mass siVar pidx

    def __init__(self, f, pep_pos, num_pep):
        self._f   = f
        self._n   = num_pep
        self._i   = 0
        self._buf = b""
        self._p   = 0
        f.seek(pep_pos + 8)

    def _ensure(self, n):
        have = len(self._buf) - self._p
        if have < n:
            more = self._f.read(max(n - have, 1 << 20))
            self._buf = self._buf[self._p:] + more
            self._p   = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self._i >= self._n:
            raise StopIteration
        self._ensure(4)
        (iLen,) = struct.unpack_from("<i", self._buf, self._p);  self._p += 4
        self._ensure(iLen + self._TAIL)
        seq    = self._buf[self._p : self._p + iLen].decode("ascii");  self._p += iLen
        prev   = chr(self._buf[self._p]);  self._p += 1
        nxt    = chr(self._buf[self._p]);  self._p += 1
        (mass,)  = struct.unpack_from("<d", self._buf, self._p);  self._p += 8
        (sivar,) = struct.unpack_from("<H", self._buf, self._p);  self._p += 2
        (pidx,)  = struct.unpack_from("<q", self._buf, self._p);  self._p += 8
        self._i += 1
        return seq, prev, nxt, mass, sivar, pidx


def _read_prot_counts(f, prot_pos, prot_section_size, num_lists):
    """
    Read protein-list section and return an array.array('I') of per-list
    protein counts.  Reads the section in one I/O call (bytes object, freed
    immediately after walk) then stores 4 bytes per list entry.
    Peak extra memory: prot_section_size (bytes) + num_lists * 4 (array).
    """
    f.seek(prot_pos + 8)                       # skip int64 num_lists
    buf = f.read(prot_section_size - 8)        # bytes object (~3.1 GB)
    counts = array.array('I')                  # uint32, 4 bytes each
    p = 0
    for _ in range(num_lists):
        (cnt,) = struct.unpack_from("<Q", buf, p)
        p += 8 + cnt * 8
        counts.append(cnt & 0xFFFFFFFF)
    return counts                              # buf freed after return


def _semantic_compare(fo, fn, num_pep, num_lists_o, num_lists_n,
                      pep_pos_o, pep_pos_n,
                      prot_pos_o, prot_pos_n,
                      perm_pos_o, perm_pos_n, verbose):
    failures = 0
    warnings = 0

    prot_sec_o = perm_pos_o - prot_pos_o
    prot_sec_n = perm_pos_n - prot_pos_n

    if verbose:
        print("  Reading protein-list counts (old) ...")
    counts_o = _read_prot_counts(fo, prot_pos_o, prot_sec_o, num_lists_o)

    if verbose:
        print("  Reading protein-list counts (new) ...")
    counts_n = _read_prot_counts(fn, prot_pos_n, prot_sec_n, num_lists_n)

    if verbose:
        print("  Streaming peptide comparison ...")

    seq_fail = mass_fail = sivar_fail = prot_fail = flank_warn = 0
    seq_ex = []
    mass_ex = []
    sivar_ex = []
    prot_ex = []
    flank_ex = []

    n = min(num_pep,
            # num_pep for both files may differ if counts differ
            num_pep)

    for i, (ro, rn) in enumerate(zip(
            _PepReader(fo, pep_pos_o, num_pep),
            _PepReader(fn, pep_pos_n, num_pep))):
        so, prev_o, next_o, mass_o, sivar_o, pidx_o = ro
        sn, prev_n, next_n, mass_n, sivar_n, pidx_n = rn

        if so != sn:
            seq_fail += 1
            if len(seq_ex) < MAX_PRINT:
                seq_ex.append((i, so, sn))

        dm = abs(mass_o - mass_n)
        if dm > MASS_TOL:
            mass_fail += 1
            if len(mass_ex) < MAX_PRINT:
                mass_ex.append((so, mass_o, mass_n, dm))

        if sivar_o != sivar_n:
            sivar_fail += 1
            if len(sivar_ex) < MAX_PRINT:
                sivar_ex.append((so, sivar_o, sivar_n))

        pc_o = counts_o[pidx_o] if pidx_o < len(counts_o) else -1
        pc_n = counts_n[pidx_n] if pidx_n < len(counts_n) else -1
        if pc_o != pc_n:
            prot_fail += 1
            if len(prot_ex) < MAX_PRINT:
                prot_ex.append((so, pc_o, pc_n))

        if prev_o != prev_n or next_o != next_n:
            flank_warn += 1
            if len(flank_ex) < MAX_PRINT:
                flank_ex.append((so, prev_o, next_o, prev_n, next_n))

        if verbose and (i + 1) % PROGRESS_EVERY == 0:
            print(f"  ... {i+1:,} / {num_pep:,} peptides compared")

    if seq_fail:
        print(f"FAIL: {seq_fail:,} sequence mismatch(es)")
        for idx, a, b in seq_ex:
            print(f"      record {idx}: old={a!r} new={b!r}")
        failures += 1
    if mass_fail:
        print(f"FAIL: {mass_fail:,} mass mismatch(es) (tol={MASS_TOL})")
        for seq, mo, mn, dm in mass_ex:
            print(f"      {seq}: old={mo:.8f} new={mn:.8f} delta={dm:.2e}")
        failures += 1
    if sivar_fail:
        print(f"FAIL: {sivar_fail:,} siVarModProteinFilter mismatch(es)")
        for seq, vo, vn in sivar_ex:
            print(f"      {seq}: old={vo} new={vn}")
        failures += 1
    if prot_fail:
        print(f"FAIL: {prot_fail:,} protein-count mismatch(es)")
        for seq, po, pn in prot_ex:
            print(f"      {seq}: old_count={po} new_count={pn}")
        failures += 1
    if flank_warn:
        warnings = flank_warn
        print(f"WARN: {flank_warn:,} flanking-AA difference(s) (cPrevAA/cNextAA) — acceptable per design")
        for seq, po, no_, pn, nn in flank_ex:
            print(f"      {seq}: old={po}.{no_}  new={pn}.{nn}")

    return failures, warnings


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def compare(old_path, new_path, verbose=True):
    fo, num_pep_o, num_lists_o, pep_pos_o, prot_pos_o, perm_pos_o = _open_idx(old_path)
    fn, num_pep_n, num_lists_n, pep_pos_n, prot_pos_n, perm_pos_n = _open_idx(new_path)

    failures = 0

    # structural header checks
    if num_pep_o != num_pep_n:
        print(f"FAIL: peptide count old={num_pep_o:,} new={num_pep_n:,}")
        failures += 1
    else:
        print(f"Peptide count: {num_pep_o:,}  (match)")
    if num_lists_o != num_lists_n:
        print(f"FAIL: protein-list count old={num_lists_o:,} new={num_lists_n:,}")
        failures += 1
    else:
        print(f"Protein-list count: {num_lists_o:,}  (match)")

    if failures:
        fo.close(); fn.close()
        print("FAIL: structural header mismatch — aborting")
        return failures

    # --- Strategy 1: binary section compare (O(16 MB) memory) ---
    if verbose:
        print("Binary comparison: peptide section ...")
    pep_match = _sections_identical(fo, fn, pep_pos_o, prot_pos_o)
    if verbose:
        print("Binary comparison: protein-list section ...")
    prot_match = _sections_identical(fo, fn, prot_pos_o, perm_pos_o)

    if pep_match and prot_match:
        fo.close(); fn.close()
        print("PASS: peptide and protein-list sections are byte-for-byte identical")
        return 0

    # --- Strategy 2: semantic streaming compare ---
    if verbose:
        if not pep_match:
            print("Peptide sections differ — running semantic comparison ...")
        if not prot_match:
            print("Protein-list sections differ — running semantic comparison ...")

    failures, warnings = _semantic_compare(
        fo, fn,
        num_pep_o, num_lists_o, num_lists_n,
        pep_pos_o, pep_pos_n,
        prot_pos_o, prot_pos_n,
        perm_pos_o, perm_pos_n,
        verbose)

    fo.close(); fn.close()

    if failures == 0:
        print(f"PASS: {num_pep_o:,} peptides semantically equivalent, "
              f"{warnings:,} flanking-AA warning(s)")
    else:
        print(f"FAIL: {failures} failure category(s), {warnings:,} warning(s)")
    return failures


def dump_idx(path, max_rows=50):
    f, num_pep, num_lists, pep_pos, prot_pos, perm_pos = _open_idx(path)
    print(f"Peptides: {num_pep:,}   ProtLists: {num_lists:,}")
    print(f"{'#':<6} {'Peptide':<30} {'Mass':>14}  Prev Next  siVar  pidx")
    print("-" * 80)
    for i, (seq, prev, nxt, mass, sivar, pidx) in enumerate(
            _PepReader(f, pep_pos, num_pep)):
        if i >= max_rows:
            print(f"  ... {num_pep - max_rows:,} more rows ...")
            break
        print(f"{i:<6} {seq:<30} {mass:>14.6f}  {prev:>4} {nxt:>4}  {sivar:>5}  {pidx}")
    f.close()


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] != "--dump":
        rc = compare(sys.argv[1], sys.argv[2])
        sys.exit(0 if rc == 0 else 1)
    elif len(sys.argv) == 3 and sys.argv[1] == "--dump":
        dump_idx(sys.argv[2])
        sys.exit(0)
    else:
        print("Usage:")
        print("  compare_idx.py <old.idx> <new.idx>       # compare two index files")
        print("  compare_idx.py --dump <file.idx>         # dump contents to text")
        sys.exit(1)
