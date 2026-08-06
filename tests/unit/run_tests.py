#!/usr/bin/env python3
"""
Comet plain-peptide index unit tests (T1-T7, T11-T16) and integration tests (T17-T18).

Runs Comet.exe -i on each crafted FASTA and verifies expected properties.

Usage:
    python run_tests.py [--comet PATH] [--integration] [--baseline PATH] [test_id ...]

    --comet       path to Comet binary (default: ../../comet.exe); repeatable
    --integration also run T17, T18, T22-T24 (require human.small.fasta and/or --bigdata)
    --baseline    path to a previous-Comet-version binary for T23/T24's cross-version
                  checks (default: tests/regression/baselines/v2025.03.0/comet,
                  auto-downloaded from GitHub Releases on first use if missing)
    test_id       one or more test IDs (default: all non-integration tests)

Exit code 0 = all tests passed; non-zero = failures.
"""

import argparse
import filecmp
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import textwrap
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import legacy_cases  # noqa: E402

sys.path.insert(0, str(Path(__file__).parent.parent / "rts_repro"))
import ms2_to_fixture  # noqa: E402

sys.path.insert(0, str(Path(__file__).parent.parent / "regression"))
import setup_baselines  # noqa: E402


UNIT_DIR      = Path(__file__).parent.resolve()
DATA_DIR      = UNIT_DIR / "data"
REPO_ROOT     = UNIT_DIR.parent.parent
REAL_DATA_DIR = REPO_ROOT / "data"

# Default binary paths
COMET_EXE            = REPO_ROOT / "comet.exe"

# Previous-version binary T23/T24 compare against (see _ensure_baseline()).
# setup_baselines.py already knows how to fetch this tag's release asset; reusing
# it here means run_tests.py always gets the same comet.linux.exe/win64.exe naming
# convention without duplicating the download logic.
BASELINE_TAG         = "v2025.03.0"
DEFAULT_BASELINE_EXE = setup_baselines.BASELINES_DIR / BASELINE_TAG / setup_baselines.asset_url(BASELINE_TAG)[1]

MASS_TOL        = 0.002   # Da -- loose tolerance for monoisotopic masses
WIDTH_REFERENCE = 512

# Set by main() before running integration tests
_RUN_INTEGRATION = False
_BASELINE_EXE    = str(DEFAULT_BASELINE_EXE)

# Tests gated behind --integration: they need large/manually-supplied data
# and/or take much longer than the T1-T16/T19-T21 unit tests.
INTEGRATION_TESTS = ("t17", "t18", "t22_rts_fi", "t22_rts_pi", "t23_decoy_modes", "t24_index_parity")

# Set by main() for T23/T24 (--bigdata)
_BIGDATA_DIR = str(REPO_ROOT.parent / "20130226-comet-tests")


# ---------------------------------------------------------------------------
# params template
# ---------------------------------------------------------------------------

PARAMS_TEMPLATE = textwrap.dedent("""\
# comet_version {comet_version}
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
# path helpers
# ---------------------------------------------------------------------------

def _to_win(p):
    """Convert /mnt/<drive>/... path to Drive:\\... for Windows binaries."""
    p = str(p)
    if p.startswith("/mnt/"):
        parts = p[5:].split("/", 1)
        drive = parts[0].upper() + ":"
        rest  = parts[1].replace("/", "\\") if len(parts) > 1 else ""
        return drive + "\\" + rest
    return p


def _binary_uses_win_paths(binary):
    """Return True if binary is a Windows PE (MZ magic) -- needs Windows-format paths."""
    try:
        with open(str(binary), "rb") as f:
            return f.read(2) == b"MZ"
    except Exception:
        return False


# ---------------------------------------------------------------------------
# .idx reader
# ---------------------------------------------------------------------------

def parse_idx(path):
    """
    Returns dict: {peptide_seq -> {"mass", "prevAA", "nextAA", "proteins": list[str]}}
    """
    with open(path, "rb") as f:
        while True:
            line = f.readline()
            if not line or line.startswith(b"RequireVariableMod:"):
                f.readline()   # blank line after header
                break

        # docs/20260730_PI_reduction.md Phase 0: footer grew from 3 pointers (peptides,
        # proteins, permutations) to 4 (+ variants, PI_DB-mode search data this FI_DB-focused
        # parser doesn't need) when PI_DB and FI_DB were unified onto one .idx format.
        f.seek(-32, 2)
        pep_pos, prot_pos, perm_pos, _variants_pos = struct.unpack("<qqqq", f.read(32))

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

        f.seek(prot_pos)
        prot_buf = f.read(perm_pos - prot_pos)
        pp = 0
        (num_lists,) = struct.unpack_from("<q", prot_buf, pp);  pp += 8
        prot_lists = []
        for _ in range(num_lists):
            (cnt,) = struct.unpack_from("<Q", prot_buf, pp);    pp += 8
            offsets = list(struct.unpack_from(f"<{cnt}q", prot_buf, pp));  pp += cnt*8
            prot_lists.append(offsets)

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

def run_comet_index(comet_exe, fasta_path, params_kwargs,
                    comet_version="2026.02 rev. 0"):
    """Write a temp params file, run Comet -i, return path of generated .idx."""
    fasta_path = Path(fasta_path)
    idx_path   = fasta_path.with_suffix(".fasta.idx")

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    params_content = PARAMS_TEMPLATE.format(
        database=fmt(fasta_path),
        comet_version=comet_version,
        **params_kwargs,
    )

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(params_content)
        params_file = Path(pf.name)

    if idx_path.exists():
        idx_path.unlink()

    try:
        result = subprocess.run(
            [str(comet_exe), "-i", f"-P{fmt(params_file)}"],
            capture_output=True, text=True, timeout=300,
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
    return bool(condition)


# ---------------------------------------------------------------------------
# test registry
# ---------------------------------------------------------------------------

TESTS = {}


def register(name):
    def decorator(fn):
        TESTS[name] = fn
        return fn
    return decorator


# ---------------------------------------------------------------------------
# T1 -- basic peptide generation
# ---------------------------------------------------------------------------

@register("t1")
def test_t1(comet_exe):
    """T1: Basic peptide generation -- single short protein ACDEFGHIKL, length 8-10."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t1_basic.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 10, "mass_low": 200.0,
        "equal_IL": 0, "static_C": 0.0,
    })
    data = parse_idx(idx)
    expected_seqs = {
        "ACDEFGHI", "CDEFGHIK", "DEFGHIKL",   # length 8
        "ACDEFGHIK", "CDEFGHIKL",              # length 9
        "ACDEFGHIKL",                          # length 10
    }
    check(set(data.keys()) == expected_seqs,
          f"Expected {len(expected_seqs)} peptides, got {set(data.keys())}", failures)
    for seq in expected_seqs:
        if seq in data:
            check(len(data[seq]["proteins"]) == 1, f"{seq} has exactly 1 protein", failures)
    return failures


# ---------------------------------------------------------------------------
# T2 -- within-protein deduplication
# ---------------------------------------------------------------------------

@register("t2")
def test_t2(comet_exe):
    """T2: Within-protein dedup -- AAAKAAAKAAAK, length 8 only."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t2_repeat.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 8, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    data = parse_idx(idx)
    # 5 substrings of length 8 from AAAKAAAKAAAK but AAAKAAAK is duplicated
    # unique: AAAKAAAK, AAKAAAKA, AKAAAKAA, KAAAKAAA
    check(len(data) == 4, f"Expected 4 unique peptides (within-protein dedup), got {len(data)}: {sorted(data)}", failures)
    check("AAAKAAAK" in data, "AAAKAAAK deduplicated to single entry", failures)
    if "AAAKAAAK" in data:
        check(len(data["AAAKAAAK"]["proteins"]) == 1, "AAAKAAAK maps to 1 protein", failures)
    return failures


# ---------------------------------------------------------------------------
# T3 -- cross-protein deduplication
# ---------------------------------------------------------------------------

@register("t3")
def test_t3(comet_exe):
    """T3: Cross-protein dedup -- two proteins with identical sequence ACDEFGHIKL, length 8-10."""
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


# ---------------------------------------------------------------------------
# T4 -- I/L treatment (existing tests)
# ---------------------------------------------------------------------------

@register("t4_il_true")
def test_t4_il_true(comet_exe):
    """T4a: equal_I_and_L=1 -- PEPTIRDE and PEPTLRDE merge into one entry."""
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

    check(len(data_il1) < len(data_il0),
          f"equal_I_and_L=1 reduces count (merges I/L): il1={len(data_il1)} il0={len(data_il0)}", failures)
    check("PEPTIRDE" in data_il1, "PEPTIRDE present (canonical I-form, equal_IL=1)", failures)
    check("PEPTLRDE" not in data_il1, "PEPTLRDE absent (merged into PEPTIRDE)", failures)
    if "PEPTIRDE" in data_il1:
        check(len(data_il1["PEPTIRDE"]["proteins"]) == 2, "PEPTIRDE maps to 2 proteins", failures)
    return failures


@register("t4_il_false")
def test_t4_il_false(comet_exe):
    """T4b: equal_I_and_L=0 -- PEPTIRDE and PEPTLRDE are distinct entries."""
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


# ---------------------------------------------------------------------------
# T5 -- enzyme constraints
# ---------------------------------------------------------------------------

@register("t5_noenz")
def test_t5_noenz(comet_exe):
    """T5a: No-enzyme -- all length-8 substrings of MAKRPEPTIDEKGASTMVR."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t5_enzyme.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 8, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    data = parse_idx(idx)
    protein = "MAKRPEPTIDEKGASTMVR"
    expected = {protein[i:i+8] for i in range(len(protein) - 7)}
    check(set(data.keys()) == expected,
          f"No-enzyme: expected {len(expected)} length-8 peptides, got {len(data)}", failures)
    return failures


