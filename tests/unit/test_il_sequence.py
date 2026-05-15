#!/usr/bin/env python3
"""
Verify that with equal_I_and_L=1, the fragment index stores the original FASTA
sequence (preserving 'L') rather than the canonicalized form (all-'I').

Two peptide pairs are tested:
  - short (<=12 AA):  ACLIVERK  (protein1, first in FASTA)
                      ACIIVERK  (protein2, second in FASTA)
  - long  (>=13 AA):  ACLIVERPEPTIDER  (protein3, first of long pair)
                      ACIIVERPEPTIDER  (protein4, second of long pair)

With equal_I_and_L=1 the two peptides in each pair are canonical equivalents.
The stored sequence must come from the protein with the smaller file offset
(i.e. the first occurrence in the FASTA), so it must contain 'L' not 'I'.
"""

import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

REPO  = Path(__file__).resolve().parent.parent.parent
COMET = REPO / "comet.exe"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

FASTA_TEXT = """\
>sp|P00001|PROT1 Short L-peptide (first in file)
ACLIVERK
>sp|P00002|PROT2 Short I-peptide (second in file)
ACIIVERK
>sp|P00003|PROT3 Long L-peptide (first of long pair)
ACLIVERPEPTIDER
>sp|P00004|PROT4 Long I-peptide (second of long pair)
ACIIVERPEPTIDER
"""

PARAMS_TEMPLATE = """\
# comet_version 2026.02 rev. 0
database_name = {db}
digest_mass_range = 600 5000
peptide_mass_tolerance_upper = 20.0
peptide_mass_tolerance_lower = -20.0
peptide_mass_units = 2
peptide_length_range = 7 50
fragment_bin_tol = 0.02
equal_I_and_L = {il}
num_enzyme_termini = 2
search_enzyme_number = 1
output_percolatorfile = 0

[COMET_ENZYME_INFO]
0.  Cut_everywhere         0      -           -
1.  Trypsin                1      KR          P
"""


def build_index(comet, params_path, work_dir):
    result = subprocess.run(
        [str(comet), "-i", f"-P{params_path}"],
        capture_output=True, text=True, cwd=str(work_dir)
    )
    if result.returncode != 0:
        print("STDOUT:", result.stdout)
        print("STDERR:", result.stderr)
        raise RuntimeError(f"Index build failed (exit {result.returncode})")


def read_peptide_sequences(idx_path):
    """Parse the binary plain-peptide section and return list of sequence strings."""
    with open(idx_path, "rb") as fh:
        # Footer: last 3 × int64 = clPeptidesFilePos, clProteinsFilePos, clPermutationsFilePos
        fh.seek(-24, 2)
        pep_pos, _, _ = struct.unpack("<qqq", fh.read(24))

        fh.seek(pep_pos)
        (n_pep,) = struct.unpack("<Q", fh.read(8))  # size_t = uint64 on Linux

        sequences = []
        for _ in range(n_pep):
            (iLen,) = struct.unpack("<i", fh.read(4))
            seq = fh.read(iLen).decode("ascii")
            fh.read(1)            # cPrevAA
            fh.read(1)            # cNextAA
            fh.read(8)            # dPepMass (double)
            fh.read(2)            # siVarModProteinFilter (uint16)
            fh.read(8)            # lIndexProteinFilePosition (int64)
            sequences.append(seq)

    return sequences


# ---------------------------------------------------------------------------
# Test
# ---------------------------------------------------------------------------

def run_test():
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        fasta = tmp / "test_il.fasta"
        idx   = tmp / "test_il.fasta.idx"

        fasta.write_text(FASTA_TEXT)

        # --- Build with equal_I_and_L = 1 ---
        params_il = tmp / "params_il.txt"
        params_il.write_text(PARAMS_TEMPLATE.format(db=fasta, il=1))
        build_index(COMET, params_il, tmp)

        seqs_il = read_peptide_sequences(idx)
        print(f"  Sequences stored with equal_I_and_L=1: {seqs_il}")

        assert "ACLIVERK" in seqs_il, (
            f"FAIL: expected 'ACLIVERK' (original L, short) in idx, got {seqs_il}")
        assert "ACIIVERK" not in seqs_il, (
            f"FAIL: 'ACIIVERK' (canonicalized I, short) should not appear; got {seqs_il}")
        assert "ACLIVERPEPTIDER" in seqs_il, (
            f"FAIL: expected 'ACLIVERPEPTIDER' (original L, long) in idx, got {seqs_il}")
        assert "ACIIVERPEPTIDER" not in seqs_il, (
            f"FAIL: 'ACIIVERPEPTIDER' (canonicalized I, long) should not appear; got {seqs_il}")

        n_il = len(seqs_il)

        # --- Build with equal_I_and_L = 0 (baseline, no equivalence) ---
        idx.unlink()
        params_no = tmp / "params_no.txt"
        params_no.write_text(PARAMS_TEMPLATE.format(db=fasta, il=0))
        build_index(COMET, params_no, tmp)

        seqs_no = read_peptide_sequences(idx)
        print(f"  Sequences stored with equal_I_and_L=0: {seqs_no}")

        # Without equivalence both variants are distinct entries.
        assert "ACLIVERK"        in seqs_no, "FAIL: ACLIVERK missing when IL=0"
        assert "ACIIVERK"        in seqs_no, "FAIL: ACIIVERK missing when IL=0"
        assert "ACLIVERPEPTIDER" in seqs_no, "FAIL: ACLIVERPEPTIDER missing when IL=0"
        assert "ACIIVERPEPTIDER" in seqs_no, "FAIL: ACIIVERPEPTIDER missing when IL=0"

        n_no = len(seqs_no)
        # Trypsin cleaves ACLIVERK→ACLIVER+K and ACIIVERK→ACIIVER+K, so there are
        # 3 canonical-equivalent pairs: ACLIVER/ACIIVER, ACLIVERK/ACIIVERK,
        # ACLIVERPEPTIDER/ACIIVERPEPTIDER.
        assert n_no == n_il + 3, (
            f"FAIL: IL=0 should have 3 more entries than IL=1 "
            f"(got IL=1: {n_il}, IL=0: {n_no})")

    print("PASS")


if __name__ == "__main__":
    if not COMET.exists():
        print(f"ERROR: comet binary not found at {COMET}", file=sys.stderr)
        sys.exit(1)
    run_test()
