#!/usr/bin/env python3
"""
Comet plain-peptide index unit tests (T1–T7, T11–T12).

Runs Comet.exe -i on each crafted FASTA and verifies expected properties.

Usage:
    python run_tests.py [--comet PATH]  [test_id ...]

    --comet   path to Comet.exe  (default: ../x64/Release/Comet.exe)
    test_id   one or more of: t1 t2 t3 t4_il_true t4_il_false t5_noenz
              t5_trypsin_0mc t5_trypsin_1mc t6 t7 t11 t12
              (default: run all)

Exit code 0 = all tests passed; non-zero = failures.
"""

import argparse
import os
import struct
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path


TESTS_DIR  = Path(__file__).parent.resolve()
DATA_DIR   = TESTS_DIR / "data"
# Windows path for the Comet binary (default)
COMET_EXE  = TESTS_DIR.parent / "x64" / "Release" / "Comet.exe"

MASS_TOL   = 0.002   # Da — loose tolerance for short-peptide monoisotopic masses
WIDTH_REFERENCE = 512


# ---------------------------------------------------------------------------
# params template
# ---------------------------------------------------------------------------

PARAMS_TEMPLATE = textwrap.dedent("""\
# comet_version 2026.02 rev. 0
database_name = {database}
decoy_search = 0
num_threads = 4
print_ascorepro_score = -1
peptide_mass_tolerance_upper = 20.0
peptide_mass_tolerance_lower = -20.0
peptide_mass_units = 2
precursor_tolerance_type = 1
isotope_error = 0
search_enzyme_number = {enzyme}
search_enzyme2_number = 0
sample_enzyme_number = 0
num_enzyme_termini = 2
allowed_missed_cleavage = {missed_cleavage}
variable_mod03 = 0.0 X 0 3 -1 0 0 0.0
variable_mod04 = 0.0 X 0 3 -1 0 0 0.0
variable_mod05 = 0.0 X 0 3 -1 0 0 0.0
max_variable_mods_in_peptide = 4
require_variable_mod = 0
fragment_bin_tol = 0.02
fragment_bin_offset = 0.0
theoretical_fragment_ions = 0
use_A_ions = 0
use_B_ions = 1
use_C_ions = 0
use_X_ions = 0
use_Y_ions = 1
use_Z_ions = 0
use_Z1_ions = 0
use_NL_ions = 0
output_sqtfile = 0
output_txtfile = 1
output_pepxmlfile = 0
output_mzidentmlfile = 0
output_percolatorfile = 0
num_output_lines = 1
scan_range = 0 0
precursor_charge = 0 0
override_charge = 0
ms_level = 2
activation_method = ALL
digest_mass_range = {mass_low} 5000.0
peptide_length_range = {len_min} {len_max}
max_duplicate_proteins = -1
max_fragment_charge = 3
min_precursor_charge = 1
max_precursor_charge = 6
clip_nterm_methionine = 0
spectrum_batch_size = 15000
decoy_prefix = DECOY_
equal_I_and_L = {equal_IL}
mass_offsets =
minimum_peaks = 10
minimum_intensity = 0
remove_precursor_peak = 0
remove_precursor_tolerance = 1.5
clear_mz_range = 0.0 0.0
percentage_base_peak = 0.0
add_Cterm_peptide = 0.0
add_Nterm_peptide = 0.0
add_Cterm_protein = 0.0
add_Nterm_protein = 0.0
add_G_glycine = 0.0
add_A_alanine = 0.0
add_S_serine = 0.0
add_P_proline = 0.0
add_V_valine = 0.0
add_T_threonine = 0.0
add_C_cysteine = {static_C}
add_L_leucine = 0.0
add_I_isoleucine = 0.0
add_N_asparagine = 0.0
add_D_aspartic_acid = 0.0
add_Q_glutamine = 0.0
add_K_lysine = 0.0
add_E_glutamic_acid = 0.0
add_M_methionine = 0.0
add_H_histidine = 0.0
add_F_phenylalanine = 0.0
add_U_selenocysteine = 0.0
add_R_arginine = 0.0
add_Y_tyrosine = 0.0
add_W_tryptophan = 0.0
add_O_pyrrolysine = 0.0
add_B_user_amino_acid = 0.0
add_J_user_amino_acid = 0.0
add_X_user_amino_acid = 0.0
add_Z_user_amino_acid = 0.0
[COMET_ENZYME_INFO]
0.  Cut_everywhere         0      -           -
1.  Trypsin                1      KR          P
2.  Trypsin/P              1      KR          -
""")