@register("t5_trypsin_0mc")
def test_t5_trypsin_0mc(comet_exe):
    """T5b: Trypsin, 0 missed cleavages -- only RPEPTIDEK qualifies (length >= 8)."""
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
    """T5c: Trypsin, 1 missed cleavage -- RPEPTIDEK plus two MC peptides."""
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


# ---------------------------------------------------------------------------
# T6 -- flanking AAs
# ---------------------------------------------------------------------------

@register("t6")
def test_t6(comet_exe):
    """T6: cPrevAA/cNextAA -- verify flanking AAs including '-' at protein termini."""
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t6_flanking.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 8, "mass_low": 200.0,
        "equal_IL": 0, "static_C": 0.0,
    })
    data = parse_idx(idx)
    protein = "ACDEFGHIKLMNPQ"
    expected_flanking = {}
    for i in range(len(protein) - 7):
        seq  = protein[i:i+8]
        prev = "-" if i == 0 else protein[i-1]
        nxt  = "-" if i + 8 == len(protein) else protein[i+8]
        expected_flanking[seq] = (prev, nxt)

    for seq, (exp_prev, exp_next) in expected_flanking.items():
        if seq in data:
            check(data[seq]["prevAA"] == exp_prev,
                  f"{seq}: prevAA expected '{exp_prev}' got '{data[seq]['prevAA']}'", failures)
            check(data[seq]["nextAA"] == exp_next,
                  f"{seq}: nextAA expected '{exp_next}' got '{data[seq]['nextAA']}'", failures)
        else:
            check(False, f"{seq} missing from index", failures)
    return failures


# ---------------------------------------------------------------------------
# T7 -- mass accuracy
# ---------------------------------------------------------------------------

@register("t7")
def test_t7(comet_exe):
    """T7: Mass accuracy -- PEPTIDE embedded in AAAPEPTIDEAAA, no static mods."""
    # monoisotopic MH+ of PEPTIDE: P+E+P+T+I+D+E residues + H2O + H = 800.36722 Da
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


# ---------------------------------------------------------------------------
# T11 -- protein too short
# ---------------------------------------------------------------------------

@register("t11")
def test_t11(comet_exe):
    """T11: Edge case -- protein ACDE (4 AA) too short for length >= 8; no crash."""
    failures = []
    try:
        run_comet_index(comet_exe, DATA_DIR / "t11_short.fasta", {
            "enzyme": 0, "missed_cleavage": 0,
            "len_min": 8, "len_max": 25, "mass_low": 200.0,
            "equal_IL": 1, "static_C": 0.0,
        })
        idx_path = (DATA_DIR / "t11_short.fasta").with_suffix(".fasta.idx")
        if idx_path.exists():
            data = parse_idx(idx_path)
            check(len(data) == 0, f"Expected 0 peptides, got {len(data)}", failures)
        print("  info: Comet succeeded with empty index")
    except RuntimeError as e:
        msg = str(e).lower()
        check("no peptides" in msg, f"Graceful error for empty database: {str(e)[:100]}", failures)
        print("  info: Comet exited with expected error (no crash)")
    return failures


# ---------------------------------------------------------------------------
# T12 -- exact minimum length
# ---------------------------------------------------------------------------

@register("t12")
def test_t12(comet_exe):
    """T12: Edge case -- protein ACDEFGHI (8 AA, exactly minimum length); 1 peptide."""
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
# T13 -- 5-bit encoding round-trip (pure Python, no Comet run)
# ---------------------------------------------------------------------------

@register("t13")
def test_t13(comet_exe):
    """T13: PackPeptide/UnpackPeptide round-trips (pure Python, no Comet invocation)."""
    import random as _random

    # Replicate C++ encoding from CometDataInternal.h
    _AAS = "ACDEFGHIKLMNPQRSTVWY"   # 20 standard AAs, alphabetical order -> codes 1-20
    _AA_CODE = {c: i + 1 for i, c in enumerate(_AAS)}    # A->1, C->2, ..., Y->20
    _CODE_AA = ["\0"] + list(_AAS) + ["\0"] * (32 - len(_AAS) - 1)  # code->char

    def pack(seq, bIL):
        key = 0
        for i, c in enumerate(seq):
            if bIL and c == "L":
                c = "I"
            key |= _AA_CODE.get(c, 0) << (55 - i * 5)
        return key

    def unpack(key, iLen):
        return "".join(_CODE_AA[(key >> (55 - i * 5)) & 0x1F] for i in range(iLen))

    failures = []
    _AAS_NO_L = _AAS.replace("L", "")

    # 1. Round-trip all 20 AAs x lengths 8-12, bIL=False
    for iLen in range(8, 13):
        for c in _AAS_NO_L:
            seq = c * iLen
            got = unpack(pack(seq, False), iLen)
            check(got == seq, f"Round-trip '{seq[:3]}...' len={iLen} bIL=False", failures)

    # 2. L round-trips cleanly when bIL=False
    for iLen in range(8, 13):
        seq = "L" * iLen
        got = unpack(pack(seq, False), iLen)
        check(got == seq, f"L-only seq len={iLen} round-trips when bIL=False", failures)

    # 3. I and L produce the same key when bIL=True
    for iLen in range(8, 13):
        ki = pack("I" * iLen, True)
        kl = pack("L" * iLen, True)
        check(ki == kl, f"I*{iLen} and L*{iLen} give same key when bIL=True", failures)

    # 4. I and L produce different keys when bIL=False
    for iLen in range(8, 13):
        ki = pack("I" * iLen, False)
        kl = pack("L" * iLen, False)
        check(ki != kl, f"I*{iLen} and L*{iLen} differ when bIL=False", failures)

    # 5. L encodes to canonical I-form when bIL=True
    for iLen in range(8, 13):
        got = unpack(pack("L" * iLen, True), iLen)
        check(got == "I" * iLen,
              f"L*{iLen} bIL=True round-trips to I (canonical): got {got[:3]}...", failures)

    # 6. Integer sort order matches lexicographic order within each length
    _random.seed(42)
    for iLen in range(8, 13):
        seqs = ["".join(_random.choice(_AAS_NO_L) for _ in range(iLen)) for _ in range(200)]
        lex_order = sorted(seqs)
        int_order = sorted(seqs, key=lambda s: pack(s, False))
        check(lex_order == int_order,
              f"Integer sort matches lex sort for length {iLen}", failures)

    # 7. Known fixed value: ACDEFGHI len=8, bIL=False
    # A=1@bit55, C=2@bit50, D=3@bit45, E=4@bit40, F=5@bit35, G=6@bit30, H=7@bit25, I=8@bit20
    expected_key = ((1 << 55) | (2 << 50) | (3 << 45) | (4 << 40) |
                    (5 << 35) | (6 << 30) | (7 << 25) | (8 << 20))
    computed_key = pack("ACDEFGHI", False)
    check(computed_key == expected_key,
          f"ACDEFGHI known-value: computed={hex(computed_key)} expected={hex(expected_key)}", failures)

    return failures


# ---------------------------------------------------------------------------
# T14 -- boundary length 12/13
# ---------------------------------------------------------------------------

@register("t14")
def test_t14(comet_exe):
    """T14: Boundary len-12/13 -- ACDEFGHIKLMNPQ (14 AA), no-enzyme, len 12-13."""
    # length-12 windows: ACDEFGHIKLMN, CDEFGHIKLMNP, DEFGHIKLMNPQ  (3)
    # length-13 windows: ACDEFGHIKLMNP, CDEFGHIKLMNPQ               (2)
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t14_boundary.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 12, "len_max": 13, "mass_low": 200.0,
        "equal_IL": 0, "static_C": 0.0,
    })
    data = parse_idx(idx)
    protein = "ACDEFGHIKLMNPQ"
    expected_12 = {protein[i:i+12] for i in range(len(protein) - 11)}
    expected_13 = {protein[i:i+13] for i in range(len(protein) - 12)}
    expected = expected_12 | expected_13
    check(len(data) == 5,
          f"Expected 5 peptides (3 len-12 + 2 len-13), got {len(data)}: {sorted(data)}", failures)
    for seq in expected:
        check(seq in data, f"Expected peptide {seq!r} present", failures)
        if seq in data:
            check(len(data[seq]["proteins"]) == 1, f"{seq!r} maps to 1 protein", failures)
    return failures


# ---------------------------------------------------------------------------
# T15 -- I/L canonicalization: short path (len 8)
# ---------------------------------------------------------------------------

@register("t15_il_short")
def test_t15_il_short(comet_exe):
    """T15a: I/L short path (len 8) -- equal_IL=1 merges ACDEFGHI+ACDEFGHL, =0 separates."""
    failures = []

    # equal_IL = 1: both collapse to one I-form entry mapping to 2 proteins
    idx1 = run_comet_index(comet_exe, DATA_DIR / "t15_IL_short.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 8, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    d1 = parse_idx(idx1)
    check(len(d1) == 1,
          f"equal_IL=1: expected 1 merged entry, got {len(d1)}: {sorted(d1)}", failures)
    check("ACDEFGHI" in d1,
          "equal_IL=1: I-form 'ACDEFGHI' stored (first-in-file canonical)", failures)
    check("ACDEFGHL" not in d1,
          "equal_IL=1: L-form 'ACDEFGHL' absent (merged)", failures)
    if "ACDEFGHI" in d1:
        check(len(d1["ACDEFGHI"]["proteins"]) == 2,
              f"equal_IL=1: ACDEFGHI maps to 2 proteins; got {len(d1['ACDEFGHI']['proteins'])}", failures)

    # equal_IL = 0: two distinct entries, each with 1 protein
    idx0 = run_comet_index(comet_exe, DATA_DIR / "t15_IL_short.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 8, "mass_low": 200.0,
        "equal_IL": 0, "static_C": 0.0,
    })
    d0 = parse_idx(idx0)
    check(len(d0) == 2,
          f"equal_IL=0: expected 2 distinct entries, got {len(d0)}: {sorted(d0)}", failures)
    for seq in ("ACDEFGHI", "ACDEFGHL"):
        check(seq in d0, f"equal_IL=0: {seq!r} present", failures)
        if seq in d0:
            check(len(d0[seq]["proteins"]) == 1,
                  f"equal_IL=0: {seq!r} maps to 1 protein", failures)

    return failures


