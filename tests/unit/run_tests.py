#!/usr/bin/env python3
"""
Comet plain-peptide index unit tests (T1-T7, T11-T16) and integration tests (T17-T18).

Runs Comet.exe -i on each crafted FASTA and verifies expected properties.

Usage:
    python run_tests.py [--comet PATH] [--integration] [--baseline PATH] [test_id ...]

    --comet       path to Comet binary (default: ../../comet.exe)
    --integration also run T17 and T18 (require human.small.fasta in data/)
    --baseline    path to v2026.01.1 baseline binary (for T17)
    test_id       one or more test IDs (default: all non-integration tests)

Exit code 0 = all tests passed; non-zero = failures.
"""

import argparse
import filecmp
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path


UNIT_DIR      = Path(__file__).parent.resolve()
DATA_DIR      = UNIT_DIR / "data"
REPO_ROOT     = UNIT_DIR.parent.parent
REAL_DATA_DIR = REPO_ROOT / "data"

# Default binary paths
COMET_EXE            = REPO_ROOT / "comet.exe"
DEFAULT_BASELINE_EXE = REPO_ROOT / "tests" / "regression" / "baselines" / "v2026.01.1" / "comet"

MASS_TOL        = 0.002   # Da -- loose tolerance for monoisotopic masses
WIDTH_REFERENCE = 512

# Set by main() before running integration tests
_RUN_INTEGRATION = False
_BASELINE_EXE    = str(DEFAULT_BASELINE_EXE)


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

        f.seek(-24, 2)
        pep_pos, prot_pos, perm_pos = struct.unpack("<qqq", f.read(24))

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
# main
# ---------------------------------------------------------------------------

def main():
    global _RUN_INTEGRATION, _BASELINE_EXE

    all_tests = list(TESTS.keys())
    non_integration = [t for t in all_tests if t not in ("t17", "t18")]

    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--comet", default=str(COMET_EXE),
                        help="path to Comet binary")
    parser.add_argument("--integration", action="store_true",
                        help="run integration tests T17 and T18 (require human.small.fasta)")
    parser.add_argument("--baseline", default=str(DEFAULT_BASELINE_EXE),
                        help="path to v2026.01.1 baseline binary (for T17)")
    parser.add_argument("tests", nargs="*", default=non_integration,
                        help="test IDs to run (default: all non-integration tests)")
    args = parser.parse_args()

    _RUN_INTEGRATION = args.integration
    _BASELINE_EXE    = args.baseline

    comet_exe = Path(args.comet)
    if not comet_exe.exists():
        print(f"ERROR: Comet binary not found: {comet_exe}", file=sys.stderr)
        sys.exit(2)

    requested = args.tests
    # If --integration is passed and T17/T18 not explicitly listed, add them
    if args.integration and "t17" not in requested:
        requested = requested + ["t17", "t18"]

    unknown = set(requested) - set(TESTS)
    if unknown:
        print(f"ERROR: Unknown test(s): {unknown}", file=sys.stderr)
        print(f"Available: {all_tests}", file=sys.stderr)
        sys.exit(2)

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

        if failures == [] and name in ("t17", "t18") and not _RUN_INTEGRATION:
            total_skip += 1
            print("  --> SKIPPED")
        elif failures:
            total_fail += 1
            print(f"  --> FAILED ({len(failures)} check(s))")
        else:
            total_pass += 1
            print("  --> PASSED")

    print(f"\n{'='*60}")
    print(f"  Results: {total_pass} passed, {total_fail} failed, {total_skip} skipped")
    print(f"{'='*60}")
    sys.exit(0 if total_fail == 0 else 1)


if __name__ == "__main__":
    main()