# ---------------------------------------------------------------------------
# .idx reader (minimal — only what the tests need)
# ---------------------------------------------------------------------------

def parse_idx(path):
    """
    Returns dict: {peptide_seq -> {"mass", "prevAA", "nextAA", "proteins": list[str]}}
    """
    with open(path, "rb") as f:
        # skip text header
        while True:
            line = f.readline()
            if not line or line.startswith(b"RequireVariableMod:"):
                f.readline()   # blank line
                break

        # section offsets at file tail
        f.seek(-24, 2)
        pep_pos, prot_pos, perm_pos = struct.unpack("<qqq", f.read(24))

        # peptides
        f.seek(pep_pos)
        (num_pep,) = struct.unpack("<Q", f.read(8))
        buf = f.read(prot_pos - pep_pos - 8)
        p = 0
        peptides = []
        for _ in range(num_pep):
            (iLen,)  = struct.unpack_from("<i", buf, p);   p += 4
            seq      = buf[p:p+iLen].decode("ascii");      p += iLen
            prevAA   = chr(buf[p]);                        p += 1
            nextAA   = chr(buf[p]);                        p += 1
            (mass,)  = struct.unpack_from("<d", buf, p);   p += 8
            p += 2   # siVarMod (skip)
            (pidx,)  = struct.unpack_from("<q", buf, p);   p += 8
            peptides.append({"seq": seq, "mass": mass,
                             "prevAA": prevAA, "nextAA": nextAA, "pidx": pidx})

        # protein lists
        f.seek(prot_pos)
        prot_buf = f.read(perm_pos - prot_pos)
        pp = 0
        (num_lists,) = struct.unpack_from("<q", prot_buf, pp);  pp += 8
        prot_lists = []
        for _ in range(num_lists):
            (cnt,) = struct.unpack_from("<Q", prot_buf, pp);    pp += 8
            offsets = list(struct.unpack_from(f"<{cnt}q", prot_buf, pp));  pp += cnt*8
            prot_lists.append(offsets)

        # resolve protein names
        result = {}
        for pep in peptides:
            names = []
            for off in prot_lists[pep["pidx"]]:
                f.seek(off)
                raw = f.read(WIDTH_REFERENCE).rstrip(b"\x00").decode("ascii", errors="replace")
                names.append(raw)
            result[pep["seq"]] = {
                "mass":   pep["mass"],
                "prevAA": pep["prevAA"],
                "nextAA": pep["nextAA"],
                "proteins": names,
            }
    return result


# ---------------------------------------------------------------------------
# test runner helpers
# ---------------------------------------------------------------------------

def run_comet_index(comet_exe, fasta_path, params_kwargs):
    """Write a temp params file, run Comet -i, return path of generated .idx."""
    fasta_path = Path(fasta_path)
    idx_path = fasta_path.with_suffix(".fasta.idx")

    # params database_name must be the Windows path seen by Comet.exe
    # In WSL2, /mnt/c/... maps to C:\...
    def to_win(p):
        p = str(p)
        if p.startswith("/mnt/"):
            parts = p[5:].split("/", 1)
            drive = parts[0].upper() + ":"
            rest  = parts[1].replace("/", "\\") if len(parts) > 1 else ""
            return drive + "\\" + rest
        return p

    params_content = PARAMS_TEMPLATE.format(
        database=to_win(fasta_path),
        **params_kwargs,
    )

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(params_content)
        params_file = Path(pf.name)

    # remove old .idx so Comet regenerates it
    if idx_path.exists():
        idx_path.unlink()

    try:
        result = subprocess.run(
            [str(comet_exe), "-i", f"-P{to_win(params_file)}"],
            capture_output=True, text=True, timeout=120,
        )
        stdout = result.stdout + result.stderr
        if result.returncode != 0:
            raise RuntimeError(f"Comet exited {result.returncode}:\n{stdout}")
        if not idx_path.exists():
            raise RuntimeError(f".idx not created. Comet output:\n{stdout}")
    finally:
        params_file.unlink(missing_ok=True)

    return idx_path