# ---------------------------------------------------------------------------
# T15b -- I/L canonicalization: long path (len 13)
# ---------------------------------------------------------------------------

@register("t15_il_long")
def test_t15_il_long(comet_exe):
    """T15b: I/L long path (len 13) -- equal_IL=1 merges ACDEFGHKMNPQI+ACDEFGHKMNPQL, =0 separates."""
    # Proteins: ACDEFGHKMNPQI (I at pos 12) and ACDEFGHKMNPQL (L at pos 12).
    # No I or L elsewhere -- only pos-12 differs.
    failures = []

    # equal_IL = 1: L->I canonical, both collapse to I-form with 2 proteins
    idx1 = run_comet_index(comet_exe, DATA_DIR / "t15_IL_long.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 13, "len_max": 13, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    })
    d1 = parse_idx(idx1)
    check(len(d1) == 1,
          f"equal_IL=1: expected 1 merged entry, got {len(d1)}: {sorted(d1)}", failures)
    check("ACDEFGHKMNPQI" in d1,
          "equal_IL=1: I-form stored (first-in-file)", failures)
    check("ACDEFGHKMNPQL" not in d1,
          "equal_IL=1: L-form absent (merged)", failures)
    if "ACDEFGHKMNPQI" in d1:
        check(len(d1["ACDEFGHKMNPQI"]["proteins"]) == 2,
              f"equal_IL=1: maps to 2 proteins; got {len(d1['ACDEFGHKMNPQI']['proteins'])}", failures)

    # equal_IL = 0: two distinct entries, each with 1 protein
    idx0 = run_comet_index(comet_exe, DATA_DIR / "t15_IL_long.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 13, "len_max": 13, "mass_low": 200.0,
        "equal_IL": 0, "static_C": 0.0,
    })
    d0 = parse_idx(idx0)
    check(len(d0) == 2,
          f"equal_IL=0: expected 2 distinct entries, got {len(d0)}: {sorted(d0)}", failures)
    for seq in ("ACDEFGHKMNPQI", "ACDEFGHKMNPQL"):
        check(seq in d0, f"equal_IL=0: {seq!r} present", failures)
        if seq in d0:
            check(len(d0[seq]["proteins"]) == 1,
                  f"equal_IL=0: {seq!r} maps to 1 protein", failures)

    return failures


# ---------------------------------------------------------------------------
# T16 -- cross-path protein list correctness
# ---------------------------------------------------------------------------

@register("t16")
def test_t16(comet_exe):
    """T16: Cross-path protein list -- two identical 13-AA proteins, len 8-13."""
    # Protein ACDEFGHIKLMNA (13 AA); 6+5+4+3+2+1 = 21 unique substrings of lengths 8-13.
    # Both proteins are identical -> every entry must map to both proteins.
    # Lengths 8-12 go through the short (uint64) path; length 13 through the long (char[]) path.
    failures = []
    idx = run_comet_index(comet_exe, DATA_DIR / "t16_crosspath.fasta", {
        "enzyme": 0, "missed_cleavage": 0,
        "len_min": 8, "len_max": 13, "mass_low": 200.0,
        "equal_IL": 0, "static_C": 0.0,
    })
    data = parse_idx(idx)

    protein = "ACDEFGHIKLMNA"
    expected = set()
    for iLen in range(8, 14):
        for i in range(len(protein) - iLen + 1):
            expected.add(protein[i:i+iLen])

    check(len(data) == len(expected),
          f"Expected {len(expected)} unique peptides, got {len(data)}", failures)

    for seq in expected:
        if seq in data:
            n = len(data[seq]["proteins"])
            path = "short" if len(seq) <= 12 else "long"
            check(n == 2,
                  f"{seq!r} ({path} path, len={len(seq)}) maps to 2 proteins; got {n}", failures)
        else:
            check(False, f"Expected peptide {seq!r} missing from index", failures)

    return failures


# ---------------------------------------------------------------------------
# T17 -- integration build sanity check (human.small.fasta)
# ---------------------------------------------------------------------------
#
# Cross-version byte-comparison is not reliable: v2026.01.1 used a single
# flat-sort approach while the current binary uses per-length stratification
# with I/L canonical dedup in the long path -- producing a slightly different
# (and more correct) count.  Instead, T17 verifies that the build succeeds
# and the peptide count is within the expected range observed for this dataset.
#
# Expected count (no-enzyme, len 8-13, equal_IL=1, human.small.fasta):
#   current binary (stratified + canonical I/L dedup): 8,929,331
#   Acceptable range: 8,800,000 -- 9,100,000 (+-1.5% of expected)
#
# PSM equivalence (trypsin, HeLa run) vs v2026.01.1 is already validated by
# the regression suite (1522/1522 agreement, see docs/20260513_FI_PlainPeptideIdx.md).

@register("t17")
def test_t17(comet_exe):
    """T17 [integration]: Build human.small.fasta (no-enzyme len 8-13) and verify peptide count."""
    if not _RUN_INTEGRATION:
        print("  SKIP: pass --integration to run this test")
        return []

    failures = []
    small_fasta = REAL_DATA_DIR / "human.small.fasta"

    if not small_fasta.exists():
        print(f"  SKIP: {small_fasta} not found")
        return []

    sys.path.insert(0, str(UNIT_DIR))
    import compare_idx as _cmp   # noqa: PLC0415

    kwargs = {
        "enzyme": 0, "missed_cleavage": 2,
        "len_min": 8, "len_max": 13, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    }

    print("  Building index with current binary ...")
    idx = run_comet_index(comet_exe, small_fasta, kwargs)

    fo, num_pep, num_lists, *_ = _cmp._open_idx(str(idx))
    fo.close()
    print(f"  Peptide count: {num_pep:,}")

    lo, hi = 8_800_000, 9_100_000
    in_range = lo <= num_pep <= hi
    check(in_range,
          f"peptide count {num_pep:,} {'in' if in_range else 'OUTSIDE'} expected range [{lo:,}, {hi:,}]",
          failures)
    check(num_pep == num_lists,
          f"peptide count ({num_pep:,}) {'==' if num_pep == num_lists else '!='} protein-list count ({num_lists:,})",
          failures)

    return failures


# ---------------------------------------------------------------------------
# T18 -- determinism (integration)
# ---------------------------------------------------------------------------

@register("t18")
def test_t18(comet_exe):
    """T18 [integration]: Two stratified builds of human.small.fasta are byte-identical (no-enzyme len 8-13)."""
    if not _RUN_INTEGRATION:
        print("  SKIP: pass --integration to run this test")
        return []

    failures = []
    small_fasta = REAL_DATA_DIR / "human.small.fasta"
    if not small_fasta.exists():
        print(f"  SKIP: {small_fasta} not found")
        return []

    kwargs = {
        "enzyme": 0, "missed_cleavage": 2,
        "len_min": 8, "len_max": 13, "mass_low": 200.0,
        "equal_IL": 1, "static_C": 0.0,
    }

    print("  Building index (run 1) ...")
    idx1 = run_comet_index(comet_exe, small_fasta, kwargs)

    tmp_fd, tmp_path_str = tempfile.mkstemp(suffix=".run1.idx")
    os.close(tmp_fd)
    tmp_path = Path(tmp_path_str)
    shutil.copy2(idx1, tmp_path)

    try:
        print("  Building index (run 2) ...")
        idx2 = run_comet_index(comet_exe, small_fasta, kwargs)

        same = filecmp.cmp(str(tmp_path), str(idx2), shallow=False)
        check(same, "Two builds produce byte-identical .idx files", failures)
    finally:
        tmp_path.unlink(missing_ok=True)

    return failures


# ---------------------------------------------------------------------------
# T19 -- AScore + FI_DB regression (docs/20260617_codereview3.md issue 2a)
# ---------------------------------------------------------------------------
#
# CometSearchManager::SetAScoreOptions() reads g_staticParams.variableModParameters.
# varModList[] to configure AScorePro's differential-mod list. For an FI_DB (.idx)
# search, FiStrategy::initialize() loads the index and overwrites that same
# varModList[] from the .idx file's "VariableMod:" header line. If AScore were
# configured *before* that overwrite, it would be left with whatever (possibly blank)
# variable_mod01 the search-time params declared instead of the index's actual mod --
# see the ordering comment in CometSearch/search/Pipeline.cpp. This test builds an
# FI_DB index with a real variable mod, then searches it with print_ascorepro_score
# enabled but a deliberately blank variable_mod01 in the search-time params (the
# common real-world case, since FI_DB search params don't need to redeclare mods
# already baked into the index), and checks that AScorePro actually ran rather than
# being silently skipped.
#
# Fixture peptide: ACDEFGS[+79.966331]K (charge 2+), the only candidate in the index
# within the configured mass range, with a single phospho-acceptor S so localization
# is unambiguous. tests/unit/data/t19_ascore_fidb.ms2 contains the matching singly
# charged b/y ions, precomputed from monoisotopic residue masses.