def check(condition, msg, failures):
    if not condition:
        print(f"  FAIL: {msg}")
        failures.append(msg)
    else:
        print(f"  pass: {msg}")


# ---------------------------------------------------------------------------
# individual tests
# ---------------------------------------------------------------------------

TESTS = {}


def register(name):
    def decorator(fn):
        TESTS[name] = fn
        return fn
    return decorator


@register("t1")
def test_t1(comet_exe):
    """T1: Basic peptide generation — single short protein ACDEFGHIKL, length 8-10."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t1_basic.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 10, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    data = parse_idx(idx)
    expected_seqs = {
        "ACDEFGHI", "CDEFGHIK", "DEFGHIKL",   # length 8
        "ACDEFGHIK", "CDEFGHIKL",              # length 9
        "ACDEFGHIKL",                          # length 10
    }
    check(set(data.keys()) == expected_seqs,
          f"Expected {len(expected_seqs)} peptides {expected_seqs}, got {set(data.keys())}", failures)
    for seq in expected_seqs:
        check(seq in data, f"Peptide {seq} present", failures)
        if seq in data:
            check(len(data[seq]["proteins"]) == 1, f"{seq} has exactly 1 protein", failures)
    return failures


@register("t2")
def test_t2(comet_exe):
    """T2: Within-protein dedup — AAAKAAAKAAAK, length 8 only."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t2_repeat.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 8, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    data = parse_idx(idx)
    # 5 substrings of length 8 from AAAKAAAKAAAK but AAAKAAAK is duplicated
    # unique: AAAKAAAK, AAKAAAKA, AKAAAKAA, KAAAKAAА
    check(len(data) == 4, f"Expected 4 unique peptides (within-protein dedup), got {len(data)}: {sorted(data)}", failures)
    check("AAAKAAAK" in data, "AAAKAAAK deduplicated to single entry", failures)
    if "AAAKAAAK" in data:
        check(len(data["AAAKAAAK"]["proteins"]) == 1, "AAAKAAAK maps to 1 protein", failures)
    return failures


@register("t3")
def test_t3(comet_exe):
    """T3: Cross-protein dedup — two proteins with identical sequence ACDEFGHIKL, length 8-10."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t3_shared.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 10, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    data = parse_idx(idx)
    check(len(data) == 6, f"Expected 6 unique peptides, got {len(data)}", failures)
    for seq, entry in data.items():
        check(len(entry["proteins"]) == 2,
              f"{seq} maps to 2 proteins (got {len(entry['proteins'])})", failures)
    return failures


@register("t4_il_true")
def test_t4_il_true(comet_exe):
    """T4a: equal_I_and_L=1 — plain peptide index stores raw sequences unchanged.
    bTreatSameIL does NOT merge I/L variants in the plain peptide index; it is
    applied at fragment-index search time.  So PEPTIRDE and PEPTLRDE remain
    separate entries regardless of the equal_I_and_L setting."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t4_IL.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 13, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    data_il1 = parse_idx(idx)

    idx2 = run_comet_index(comet_exe, DATA_DIR / "t4_IL.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 13, "mass_low": 200.0,
        "equal_IL": 0, "static_C": 0.0,
    })
    data_il0 = parse_idx(idx2)

    check(len(data_il1) == len(data_il0),
          f"equal_I_and_L does not affect plain peptide count: il1={len(data_il1)} il0={len(data_il0)}", failures)
    check("PEPTIRDE" in data_il1, "PEPTIRDE present (equal_IL=1)", failures)
    check("PEPTLRDE" in data_il1, "PEPTLRDE present as separate entry (equal_IL=1)", failures)
    if "PEPTIRDE" in data_il1:
        check(len(data_il1["PEPTIRDE"]["proteins"]) == 1, "PEPTIRDE maps to 1 protein", failures)
    if "PEPTLRDE" in data_il1:
        check(len(data_il1["PEPTLRDE"]["proteins"]) == 1, "PEPTLRDE maps to 1 protein", failures)
    return failures


@register("t4_il_false")
def test_t4_il_false(comet_exe):
    """T4b: I/L treatment — bTreatSameIL=false keeps PEPTIRDE and PEPTLRDE separate."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t4_IL.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 13, "mass_low": 200.0,
        "equal_IL": 0, "static_C": 0.0,
    })
    data = parse_idx(idx)
    check("PEPTIRDE" in data, "PEPTIRDE present when equal_I_and_L=0", failures)
    check("PEPTLRDE" in data, "PEPTLRDE present when equal_I_and_L=0", failures)
    if "PEPTIRDE" in data:
        check(len(data["PEPTIRDE"]["proteins"]) == 1, "PEPTIRDE maps to 1 protein", failures)
    if "PEPTLRDE" in data:
        check(len(data["PEPTLRDE"]["proteins"]) == 1, "PEPTLRDE maps to 1 protein", failures)
    return failures


@register("t5_noenz")
def test_t5_noenz(comet_exe):
    """T5a: No-enzyme — all length-8 substrings of MAKRPEPTIDEKGASTMVR."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t5_enzyme.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 8, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    data = parse_idx(idx)
    protein = "MAKRPEPTIDEKGASTMVR"   # length 19
    expected = {protein[i:i+8] for i in range(len(protein) - 7)}  # 12 unique substrings of length 8
    check(set(data.keys()) == expected,
          f"No-enzyme: expected {len(expected)} length-8 peptides, got {len(data)}", failures)
    return failures