T19_PARAMS_TEMPLATE = textwrap.dedent("""\
# comet_version {comet_version}
database_name = {database}
decoy_search = 0
num_threads = 4
print_ascorepro_score = {ascorepro}
peptide_mass_tolerance_upper = 20.0
peptide_mass_tolerance_lower = -20.0
peptide_mass_units = 2
precursor_tolerance_type = 1
isotope_error = 0
search_enzyme_number = 0
search_enzyme2_number = 0
sample_enzyme_number = 0
num_enzyme_termini = 2
allowed_missed_cleavage = 0
variable_mod01 = {mod1}
variable_mod02 = 0.0 X 0 3 -1 0 0 0.0
variable_mod03 = 0.0 X 0 3 -1 0 0 0.0
variable_mod04 = 0.0 X 0 3 -1 0 0 0.0
variable_mod05 = 0.0 X 0 3 -1 0 0 0.0
max_variable_mods_in_peptide = 1
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
digest_mass_range = 200.0 2000.0
peptide_length_range = 8 8
max_duplicate_proteins = -1
max_fragment_charge = 3
min_precursor_charge = 1
max_precursor_charge = 6
clip_nterm_methionine = 0
spectrum_batch_size = 15000
decoy_prefix = DECOY_
equal_I_and_L = 0
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
add_C_cysteine = 0.0
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


def _run_t19_step(comet_exe, args, timeout=120):
    """Run comet_exe with args, return (returncode, combined stdout+stderr)."""
    result = subprocess.run(
        [str(comet_exe)] + args, capture_output=True, text=True, timeout=timeout,
    )
    return result.returncode, result.stdout + result.stderr


@register("t19")
def test_t19(comet_exe):
    """T19: AScore + FI_DB regression -- AScore must use the .idx file's variable mod,
    not the search-time params' (blank) mod, for FI_DB searches."""
    failures = []

    fasta = DATA_DIR / "t19_ascore_fidb.fasta"
    ms2   = DATA_DIR / "t19_ascore_fidb.ms2"
    idx   = fasta.with_suffix(".fasta.idx")
    txt   = ms2.with_suffix(".txt")

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    # Step 1: build an FI_DB index with a real phospho-S mod baked into its header.
    if idx.exists():
        idx.unlink()

    build_params = T19_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(fasta),
        ascorepro=0, mod1="79.966331 S 0 1 -1 0 0 0.0",
    )
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(build_params)
        build_params_file = Path(pf.name)

    try:
        rc, out = _run_t19_step(comet_exe, ["-i", f"-P{fmt(build_params_file)}"])
        if rc != 0 or not idx.exists():
            failures.append(f"index build failed (rc={rc}):\n{out}")
            return failures
    finally:
        build_params_file.unlink(missing_ok=True)

    # Step 2: search the index with print_ascorepro_score enabled but a blank
    # variable_mod01 in the search-time params.
    if txt.exists():
        txt.unlink()

    search_params = T19_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(idx),
        ascorepro=1, mod1="0.0 X 0 3 -1 0 0 0.0",
    )
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(search_params)
        search_params_file = Path(pf.name)

    try:
        rc, out = _run_t19_step(comet_exe, [f"-P{fmt(search_params_file)}", fmt(ms2)])
        if rc != 0:
            failures.append(f"search failed (rc={rc}):\n{out}")
            return failures
        if not txt.exists():
            failures.append(f".txt not created. Comet output:\n{out}")
            return failures

        lines  = txt.read_text().splitlines()
        header = lines[1].split("\t")             # line 0 is the CometVersion/.../database line
        rows   = [l.split("\t") for l in lines[2:] if l.strip()]

        check(len(rows) == 1, f"expected exactly 1 PSM row, got {len(rows)}", failures)
        if not rows:
            return failures

        row = dict(zip(header, rows[0]))

        check(row.get("plain_peptide") == "ACDEFGSK",
              f"plain_peptide: expected ACDEFGSK, got {row.get('plain_peptide')!r}", failures)
        check("7_V_79.966331" in row.get("modifications", ""),
              f"modifications: expected to contain 7_V_79.966331, got "
              f"{row.get('modifications')!r}", failures)

        ascorepro = float(row.get("ascorepro", "0") or "0")
        check(ascorepro > 0.0,
              f"ascorepro: expected > 0 (AScore must run using the .idx file's mod, "
              f"not the search-time params' blank mod), got {ascorepro}", failures)
    finally:
        search_params_file.unlink(missing_ok=True)
        idx.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)

    return failures


# ---------------------------------------------------------------------------
# T20 -- PI_DB batch search regression (_pQueries never assigned)
# ---------------------------------------------------------------------------
#
# CometSearch::BinarySearchMass() and AnalyzePeptideIndex() read the query list
# through the _pQueries member (mirroring CometSearch::DoSearch(), the FASTA path,
# which sets _pQueries = &queries at entry) rather than through a parameter. The
# batch PI_DB path, CometSearch::SearchPeptideIndex(ThreadPool*, vector<Query*>&),
# never set _pQueries, so it stayed nullptr on the freshly constructed CometSearch
# instance RunSearch() uses for PI_DB, and the first dereference inside
# BinarySearchMass() segfaulted -- silently, with only the "- searching ..." progress
# message printed and no error text, exactly as reported against the VS-built
# Windows binary. This test reuses T19's phospho fixture but builds a PI_DB (plain
# peptide) index instead of an FI_DB (fragment ion) index, to cover the code path
# that crashed.

@register("t20")
def test_t20(comet_exe):
    """T20: PI_DB batch search regression -- a peptide-index (-j) search must
    complete and score correctly, not crash on the first scored candidate."""
    failures = []

    fasta = DATA_DIR / "t19_ascore_fidb.fasta"
    ms2   = DATA_DIR / "t19_ascore_fidb.ms2"
    idx   = fasta.with_suffix(".fasta.idx")
    txt   = ms2.with_suffix(".txt")

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    # Step 1: build a PI_DB (peptide index) with a real phospho-S mod baked into
    # its header. "-j" selects create_peptide_index, unlike T19's "-i"
    # (create_fragment_index).
    if idx.exists():
        idx.unlink()

    build_params = T19_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(fasta),
        ascorepro=0, mod1="79.966331 S 0 1 -1 0 0 0.0",
    )
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(build_params)
        build_params_file = Path(pf.name)

    try:
        rc, out = _run_t19_step(comet_exe, ["-j", f"-P{fmt(build_params_file)}"])
        if rc != 0 or not idx.exists():
            failures.append(f"index build failed (rc={rc}):\n{out}")
            return failures
    finally:
        build_params_file.unlink(missing_ok=True)

    # Step 2: search the PI_DB index. This is the call sequence that previously
    # segfaulted inside CometSearch::BinarySearchMass() before any output was
    # written, so a non-crashing exit with the expected PSM is the regression check.
    if txt.exists():
        txt.unlink()

    search_params = T19_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(idx),
        ascorepro=1, mod1="0.0 X 0 3 -1 0 0 0.0",
    )
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(search_params)
        search_params_file = Path(pf.name)

    try:
        rc, out = _run_t19_step(comet_exe, [f"-P{fmt(search_params_file)}", fmt(ms2)])
        if rc != 0:
            failures.append(f"search exited rc={rc} (expected 0, i.e. no crash):\n{out}")
            return failures
        check(True, "search exited cleanly (rc=0)", failures)
        if not txt.exists():
            failures.append(f".txt not created. Comet output:\n{out}")
            return failures

        lines  = txt.read_text().splitlines()
        header = lines[1].split("\t")             # line 0 is the CometVersion/.../database line
        rows   = [l.split("\t") for l in lines[2:] if l.strip()]

        check(len(rows) == 1, f"expected exactly 1 PSM row, got {len(rows)}", failures)
        if not rows:
            return failures

        row = dict(zip(header, rows[0]))

        check(row.get("plain_peptide") == "ACDEFGSK",
              f"plain_peptide: expected ACDEFGSK, got {row.get('plain_peptide')!r}", failures)
        check("7_V_79.966331" in row.get("modifications", ""),
              f"modifications: expected to contain 7_V_79.966331, got "
              f"{row.get('modifications')!r}", failures)

        ascorepro = float(row.get("ascorepro", "0") or "0")
        check(ascorepro > 0.0,
              f"ascorepro: expected > 0, got {ascorepro}", failures)
    finally:
        search_params_file.unlink(missing_ok=True)
        idx.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)

    return failures


# ---------------------------------------------------------------------------
# T21 -- legacy functional-correctness cases (from 20130226-comet-tests/runall.sh)
# ---------------------------------------------------------------------------

def _legacy_case_dir(case):
    return legacy_cases.LEGACY_DIR / case["dir"]


def _legacy_write_params(case_dir, params_kwargs, fmt, database_path):
    params_content = legacy_cases.build_params(database=fmt(database_path), **params_kwargs)
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(params_content)
        return Path(pf.name)


def _legacy_run(comet_exe, case, extra_args=(), params_override=None):
    """Run one legacy case's search. Returns (rows, stdout+stderr)."""
    case_dir = _legacy_case_dir(case)
    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    if case["database"].startswith("../"):
        database_path = legacy_cases.LEGACY_DIR / case["database"][3:]
    else:
        database_path = case_dir / case["database"]

    ms2_path = case_dir / case["ms2"]
    txt_path = ms2_path.with_suffix(".txt")
    if txt_path.exists():
        txt_path.unlink()

    kwargs = dict(case["params"])
    if params_override:
        kwargs.update(params_override)
    params_file = _legacy_write_params(case_dir, kwargs, fmt, database_path)

    args = [f"-P{fmt(params_file)}"]
    for a in case.get("extra_args", []):
        args.append(a.format(tmp1_db=fmt(case_dir / "tmp1.db")))
    args += list(extra_args)
    args.append(fmt(ms2_path))

    try:
        rc, out = _run_t19_step(comet_exe, args)
        rows = legacy_cases.parse_txt(txt_path) if txt_path.exists() else []
        return rc, rows, out
    finally:
        params_file.unlink(missing_ok=True)
        txt_path.unlink(missing_ok=True)