@register("t5_trypsin_0mc")
def test_t5_trypsin_0mc(comet_exe):
    """T5b: Trypsin, 0 missed cleavages — only RPEPTIDEK qualifies (length >= 8)."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t5_enzyme.fasta", {
        "enzyme": 1, "missed_cleavage": 0,
        "len_min": 8, "len_max": 25, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    data = parse_idx(idx)
    check("RPEPTIDEK" in data, "RPEPTIDEK present (tryptic, 9 AA)", failures)
    check("MAK" not in data, "MAK absent (too short, 3 AA)", failures)
    check("GASTMVR" not in data, "GASTMVR absent (too short, 7 AA)", failures)
    check(len(data) == 1, f"Exactly 1 tryptic peptide >= 8 AA with 0 MC; got {len(data)}: {sorted(data.keys())}", failures)
    return failures


@register("t5_trypsin_1mc")
def test_t5_trypsin_1mc(comet_exe):
    """T5c: Trypsin, 1 missed cleavage — RPEPTIDEK + two MC peptides."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t5_enzyme.fasta", {
        "enzyme": 1, "missed_cleavage": 1,
        "len_min": 8, "len_max": 25, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    data = parse_idx(idx)
    expected = {"RPEPTIDEK", "MAKRPEPTIDEK", "RPEPTIDEKGASTMVR"}
    check(set(data.keys()) == expected,
          f"Trypsin 1MC: expected {expected}, got {set(data.keys())}", failures)
    return failures


@register("t6")
def test_t6(comet_exe):
    """T6: cPrevAA/cNextAA — verify flanking AAs including '-' at protein termini."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t6_flanking.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 8, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    data = parse_idx(idx)
    protein = "ACDEFGHIKLMNPQ"  # length 14
    expected_flanking = {}
    for i in range(len(protein) - 7):
        seq = protein[i:i+8]
        prev = "-" if i == 0 else protein[i-1]
        nxt  = "-" if i + 8 == len(protein) else protein[i+8]
        expected_flanking[seq] = (prev, nxt)

    for seq, (exp_prev, exp_next) in expected_flanking.items():
        if seq in data:
            got_prev = data[seq]["prevAA"]
            got_next = data[seq]["nextAA"]
            check(got_prev == exp_prev,
                  f"{seq}: prevAA expected '{exp_prev}' got '{got_prev}'", failures)
            check(got_next == exp_next,
                  f"{seq}: nextAA expected '{exp_next}' got '{got_next}'", failures)
        else:
            check(False, f"{seq} missing from index", failures)
    return failures


@register("t7")
def test_t7(comet_exe):
    """T7: Mass accuracy — PEPTIDE embedded in AAAPEPTIDEAAA, no static mods."""
    # monoisotopic MH+ of PEPTIDE (no mods):
    # P=97.05276 E=129.04259 P=97.05276 T=101.04768 I=113.08406 D=115.02694 E=129.04259
    # sum residues = 781.34938; +H2O=18.01056; +H=1.00728 => 800.36722
    EXPECTED_MASS = 800.36722
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t7_mass.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 7, "len_max": 7, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    data = parse_idx(idx)
    check("PEPTIDE" in data, "PEPTIDE peptide present", failures)
    if "PEPTIDE" in data:
        delta = abs(data["PEPTIDE"]["mass"] - EXPECTED_MASS)
        check(delta < MASS_TOL,
              f"PEPTIDE mass {data['PEPTIDE']['mass']:.6f} within {MASS_TOL} Da of {EXPECTED_MASS} (delta={delta:.6f})", failures)
    return failures


@register("t11")
def test_t11(comet_exe):
    """T11: Edge case — protein ACDE (4 AA) too short for length >= 8.
    Comet exits with an informative error (no crash)."""
    failures = []
    try:
        run_comet_index(comet_exe, DATA_DIR / "t11_short.fasta", {
            "enzyme": 0, "missed_cleavage": 0,
            "len_min": 8, "len_max": 25, "mass_low": 200.0,
            "equal_IL": 1, "static_C": 0.0,
        })
        # If we get here with an empty .idx, that's also acceptable
        idx_path = (DATA_DIR / "t11_short.fasta").with_suffix(".fasta.idx")
        if idx_path.exists():
            data = parse_idx(idx_path)
            check(len(data) == 0, f"Expected 0 peptides, got {len(data)}", failures)
        print("  info: Comet succeeded with empty index")
    except RuntimeError as e:
        msg = str(e).lower()
        check("no peptides" in msg, f"Graceful error for empty database: {str(e)[:100]}", failures)
        print(f"  info: Comet exited with expected error (no crash)")
    return failures


@register("t12")
def test_t12(comet_exe):
    """T12: Edge case — protein ACDEFGHI (8 AA, exactly minimum length); expect 1 peptide."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t12_minlen.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 25, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    data = parse_idx(idx)
    check(len(data) == 1, f"Expected 1 peptide, got {len(data)}", failures)
    check("ACDEFGHI" in data, "ACDEFGHI present", failures)
    if "ACDEFGHI" in data:
        check(data["ACDEFGHI"]["prevAA"] == "-",
              f"prevAA should be '-', got '{data['ACDEFGHI']['prevAA']}'", failures)
        check(data["ACDEFGHI"]["nextAA"] == "-",
              f"nextAA should be '-', got '{data['ACDEFGHI']['nextAA']}'", failures)
    return failures


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--comet", default=str(COMET_EXE),
                        help="path to Comet.exe")
    parser.add_argument("tests", nargs="*", default=list(TESTS.keys()),
                        help="test IDs to run (default: all)")
    args = parser.parse_args()

    comet_exe = Path(args.comet)
    if not comet_exe.exists():
        print(f"ERROR: Comet executable not found: {comet_exe}", file=sys.stderr)
        sys.exit(2)

    requested = args.tests
    unknown = set(requested) - set(TESTS)
    if unknown:
        print(f"ERROR: Unknown test(s): {unknown}", file=sys.stderr)
        print(f"Available: {list(TESTS.keys())}", file=sys.stderr)
        sys.exit(2)

    total_fail = 0
    total_pass = 0

    for name in requested:
        print(f"\n{'='*60}")
        print(f"  {name}: {TESTS[name].__doc__.strip().splitlines()[0]}")
        print(f"{'='*60}")
        try:
            failures = TESTS[name](comet_exe)
        except Exception as e:
            print(f"  ERROR: {e}")
            failures = [str(e)]

        if failures:
            total_fail += 1
            print(f"  --> FAILED ({len(failures)} check(s))")
        else:
            total_pass += 1
            print(f"  --> PASSED")

    print(f"\n{'='*60}")
    print(f"  Results: {total_pass} passed, {total_fail} failed")
    print(f"{'='*60}")
    sys.exit(0 if total_fail == 0 else 1)


if __name__ == "__main__":
    main()