def _make_legacy_test(name, case):
    def test_fn(comet_exe):
        failures = []

        if name in ("fragmentNL", "fragmentNL2"):
            rc0, rows0, out0 = _legacy_run(comet_exe, case)
            if not check(rc0 == 0, f"base search exited 0 (rc={rc0})", failures):
                return failures
            if not check(len(rows0) >= 1, "base search: at least 1 PSM row", failures):
                return failures

            nl_idx = case["nl_mod_index"]
            nl_mods = list(case["params"]["mods"])
            base_mod = nl_mods[nl_idx].split()
            base_mod[-1] = str(case["nl_value"])
            nl_mods[nl_idx] = " ".join(base_mod)
            rc1, rows1, out1 = _legacy_run(comet_exe, case, params_override={"mods": tuple(nl_mods)})
            if not check(rc1 == 0, f"NL search exited 0 (rc={rc1})", failures):
                return failures
            if not check(len(rows1) >= 1, "NL search: at least 1 PSM row", failures):
                return failures

            xcorr0 = float(rows0[0]["xcorr"])
            xcorr1 = float(rows1[0]["xcorr"])
            check(xcorr1 > xcorr0,
                  f"xcorr(NL={case['nl_value']})={xcorr1} > xcorr(base)={xcorr0}", failures)
            return failures

        rc, rows, out = _legacy_run(comet_exe, case)
        if not check(rc == 0, f"search exited 0 (rc={rc}):\n{out}" if rc != 0 else "search exited 0", failures):
            return failures
        case["check"](rows, failures, check)
        return failures

    test_fn.__doc__ = f"T21 [legacy]: {name} -- migrated from 20130226-comet-tests/{case['dir']}"
    return test_fn


for _name, _case in legacy_cases.LEGACY_CASES.items():
    _safe = _name.replace("-", "_")
    register(f"t21_{_safe}")(_make_legacy_test(_name, _case))


# ---------------------------------------------------------------------------
# T22 -- RTS FI_DB / PI_DB regression (real-time single-spectrum search path)
# ---------------------------------------------------------------------------
#
# tests/rts_repro/rts_repro links directly against libcometsearch and calls the
# same InitializeSingleSpectrumSearch()/DoSingleSpectrumSearchMultiResults() API
# RealtimeSearch/SearchMS1MS2.cs calls through CometWrapper.dll -- no C++/CLI,
# no Thermo dependency, Linux-buildable (see tests/rts_repro/README.md).
# InitializeSingleSpectrumSearch() (CometSearchManager.cpp) auto-detects FI_DB
# vs PI_DB from the .idx header, so the same rts_repro binary drives both with
# no code changes.
#
# Two checks per index type:
#   1. Ground truth: t19_ascore_fidb's unambiguous phospho-S peptide must be
#      found via the RTS API -- the same fixture T19/T20 use for the batch
#      path, now exercised through the single-spectrum path instead.
#   2. Determinism: num_threads=1 and num_threads=8 must produce byte-identical
#      output over the 197-spectrum fixture (built from data/human.small.fasta),
#      per the determinism guarantee in tests/rts_repro/README.md.
#
# An "RTS vs batch agreement" check was evaluated during development and
# dropped: fixture_spectra.txt's peaks were extracted directly from
# 20250520_Hela_60min_06.raw via RawFileReader, while the closest available
# batch input (data/20250520_Hela_60min_06.mzXML) is an independently
# generated conversion of the same acquisition and does not centroid
# identically -- batch-FI vs batch-PI on that *same* mzXML only agreed ~51%
# rank-1-peptide, showing the mismatch is in the input data, not the RTS
# path itself, so no reliable agreement threshold could be calibrated from it.

RTS_REPRO_DIR = REPO_ROOT / "tests" / "rts_repro"
RTS_REPRO_BIN = RTS_REPRO_DIR / "rts_repro"
RTS_REPRO_CPP = RTS_REPRO_DIR / "rts_repro.cpp"
RTS_FIXTURE   = RTS_REPRO_DIR / "fixture_spectra.txt"


def _ensure_rts_repro_built():
    if RTS_REPRO_BIN.exists():
        return True
    if not RTS_REPRO_CPP.exists():
        return False
    cmd = [
        "g++", "-O2", "-std=c++20", "-fpermissive", "-Wno-write-strings",
        "-D_LARGEFILE_SOURCE", "-D_FILE_OFFSET_BITS=64", "-DGCC", "-D_NOSQLITE", "-D__int64=off64_t",
        f"-I{REPO_ROOT / 'CometSearch'}", f"-I{REPO_ROOT / 'MSToolkit' / 'include'}",
        f"-I{REPO_ROOT / 'MSToolkit' / 'extern' / 'expat-2.2.9' / 'lib'}",
        f"-I{REPO_ROOT / 'MSToolkit' / 'extern' / 'zlib-1.2.11'}", f"-I{REPO_ROOT / 'AScorePro' / 'include'}",
        str(RTS_REPRO_CPP), "-o", str(RTS_REPRO_BIN),
        f"-L{REPO_ROOT / 'MSToolkit'}", f"-L{REPO_ROOT / 'CometSearch'}", f"-L{REPO_ROOT / 'AScorePro'}",
        "-lcometsearch", "-lmstoolkit", "-lmstoolkitextern", "-lascorepro", "-lm", "-lpthread",
    ]
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=str(REPO_ROOT))
    return result.returncode == 0 and RTS_REPRO_BIN.exists()


def _rts_build_index(comet_exe, fasta_path, params_content, index_flag):
    idx_path = Path(fasta_path).with_suffix(".fasta.idx")
    if idx_path.exists():
        idx_path.unlink()
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(params_content)
        params_file = Path(pf.name)
    try:
        result = subprocess.run(
            [str(comet_exe), index_flag, f"-P{params_file}"],
            capture_output=True, text=True, timeout=300,
        )
        if result.returncode != 0 or not idx_path.exists():
            raise RuntimeError(f"index build failed (rc={result.returncode}):\n{result.stdout}{result.stderr}")
    finally:
        params_file.unlink(missing_ok=True)
    return idx_path


def _rts_run(idx_path, fixture_path, num_threads, output_path, ascorepro=0):
    result = subprocess.run(
        [str(RTS_REPRO_BIN), str(idx_path), str(fixture_path), str(num_threads), str(output_path), str(ascorepro)],
        capture_output=True, text=True, timeout=300,
    )
    return result.returncode, result.stdout + result.stderr


def _rts_sorted_lines(path):
    lines = Path(path).read_text().splitlines()
    return sorted(lines, key=lambda l: int(l.split("\t")[0].split()[1]))


def _test_rts_index_type(comet_exe, index_flag, label):
    failures = []
    if not _RUN_INTEGRATION:
        print("  SKIP: pass --integration to run this test")
        return []
    if not _ensure_rts_repro_built():
        print("  SKIP: tests/rts_repro/rts_repro could not be built (g++ missing or build failed)")
        return []
    if _binary_uses_win_paths(comet_exe):
        print("  SKIP: rts_repro is Linux-only; --comet is a Windows binary")
        return []

    small_fasta = REAL_DATA_DIR / "human.small.fasta"
    phospho_params = REAL_DATA_DIR / "comet_phospho.params"
    if not small_fasta.exists() or not RTS_FIXTURE.exists() or not phospho_params.exists():
        print(f"  SKIP: {small_fasta}, {phospho_params}, or {RTS_FIXTURE} not found")
        return []

    # --- 1. Ground truth: t19's unambiguous phospho-S peptide, same fixture
    #        T19/T20 use for the batch path ---
    t19_fasta = DATA_DIR / "t19_ascore_fidb.fasta"
    t19_ms2 = DATA_DIR / "t19_ascore_fidb.ms2"
    t19_fixture = Path(tempfile.mktemp(suffix=".fixture.txt", dir=str(DATA_DIR)))
    t19_fixture.write_text("\n".join(ms2_to_fixture.convert(str(t19_ms2))) + "\n")

    t19_params = T19_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=str(t19_fasta),
        ascorepro=0, mod1="79.966331 S 0 1 -1 0 0 0.0",
    )
    t19_idx = None
    try:
        t19_idx = _rts_build_index(comet_exe, t19_fasta, t19_params, index_flag)
        out_path = Path(tempfile.mktemp(suffix=".out", dir=str(DATA_DIR)))
        rc, out = _rts_run(t19_idx, t19_fixture, 1, out_path)
        if not check(rc == 0, f"{label}: rts_repro exits 0 on ground-truth fixture", failures):
            print(out)
            return failures
        lines = out_path.read_text().splitlines() if out_path.exists() else []
        out_path.unlink(missing_ok=True)
        if not check(len(lines) == 1, f"{label}: 1 ground-truth result line, got {len(lines)}", failures):
            return failures
        parts = lines[0].split("\t")
        pep = parts[1] if len(parts) > 1 else "NO_MATCH"
        check(pep != "NO_MATCH" and "ACDEFGS" in pep and "79.9663" in pep,
              f"{label}: RTS finds ACDEFGS[79.9663]K, got {pep!r}", failures)
    finally:
        if t19_idx and t19_idx.exists():
            t19_idx.unlink()
        t19_fixture.unlink(missing_ok=True)

    # --- 2. Determinism: 1 vs 8 threads over 197 real spectra ---
    hs_params_content = phospho_params.read_text().replace(
        "database_name = human.target-decoy.fasta", f"database_name = {small_fasta}")
    hs_idx = None
    try:
        hs_idx = _rts_build_index(comet_exe, small_fasta, hs_params_content, index_flag)
        out1 = Path(tempfile.mktemp(suffix=".1thread.out", dir=str(DATA_DIR)))
        out8 = Path(tempfile.mktemp(suffix=".8thread.out", dir=str(DATA_DIR)))
        try:
            rc1, log1 = _rts_run(hs_idx, RTS_FIXTURE, 1, out1)
            if not check(rc1 == 0, f"{label}: rts_repro (1 thread) exits 0", failures):
                print(log1)
                return failures
            rc8, log8 = _rts_run(hs_idx, RTS_FIXTURE, 8, out8)
            if not check(rc8 == 0, f"{label}: rts_repro (8 threads) exits 0", failures):
                print(log8)
                return failures

            lines1 = _rts_sorted_lines(out1)
            lines8 = _rts_sorted_lines(out8)
            check(len(lines1) == 197, f"{label}: 1-thread run covers all 197 fixture spectra, got {len(lines1)}", failures)
            check(lines1 == lines8,
                  f"{label}: 1-thread and 8-thread outputs are byte-identical after sorting by scan "
                  f"({sum(a != b for a, b in zip(lines1, lines8))} differing lines out of {len(lines1)})",
                  failures)
        finally:
            out1.unlink(missing_ok=True)
            out8.unlink(missing_ok=True)
    finally:
        if hs_idx and hs_idx.exists():
            hs_idx.unlink()

    return failures


@register("t22_rts_fi")
def test_t22_rts_fi(comet_exe):
    """T22 [integration]: RTS single-spectrum search against an FI_DB index."""
    return _test_rts_index_type(comet_exe, "-i", "FI_DB")


@register("t22_rts_pi")
def test_t22_rts_pi(comet_exe):
    """T22 [integration]: RTS single-spectrum search against a PI_DB index."""
    return _test_rts_index_type(comet_exe, "-j", "PI_DB")


# ---------------------------------------------------------------------------
# T23 -- decoy-mode parity (comet-debug3), --bigdata gated
# ---------------------------------------------------------------------------
#
# Migrated from 20130226-comet-tests/comet-debug3, which ran these same two
# configs and eyeballed an FDR scatter plot (qvalue.exe / explorer.exe *.png).
# Same two configs here, but pass/fail is a real 1%-FDR PSM-count comparison
# via tools/qvalue.py: internal-decoy and target-decoy searches of the same
# real HeLa run should identify a similar number of peptides at 1% FDR.
#
# Needs ~350MB of real data (177MB mzXML, 57MB/116MB FASTAs) referenced in
# place via --bigdata (default: sibling 20130226-comet-tests/ directory) and
# takes several minutes -- skips cleanly if absent, like T17/T18 skip without
# human.small.fasta. Data is never copied.

sys.path.insert(0, str(REPO_ROOT / "tools"))
import qvalue  # noqa: E402


def _q1pct_counts(txt_path):
    """Return (n_rank1_psms, xcorr_count_at_1pct, evalue_count_at_1pct) via tools/qvalue.py."""
    psms = qvalue.load_rank1(str(txt_path))
    sx = qvalue._sort_psms(psms, "xcorr")
    qx = qvalue.compute_qvalues(sx)
    cx, _ = qvalue._count_passing(sx, qx, 0.01, qvalue._F_XCORR)
    se = qvalue._sort_psms(psms, "evalue")
    qe = qvalue.compute_qvalues(se)
    ce, _ = qvalue._count_passing(se, qe, 0.01, qvalue._F_EVALUE)
    return len(psms), cx, ce


def _run_bigdata_search(comet_exe, params_content, mzxml_path, timeout=600):
    """Returns (returncode, txt_path, output, elapsed_seconds). elapsed_seconds
    is wall-clock time around the subprocess call -- a single-sample real-machine
    measurement, not an average of repeated runs; see _check_timing()."""
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(params_content)
        params_file = Path(pf.name)
    txt_path = Path(mzxml_path).with_suffix(".txt")
    if txt_path.exists():
        txt_path.unlink()
    try:
        t0 = time.perf_counter()
        result = subprocess.run(
            [str(comet_exe), f"-P{params_file}", str(mzxml_path)],
            capture_output=True, text=True, timeout=timeout,
        )
        elapsed = time.perf_counter() - t0
        return result.returncode, txt_path, result.stdout + result.stderr, elapsed
    finally:
        params_file.unlink(missing_ok=True)


def _set_param_line(params_text, key, value):
    """Set `key = value` in a comet.params text blob: replaces an existing line for
    that key if present, otherwise inserts a new one -- silently no-op'ing on a
    missing key (the original behavior here) is a real footgun for any key that isn't
    guaranteed to already exist in every params fixture (e.g. index_search_type is
    absent from comet-debug3/4's real, pre-unification comet.params). Insertion must
    go *before* a `[COMET_ENZYME_INFO]` section marker if present -- comet.params
    itself documents that section as required to be last, and Comet's own parser
    silently ignores a `key = value` line placed after it (confirmed directly while
    investigating a T24 dispatch bug: an appended-at-EOF index_search_type line never
    took effect). Falls back to appending at the end if there's no enzyme section."""
    new_text, n = re.subn(rf"(?m)^{re.escape(key)} = .*$", f"{key} = {value}", params_text, count=1)
    if n > 0:
        return new_text
    line = f"{key} = {value}\n"
    marker = re.search(r"(?m)^\[COMET_ENZYME_INFO\]", params_text)
    if marker:
        pos = marker.start()
        return params_text[:pos] + line + params_text[pos:]
    sep = "" if params_text.endswith("\n") else "\n"
    return params_text + sep + line


def _index_build_and_search(binary, flag, label, plain_params, idx_path, mzxml, failures, tag=""):
    """Build an FI_DB (-i) or PI_DB (-j) index with `binary` and search `mzxml`
    against it. Returns (xcorr_count_at_1pct, build_seconds, search_seconds),
    or None if the build/search failed (already recorded in `failures` via
    check()). `idx_path` is rebuilt fresh each call, so it's safe to reuse
    across binaries as long as calls aren't interleaved concurrently."""
    prefix = f"{tag} " if tag else ""
    if idx_path.exists():
        idx_path.unlink()
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(plain_params)
        idx_params_file = Path(pf.name)
    try:
        t0 = time.perf_counter()
        r = subprocess.run([str(binary), flag, f"-P{idx_params_file}"],
                            capture_output=True, text=True, timeout=400)
        build_elapsed = time.perf_counter() - t0
        if not check(r.returncode == 0 and idx_path.exists(),
                      f"{prefix}{label} index builds (rc={r.returncode})", failures):
            print((r.stdout + r.stderr)[-2000:])
            return None
    finally:
        idx_params_file.unlink(missing_ok=True)

    # -i/-j only select which build flag is passed above; per docs/20260730_PI_reduction.md
    # Phase 0, PI_DB and FI_DB now share one on-disk .idx format/builder, so the file itself
    # no longer implies a search mode -- which mode a *search* runs against it is controlled
    # purely by index_search_type (0=PI_DB, 1=FI_DB; unset defaults to FI_DB). Without setting
    # it explicitly here, both legs of this function silently searched in FI_DB mode
    # regardless of `label`/`flag`, making the "PI_DB" leg a mislabeled duplicate of the
    # "FI_DB" leg -- caught by a real PI_DB-vs-FI_DB PSM-count divergence (17,660 vs 17,033 on
    # comet-debug3/4's real data) this test should have been able to catch but couldn't.
    idx_params = _set_param_line(plain_params, "database_name", idx_path)
    idx_params = _set_param_line(idx_params, "index_search_type", 0 if label == "PI_DB" else 1)
    rc, txt, out, search_elapsed = _run_bigdata_search(binary, idx_params, mzxml)
    if not check(rc == 0, f"{prefix}{label} search exits 0 (rc={rc})", failures):
        print(out[-2000:])
        return None
    _, cx, _ = _q1pct_counts(txt)
    txt.unlink(missing_ok=True)
    return cx, build_elapsed, search_elapsed


# A single-sample wall-clock comparison on shared/real hardware can easily vary
# 10-20% run to run for a multi-minute search with no code change at all --
# this is intentionally a generous threshold so it flags a real slowdown
# rather than ordinary machine jitter. Treat one failure here as "worth a
# re-run to confirm," not proof of a regression; treat a *repeated* failure
# as a real one.
TIMING_NOISE_TOLERANCE = 0.25   # current allowed to be up to 25% slower


def _check_timing(current_s, baseline_s, label, failures, rel_tol=TIMING_NOISE_TOLERANCE):
    ratio = (current_s / baseline_s) if baseline_s else float("inf")
    check(ratio <= 1 + rel_tol,
          f"{label}: current ({current_s:.1f}s) not more than {rel_tol*100:.0f}% slower than "
          f"{BASELINE_TAG} ({baseline_s:.1f}s) -- ratio {ratio:.3f}", failures)


def _ensure_baseline():
    """Return the previous-version baseline binary Path for T23/T24's
    cross-version checks, downloading it via setup_baselines.py's fetch logic
    on first use if it's missing from its default location. Returns None
    (never raises) if unavailable -- callers should skip just their
    cross-version checks in that case, not the whole test.

    Auto-download only applies to the default path (BASELINE_TAG's expected
    location); if --baseline was pointed at a custom path that doesn't exist,
    that's treated as an explicit "no baseline" rather than downloaded over.
    """
    baseline_exe = Path(_BASELINE_EXE)
    if baseline_exe.exists():
        return baseline_exe

    if baseline_exe != DEFAULT_BASELINE_EXE:
        print(f"  Baseline not found at {baseline_exe} (--baseline was set explicitly; "
              f"not auto-downloading over a custom path)")
        return None

    print(f"  Baseline {BASELINE_TAG} not found at {baseline_exe}; downloading from "
          f"GitHub Releases ...")
    ok = setup_baselines.setup_tag(BASELINE_TAG)
    if not ok or not baseline_exe.exists():
        print(f"  Baseline {BASELINE_TAG} download failed or produced no binary")
        return None
    return baseline_exe


@register("t23_decoy_modes")
def test_t23_decoy_modes(comet_exe):
    """T23 [integration]: comet-debug3 -- internal-decoy vs target-decoy 1% FDR parity."""
    if not _RUN_INTEGRATION:
        print("  SKIP: pass --integration to run this test")
        return []

    failures = []
    d3 = Path(_BIGDATA_DIR) / "comet-debug3"
    mzxml = d3 / "20170103_HelaQC_01.mzXML"
    human_fasta = d3 / "human.fasta"
    human_td_fasta = d3 / "human.target-decoy.fasta"
    base_params_file = d3 / "comet.params"
    if not (mzxml.exists() and human_fasta.exists() and human_td_fasta.exists() and base_params_file.exists()):
        print(f"  SKIP: {d3} not found or incomplete -- pass --bigdata DIR "
              f"(this test needs ~350MB of real data not checked into the repo)")
        return []

    base = base_params_file.read_text()
    internaldecoy_params = _set_param_line(base, "database_name", human_fasta)
    internaldecoy_params = _set_param_line(internaldecoy_params, "decoy_search", "1")
    targetdecoy_params = _set_param_line(base, "database_name", human_td_fasta)
    targetdecoy_params = _set_param_line(targetdecoy_params, "decoy_search", "0")

    print("  Running internal-decoy search (human.fasta, decoy_search=1) ...")
    rc1, txt1, out1, t1 = _run_bigdata_search(comet_exe, internaldecoy_params, mzxml)
    if not check(rc1 == 0, f"internal-decoy search exits 0 (rc={rc1})", failures):
        print(out1[-2000:])
        return failures

    print("  Running target-decoy search (human.target-decoy.fasta, decoy_search=0) ...")
    rc2, txt2, out2, t2 = _run_bigdata_search(comet_exe, targetdecoy_params, mzxml)
    if not check(rc2 == 0, f"target-decoy search exits 0 (rc={rc2})", failures):
        print(out2[-2000:])
        return failures

    n1, cx1, ce1 = _q1pct_counts(txt1)
    n2, cx2, ce2 = _q1pct_counts(txt2)
    txt1.unlink(missing_ok=True)
    txt2.unlink(missing_ok=True)

    check(10_000 <= cx1 <= 30_000,
          f"internal-decoy: {cx1:,} PSMs at 1% FDR (xcorr) in plausible range [10k, 30k]", failures)
    check(10_000 <= cx2 <= 30_000,
          f"target-decoy: {cx2:,} PSMs at 1% FDR (xcorr) in plausible range [10k, 30k]", failures)
    ratio = (cx1 / cx2) if cx2 else float("inf")
    check(0.95 <= ratio <= 1.05,
          f"internal-decoy ({cx1:,}) and target-decoy ({cx2:,}) agree within 5% "
          f"at 1% FDR xcorr (ratio {ratio:.3f})", failures)

    # --- Cross-version: same two configs against the v2025.03.0 baseline ---
    baseline_exe = _ensure_baseline()
    if baseline_exe is None:
        print(f"  SKIP cross-version checks: {BASELINE_TAG} baseline unavailable")
    else:
        print(f"  Running internal-decoy search with baseline {BASELINE_TAG} ...")
        brc1, btxt1, bout1, bt1 = _run_bigdata_search(baseline_exe, internaldecoy_params, mzxml)
        if not check(brc1 == 0, f"baseline ({BASELINE_TAG}) internal-decoy search exits 0 (rc={brc1})", failures):
            print(bout1[-2000:])
        else:
            _, bcx1, _ = _q1pct_counts(btxt1)
            btxt1.unlink(missing_ok=True)
            bratio1 = (cx1 / bcx1) if bcx1 else float("inf")
            check(0.9 <= bratio1 <= 1.1,
                  f"internal-decoy: current ({cx1:,}) agrees with {BASELINE_TAG} ({bcx1:,}) "
                  f"within 10% at 1% FDR xcorr (ratio {bratio1:.3f})", failures)
            _check_timing(t1, bt1, "internal-decoy search time", failures)

        print(f"  Running target-decoy search with baseline {BASELINE_TAG} ...")
        brc2, btxt2, bout2, bt2 = _run_bigdata_search(baseline_exe, targetdecoy_params, mzxml)
        if not check(brc2 == 0, f"baseline ({BASELINE_TAG}) target-decoy search exits 0 (rc={brc2})", failures):
            print(bout2[-2000:])
        else:
            _, bcx2, _ = _q1pct_counts(btxt2)
            btxt2.unlink(missing_ok=True)
            bratio2 = (cx2 / bcx2) if bcx2 else float("inf")
            check(0.9 <= bratio2 <= 1.1,
                  f"target-decoy: current ({cx2:,}) agrees with {BASELINE_TAG} ({bcx2:,}) "
                  f"within 10% at 1% FDR xcorr (ratio {bratio2:.3f})", failures)
            _check_timing(t2, bt2, "target-decoy search time", failures)

    return failures


# ---------------------------------------------------------------------------
# T24 -- FI_DB / PI_DB index parity vs. plain FASTA (comet-debug4), --bigdata gated
# ---------------------------------------------------------------------------
#
# Migrated from 20130226-comet-tests/comet-debug4. A target-decoy search
# should identify a similar peptide population whether run against the plain
# FASTA, a fragment-ion index (-i), or a peptide index (-j) -- FI/PI trade
# exhaustiveness for speed, but shouldn't diverge sharply from brute force.
#
# NOTE: while developing this test, one manual (non-harness) attempt to search
# a full-scale target-decoy FI_DB crashed with
#   terminate called after throwing an instance of 'std::length_error':
#     cannot create std::vector larger than max_size()
# That manual build was interrupted by a shell command timeout partway through
# writing the .idx file, which most likely left a truncated/corrupt index on
# disk -- the crash was almost certainly reading that corrupt file, not a
# Comet defect. Under this test's own clean build-then-search sequence (no
# interruption), FI_DB has run correctly every time (see the count-agreement
# check below). If this ever resurfaces under the harness's own clean run,
# treat it as a live regression and start by ruling out a truncated .idx
# before going near CometFragmentIndex's read path.

@register("t24_index_parity")
def test_t24_index_parity(comet_exe):
    """T24 [integration]: comet-debug4 -- plain FASTA vs FI_DB vs PI_DB 1% FDR parity."""
    if not _RUN_INTEGRATION:
        print("  SKIP: pass --integration to run this test")
        return []

    failures = []
    d3 = Path(_BIGDATA_DIR) / "comet-debug3"  # comet-debug4 reuses comet-debug3's mzXML + fasta
    mzxml = d3 / "20170103_HelaQC_01.mzXML"
    human_td_fasta = d3 / "human.target-decoy.fasta"
    base_params_file = d3 / "comet.params"
    if not (mzxml.exists() and human_td_fasta.exists() and base_params_file.exists()):
        print(f"  SKIP: {d3} not found or incomplete -- pass --bigdata DIR "
              f"(this test needs ~350MB of real data not checked into the repo)")
        return []

    base = base_params_file.read_text()
    plain_params = _set_param_line(base, "database_name", human_td_fasta)
    plain_params = _set_param_line(plain_params, "decoy_search", "0")

    print("  Running plain-FASTA target-decoy search ...")
    rc0, txt0, out0, t0 = _run_bigdata_search(comet_exe, plain_params, mzxml)
    if not check(rc0 == 0, f"plain-FASTA search exits 0 (rc={rc0})", failures):
        print(out0[-2000:])
        return failures
    n0, cx0, ce0 = _q1pct_counts(txt0)
    txt0.unlink(missing_ok=True)
    check(10_000 <= cx0 <= 30_000,
          f"plain-FASTA: {cx0:,} PSMs at 1% FDR (xcorr) in plausible range [10k, 30k]", failures)

    idx_path = human_td_fasta.with_suffix(".fasta.idx")

    current_counts = {"plain-FASTA": cx0}
    current_search_times = {"plain-FASTA": t0}
    current_build_times = {}
    # PI_DB has no fragment-ion-index pre-filter, so it searches the same precursor-mass-
    # window candidate set plain-FASTA does -- a real A/B on comet-debug3/4's data (after
    # fixing this test's missing index_search_type, see _index_build_and_search) measured an
    # *exact* match (17,660 == 17,660), so its tolerance here is intentionally much tighter
    # than FI_DB's, which genuinely excludes some candidates via its posting-list filter and
    # legitimately varies more by dataset (measured ~3.6% low on the same data).
    tolerance = {"FI_DB": 0.05, "PI_DB": 0.01}
    for flag, label in (("-i", "FI_DB"), ("-j", "PI_DB")):
        print(f"  Building {label} index ...")
        result = _index_build_and_search(comet_exe, flag, label, plain_params, idx_path, mzxml, failures)
        if result is None:
            continue
        cx, build_s, search_s = result
        current_counts[label] = cx
        current_build_times[label] = build_s
        current_search_times[label] = search_s
        ratio = (cx / cx0) if cx0 else float("inf")
        tol = tolerance[label]
        check(1 - tol <= ratio <= 1 + tol,
              f"{label} ({cx:,}) agrees with plain-FASTA ({cx0:,}) within {tol*100:.0f}% at 1% FDR "
              f"xcorr (ratio {ratio:.3f})", failures)

    idx_path.unlink(missing_ok=True)

    # --- Cross-version: same three modes against the v2025.03.0 baseline ---
    baseline_exe = _ensure_baseline()
    if baseline_exe is None:
        print(f"  SKIP cross-version checks: {BASELINE_TAG} baseline unavailable")
        return failures

    print(f"  Running plain-FASTA target-decoy search with baseline {BASELINE_TAG} ...")
    brc0, btxt0, bout0, bt0 = _run_bigdata_search(baseline_exe, plain_params, mzxml)
    if not check(brc0 == 0, f"baseline ({BASELINE_TAG}) plain-FASTA search exits 0 (rc={brc0})", failures):
        print(bout0[-2000:])
        return failures
    _, bcx0, _ = _q1pct_counts(btxt0)
    btxt0.unlink(missing_ok=True)
    bratio0 = (current_counts["plain-FASTA"] / bcx0) if bcx0 else float("inf")
    check(0.9 <= bratio0 <= 1.1,
          f"plain-FASTA: current ({current_counts['plain-FASTA']:,}) agrees with "
          f"{BASELINE_TAG} ({bcx0:,}) within 10% at 1% FDR xcorr (ratio {bratio0:.3f})", failures)
    _check_timing(current_search_times["plain-FASTA"], bt0, "plain-FASTA search time", failures)

    for flag, label in (("-i", "FI_DB"), ("-j", "PI_DB")):
        if label not in current_counts:
            continue  # current binary's own build/search already failed for this mode
        print(f"  Building {label} index with baseline {BASELINE_TAG} ...")
        bresult = _index_build_and_search(baseline_exe, flag, label, plain_params, idx_path, mzxml,
                                           failures, tag=f"baseline ({BASELINE_TAG})")
        if bresult is None:
            continue
        bcx, bbuild_s, bsearch_s = bresult
        bratio = (current_counts[label] / bcx) if bcx else float("inf")
        check(0.9 <= bratio <= 1.1,
              f"{label}: current ({current_counts[label]:,}) agrees with {BASELINE_TAG} "
              f"({bcx:,}) within 10% at 1% FDR xcorr (ratio {bratio:.3f})", failures)
        _check_timing(current_build_times[label], bbuild_s, f"{label} index build time", failures)
        _check_timing(current_search_times[label], bsearch_s, f"{label} search time", failures)

    idx_path.unlink(missing_ok=True)
    return failures


# ---------------------------------------------------------------------------
# T25 -- FI_DB variable-mod compacted-slot-index regression
# ---------------------------------------------------------------------------
#
# CometFragmentIndex.cpp's AddFragments() (precursor-mass and fragment-ion-mass loops) and
# AddFragmentsThreadProc() (protein-variable-mod-filter check) all read
# MOD_NUMBERS[modNumIdx].modifications[] (aliased locally as "mods") and, until this fix, used
# its values directly as raw varModList indices. Those values are actually 0-based indices into
# a COMPACTED active-variable-mod-slot list (CometPeptideIndex::GetVModSlotForAllModsIdx()) --
# they only coincide with the real varModList slot when every active variable_modNN among the
# first FRAGINDEX_VMODS is contiguous starting at slot 0. A config with a gap (e.g.
# variable_mod01 left unset while variable_mod02 carries the real modification) exposed this:
# the precursor mass, the modified fragment-ion masses used for XCorr/SP scoring
# (CometSearch.cpp's SearchFragmentIndex(), a separate, independent copy of the same
# reconstruction logic), and the reported modification mass were all computed against the
# wrong (unused, zero-mass) slot instead of the real one.
#
# This test deliberately configures the real mod in variable_mod02 (slot 1), leaving
# variable_mod01 (slot 0) unused, so a regression here can't hide the way a slot-0 config
# would (every mod in slot 0 trivially has compacted-index == real-slot-index == 0). Fixture:
# ACDS[+79.966331]EFGHIK (10 residues, phospho-S at position 4, charge 2+), spectrum built from
# monoisotopic residue masses independently in Python, not read back from Comet's own output.

T25_PARAMS_TEMPLATE = textwrap.dedent("""\
# comet_version {comet_version}
database_name = {database}
decoy_search = 0
num_threads = 4
peptide_mass_tolerance_upper = 20.0
peptide_mass_tolerance_lower = -20.0
peptide_mass_units = 2
precursor_tolerance_type = 1
isotope_error = 0
search_enzyme_number = 0
search_enzyme2_number = 0
sample_enzyme_number = 0
num_enzyme_termini = 2
allowed_missed_cleavage = 0
variable_mod01 = 0.0 X 0 3 -1 0 0 0.0
variable_mod02 = 79.966331 S 0 1 -1 0 0 0.0
variable_mod03 = 0.0 X 0 3 -1 0 0 0.0
variable_mod04 = 0.0 X 0 3 -1 0 0 0.0
variable_mod05 = 0.0 X 0 3 -1 0 0 0.0
max_variable_mods_in_peptide = 1
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
digest_mass_range = 200.0 2000.0
peptide_length_range = 10 10
max_duplicate_proteins = -1
max_fragment_charge = 3
min_precursor_charge = 1
max_precursor_charge = 6
clip_nterm_methionine = 0
spectrum_batch_size = 15000
decoy_prefix = DECOY_
equal_I_and_L = 0
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
add_C_cysteine = 0.0
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


@register("t25_fi_mod_slot_gap")
def test_t25_fi_mod_slot_gap(comet_exe):
    """T25: FI_DB gap variable-mod-slot regression -- mod in variable_mod02 (slot 1),
    variable_mod01 (slot 0) left unused; must not silently resolve to the wrong slot."""
    failures = []

    fasta = DATA_DIR / "t25_fi_mod_slot_gap.fasta"
    ms2   = DATA_DIR / "t25_fi_mod_slot_gap.ms2"
    idx   = fasta.with_suffix(".fasta.idx")
    txt   = ms2.with_suffix(".txt")

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    if idx.exists():
        idx.unlink()

    build_params = T25_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(fasta))
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(build_params)
        build_params_file = Path(pf.name)
    try:
        rc, out = _run_t19_step(comet_exe, ["-i", f"-P{fmt(build_params_file)}"])
        if rc != 0 or not idx.exists():
            failures.append(f"index build failed (rc={rc}):\n{out}")
            return failures
    finally:
        build_params_file.unlink(missing_ok=True)

    if txt.exists():
        txt.unlink()

    search_params = T25_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(idx))
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(search_params)
        search_params_file = Path(pf.name)

    try:
        rc, out = _run_t19_step(comet_exe, [f"-P{fmt(search_params_file)}", fmt(ms2)])
        if rc != 0:
            failures.append(f"search failed (rc={rc}):\n{out}")
            return failures
        if not txt.exists():
            failures.append(f".txt not created (peptide not found -- the old bug corrupted "
                             f"the precursor mass for this gap config). Comet output:\n{out}")
            return failures

        lines  = txt.read_text().splitlines()
        rows   = [l.split("\t") for l in lines[2:] if l.strip()]

        check(len(rows) == 1, f"expected exactly 1 PSM row, got {len(rows)}", failures)
        if not rows:
            return failures

        header = lines[1].split("\t")
        row = dict(zip(header, rows[0]))

        check(row.get("plain_peptide") == "ACDSEFGHIK",
              f"plain_peptide: expected ACDSEFGHIK, got {row.get('plain_peptide')!r}", failures)
        # The old bug resolved the compacted index (0) directly, pointing at the unused
        # variable_mod01 slot (mass 0.0) instead of the real variable_mod02 slot (79.966331).
        check("4_V_79.966331" in row.get("modifications", ""),
              f"modifications: expected to contain 4_V_79.966331 (the real variable_mod02 "
              f"mass, not the unused gap slot's 0.0), got {row.get('modifications')!r}",
              failures)
        check(int(row.get("ions_matched", "0")) == 14,
              f"ions_matched: expected all 14 fragment ions matched, got "
              f"{row.get('ions_matched')!r}", failures)
    finally:
        search_params_file.unlink(missing_ok=True)
        idx.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)

    return failures


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    global _RUN_INTEGRATION, _BASELINE_EXE, _BIGDATA_DIR

    all_tests = list(TESTS.keys())
    non_integration = [t for t in all_tests if t not in INTEGRATION_TESTS]

    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--comet", action="append", default=None,
                        help="path to a Comet binary; repeat to run the suite against "
                             "multiple binaries (e.g. Linux comet.exe and Windows Comet.exe)")
    parser.add_argument("--integration", action="store_true",
                        help=f"run integration tests {', '.join(INTEGRATION_TESTS)} "
                             "(require human.small.fasta and/or --bigdata)")
    parser.add_argument("--baseline", default=str(DEFAULT_BASELINE_EXE),
                        help="path to v2026.01.1 baseline binary (for T17)")
    parser.add_argument("--bigdata", default=str(REPO_ROOT.parent / "20130226-comet-tests"),
                        help="directory holding comet-debug3/comet-debug4 big-data fixtures "
                             "(for T23/T24); referenced in place, never copied")
    parser.add_argument("tests", nargs="*", default=non_integration,
                        help="test IDs to run (default: all non-integration tests)")
    args = parser.parse_args()

    _RUN_INTEGRATION = args.integration
    _BASELINE_EXE    = args.baseline
    _BIGDATA_DIR     = args.bigdata

    comet_binaries = args.comet or [str(COMET_EXE)]
    for b in comet_binaries:
        if not Path(b).exists():
            print(f"ERROR: Comet binary not found: {b}", file=sys.stderr)
            sys.exit(2)

    requested = args.tests
    # If --integration is passed and none of INTEGRATION_TESTS is explicitly listed, add them all
    if args.integration and not any(t in requested for t in INTEGRATION_TESTS):
        requested = requested + list(INTEGRATION_TESTS)

    unknown = set(requested) - set(TESTS)
    if unknown:
        print(f"ERROR: Unknown test(s): {unknown}", file=sys.stderr)
        print(f"Available: {all_tests}", file=sys.stderr)
        sys.exit(2)

    grand_fail = 0

    for binary in comet_binaries:
        comet_exe = Path(binary)
        print(f"\n{'#'*60}")
        print(f"  Binary: {comet_exe}")
        print(f"{'#'*60}")

        total_fail = 0
        total_pass = 0
        total_skip = 0

        for name in requested:
            print(f"\n{'='*60}")
            print(f"  {name}: {TESTS[name].__doc__.strip().splitlines()[0]}")
            print(f"{'='*60}")
            try:
                failures = TESTS[name](comet_exe)
            except Exception as e:
                print(f"  ERROR: {e}")
                failures = [str(e)]

            if failures == [] and name in INTEGRATION_TESTS and not _RUN_INTEGRATION:
                total_skip += 1
                print("  --> SKIPPED")
            elif failures:
                total_fail += 1
                print(f"  --> FAILED ({len(failures)} check(s))")
            else:
                total_pass += 1
                print("  --> PASSED")

        print(f"\n{'='*60}")
        print(f"  [{comet_exe}] Results: {total_pass} passed, {total_fail} failed, {total_skip} skipped")
        print(f"{'='*60}")
        grand_fail += total_fail

    if len(comet_binaries) > 1:
        print(f"\n{'#'*60}")
        print(f"  Overall across {len(comet_binaries)} binaries: "
              f"{'ALL PASSED' if grand_fail == 0 else f'{grand_fail} total failure(s)'}")
        print(f"{'#'*60}")
    print(f"{'='*60}")
    sys.exit(0 if grand_fail == 0 else 1)


if __name__ == "__main__":
    main()
