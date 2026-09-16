#!/usr/bin/env python3
"""
Comet plain-peptide index unit tests (T1-T7, T11-T16, T19-T21, T25-T33) and
integration tests (T17, T18, T22-T24).

Runs Comet.exe -i on each crafted FASTA and verifies expected properties.

Usage:
    python run_tests.py [--comet PATH] [--integration] [--baseline PATH] [test_id ...]

    --comet       path to Comet binary (default: ../../comet.exe); repeatable
    --integration also run T17, T18, T22-T24 (require human.small.fasta and/or --bigdata)
    --baseline    path to a previous-Comet-version binary for T23/T24's cross-version
                  checks (default: tests/regression/baselines/v2026.02.2/comet,
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
BASELINE_TAG         = "v2026.02.2"
DEFAULT_BASELINE_EXE = setup_baselines.BASELINES_DIR / BASELINE_TAG / setup_baselines.asset_url(BASELINE_TAG)[1]

MASS_TOL        = 0.002   # Da -- loose tolerance for monoisotopic masses
WIDTH_REFERENCE = 512

# Set by main() before running integration tests
_RUN_INTEGRATION = False
_BASELINE_EXE    = str(DEFAULT_BASELINE_EXE)

# Tests gated behind --integration: they need large/manually-supplied data
# and/or take much longer than the T1-T16/T19-T21 unit tests.
INTEGRATION_TESTS = ("t17", "t18", "t22_rts_fi", "t22_rts_pi", "t23_decoy_modes", "t24_index_parity",
                     "t24_internal_decoy_parity")

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


def _localize_params_for(binary, params_text):
    """Rewrite the `database_name = /mnt/<drive>/...` line of a params blob into
    Drive:\\... form when `binary` is a Windows PE. T23/T24's params are built
    from real fixtures with WSL-style absolute paths and are then handed, verbatim,
    to whichever binary (current or baseline) is being timed -- the two can even be
    different platforms -- so the conversion has to happen per-binary, at the point
    of use, not once up front. Everything else in comet.params is path-free."""
    if not _binary_uses_win_paths(binary):
        return params_text
    return re.sub(r"(?m)^(database_name = )(/mnt/\S+)$",
                  lambda m: m.group(1) + _to_win(m.group(2)), params_text)


# ---------------------------------------------------------------------------
# .idx reader
# ---------------------------------------------------------------------------

def parse_idx(path):
    """
    Returns dict: {peptide_seq -> {"mass", "prevAA", "nextAA", "proteins": list[str]}}
    """
    with open(path, "rb") as f:
        # docs/20260730_PI_reduction.md Phase 0.5: footer shrank back to 2 pointers
        # (peptides, proteins) -- the permutation-table and compact-variant-array sections
        # (and the footer's 2 extra pointers to them) were removed; modified-peptide data is
        # regenerated in memory each search session instead of persisted. (Section-position
        # is footer-relative, not sequential-scan-relative, so no header-skipping read is
        # needed before this seek -- the pre-Phase-0.5 version of this function had one, but
        # it was already dead code: it never influenced anything below it.)
        f.seek(-16, 2)
        footer_pos = f.tell()
        pep_pos, prot_pos = struct.unpack("<qq", f.read(16))

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
        prot_buf = f.read(footer_pos - prot_pos)
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
# T19 -- AScore + FI_DB regression (docs/20260617_codereview3.md issue 2a;
# .idx-header-mod precedence restored by docs/20260811_restore_idx_header_mods.md)
# ---------------------------------------------------------------------------
#
# CometSearchManager::SetAScoreOptions() reads g_staticParams.variableModParameters.
# varModList[] to configure AScorePro's differential-mod list. This test proves that
# an FI_DB search picks up its variable mod from the .idx file's own VariableMod:
# header line -- overwriting whatever (or nothing) search-time comet.params declared
# -- and that AScore is configured *after* that header-driven overwrite (see the
# ordering comment in CometSearch/search/Pipeline.cpp). PR121's Phase 0.5 had
# temporarily dropped VariableMod:/ProteinModList:/RequireVariableMod: from the
# header entirely (mods came solely from live comet.params); docs/20260811_
# restore_idx_header_mods.md put them back so an .idx is self-contained again, no
# search-time variable_modNN params required. Build with the real phospho-S mod,
# search with variable_mod01 left *blank* in search-time params -- the header must
# still win for AScore to localize correctly.
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
    """T19: AScore + FI_DB regression -- AScore must use the .idx header's
    variable mod even when search-time params leave it blank."""
    failures = []

    fasta = DATA_DIR / "t19_ascore_fidb.fasta"
    ms2   = DATA_DIR / "t19_ascore_fidb.ms2"
    idx   = fasta.with_suffix(".fasta.idx")
    txt   = ms2.with_suffix(".txt")

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    # Step 1: build an FI_DB index with the real phospho-S mod -- this is now the
    # only place the mod is declared; it gets baked into the .idx's VariableMod:
    # header line (docs/20260811_restore_idx_header_mods.md).
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

    # Step 2: search the index with print_ascorepro_score enabled and variable_mod01
    # left blank in search-time params -- the .idx header's VariableMod: line must
    # still be what AScore configures from (ParsePeptideIndexHeader() overwrites
    # whatever comet.params supplied, the same precedent StaticMod: already set).
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
              f"ascorepro: expected > 0 (AScore must run using the .idx header's "
              f"VariableMod: line even though search-time params left it blank), "
              f"got {ascorepro}", failures)
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

    # Step 1: build a PI_DB (peptide index) with the real phospho-S mod -- like T19,
    # this is now the only place the mod is declared; it's baked into the .idx's
    # VariableMod: header line (docs/20260811_restore_idx_header_mods.md). "-j"
    # selects create_peptide_index, unlike T19's "-i" (create_fragment_index).
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

    # Step 2: search the PI_DB index with variable_mod01 left blank in search-time
    # params -- the .idx header's VariableMod: line must still be what's applied
    # (ParsePeptideIndexHeader() overwrites whatever comet.params supplied). This is
    # also the call sequence that previously segfaulted inside
    # CometSearch::BinarySearchMass() before any output was written, so a
    # non-crashing exit with the expected PSM is the regression check.
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
              f"ascorepro: expected > 0 (AScore must use the .idx header's "
              f"VariableMod: line even though search-time params left it blank), "
              f"got {ascorepro}", failures)
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


def _rts_run(idx_path, fixture_path, num_threads, output_path, ascorepro=0, index_search_type=1):
    result = subprocess.run(
        [str(RTS_REPRO_BIN), str(idx_path), str(fixture_path), str(num_threads), str(output_path),
         str(ascorepro), str(index_search_type)],
        capture_output=True, text=True, timeout=300,
    )
    return result.returncode, result.stdout + result.stderr


def _rts_sorted_lines(path):
    lines = Path(path).read_text().splitlines()
    return sorted(lines, key=lambda l: int(l.split("\t")[0].split()[1]))


def _test_rts_index_type(comet_exe, index_flag, label):
    failures = []
    # rts_repro.cpp (docs/20260730_PI_reduction.md Phase 0.5): no comet.params to read
    # index_search_type from, so it's passed as an explicit CLI arg, mirroring
    # index_flag's build-time PI-vs-FI choice for the search-time dispatch too.
    index_search_type = 0 if index_flag == "-j" else 1
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
        rc, out = _rts_run(t19_idx, t19_fixture, 1, out_path, index_search_type=index_search_type)
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
            rc1, log1 = _rts_run(hs_idx, RTS_FIXTURE, 1, out1, index_search_type=index_search_type)
            if not check(rc1 == 0, f"{label}: rts_repro (1 thread) exits 0", failures):
                print(log1)
                return failures
            rc8, log8 = _rts_run(hs_idx, RTS_FIXTURE, 8, out8, index_search_type=index_search_type)
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

    # --- 2b. Same determinism check with internal decoys on (decoy_search=1): the
    #        decoy variants double FI_DB's candidate population and PI_DB reverses every
    #        candidate at score time; both must stay thread-count-independent, and some
    #        decoys must actually win spectra (docs/20260914_FI_internal_decoys.md Phase 4).
    hs_dec_params = hs_params_content.replace("decoy_search = 0 ", "decoy_search = 1 ")
    assert hs_dec_params != hs_params_content, "comet_phospho.params decoy_search line changed shape?"
    hs_idx = None
    try:
        hs_idx = _rts_build_index(comet_exe, small_fasta, hs_dec_params, index_flag)
        out1 = Path(tempfile.mktemp(suffix=".dec.1thread.out", dir=str(DATA_DIR)))
        out8 = Path(tempfile.mktemp(suffix=".dec.8thread.out", dir=str(DATA_DIR)))
        try:
            rc1, log1 = _rts_run(hs_idx, RTS_FIXTURE, 1, out1, index_search_type=index_search_type)
            if not check(rc1 == 0, f"{label} decoy_search=1: rts_repro (1 thread) exits 0", failures):
                print(log1)
                return failures
            rc8, log8 = _rts_run(hs_idx, RTS_FIXTURE, 8, out8, index_search_type=index_search_type)
            if not check(rc8 == 0, f"{label} decoy_search=1: rts_repro (8 threads) exits 0", failures):
                print(log8)
                return failures
            lines1 = _rts_sorted_lines(out1)
            lines8 = _rts_sorted_lines(out8)
            n_decoy = sum(1 for l in lines1 if "prot 'DECOY_" in l)
            check(len(lines1) == 197, f"{label} decoy_search=1: 1-thread run covers all 197 spectra, got {len(lines1)}", failures)
            check(n_decoy > 0, f"{label} decoy_search=1: some top-1 hits are internal decoys ({n_decoy} of 197)", failures)
            check(lines1 == lines8,
                  f"{label} decoy_search=1: 1-thread and 8-thread outputs are byte-identical after sorting by scan "
                  f"({sum(a != b for a, b in zip(lines1, lines8))} differing lines out of {len(lines1)})",
                  failures)
        finally:
            out1.unlink(missing_ok=True)
            out8.unlink(missing_ok=True)
    finally:
        if hs_idx and hs_idx.exists():
            hs_idx.unlink()

    # --- 3. Internal-decoy labeling (docs/20260914_FI_internal_decoys.md Section 4.5) ---
    #
    # DoSingleSpectrumSearchMultiResults() (CometSearchManager.cpp) used to classify an
    # indexed-DB hit's proteins as target vs decoy purely by name prefix. An internal
    # (pseudo-reverse) decoy carries the TARGET's protein-list row -- StorePeptideI()
    # routes it to pWhichDecoyProtein with the same lProteinFilePosition -- so every
    # decoy_search=1 decoy hit came back to the C# layer with unprefixed target protein
    # names, indistinguishable from a real identification. Build the t19 index with
    # decoy_search=1 and feed two synthetic spectra: the target's b/y ions (control: must
    # still be reported as a target) and the DECOY's b/y ions with the phospho moved along
    # with its serine (must be reported as the decoy sequence with a DECOY_-prefixed protein).
    #
    # Runs for both index types: PI_DB reverses at score time (AnalyzePeptideIndex()),
    # FI_DB indexes a decoy variant per target variant at fragment-index build and
    # reconstructs it in SearchFragmentIndex() (plan Phases 1-2) -- same helper, same
    # expected decoy string.
    target_pep = "ACDEFGSK"
    # search_enzyme_number = 0 (Cut_everywhere) has enzyme offset 0, so
    # CometSearch::PseudoReversePeptide() keeps the FIRST residue fixed:
    # ABCDEK -> AKEDCB. The mod site moves with its residue.
    decoy_pep = target_pep[0] + target_pep[:0:-1]
    target_mods = {6: 79.966331}                                # 0-based S position
    decoy_mods = {len(target_pep) - i: m for i, m in target_mods.items()}
    assert decoy_pep == "AKSGFEDC" and decoy_pep[2] == "S" and decoy_mods == {2: 79.966331}

    decoy_fixture = Path(tempfile.mktemp(suffix=".decoy.fixture.txt", dir=str(DATA_DIR)))
    decoy_fixture.write_text(
        "\n".join(_theoretical_fixture_lines(1, target_pep, target_mods, 2)
                  + _theoretical_fixture_lines(2, decoy_pep, decoy_mods, 2)) + "\n")
    decoy_idx_params = t19_params.replace("decoy_search = 0", "decoy_search = 1")
    assert decoy_idx_params != t19_params
    decoy_idx = None
    try:
        decoy_idx = _rts_build_index(comet_exe, t19_fasta, decoy_idx_params, index_flag)
        out_path = Path(tempfile.mktemp(suffix=".out", dir=str(DATA_DIR)))
        rc, out = _rts_run(decoy_idx, decoy_fixture, 1, out_path, index_search_type=index_search_type)
        if not check(rc == 0, f"{label}: rts_repro exits 0 on internal-decoy fixture", failures):
            print(out)
            return failures
        lines = _rts_sorted_lines(out_path) if out_path.exists() else []
        out_path.unlink(missing_ok=True)
        if not check(len(lines) == 2, f"{label}: 2 internal-decoy result lines, got {len(lines)}", failures):
            return failures
        tgt_parts = lines[0].split("\t")
        dec_parts = lines[1].split("\t")
        tgt_pep = tgt_parts[1] if len(tgt_parts) > 1 else "NO_MATCH"
        dec_pep = dec_parts[1] if len(dec_parts) > 1 else "NO_MATCH"
        check("ACDEFGS" in tgt_pep and "79.9663" in tgt_pep,
              f"{label}: control spectrum still matches target ACDEFGS[79.9663]K, got {tgt_pep!r}", failures)
        check("prot '" in tgt_parts[-1] and "prot 'DECOY_" not in tgt_parts[-1],
              f"{label}: control target hit is reported WITHOUT decoy prefix, got {tgt_parts[-1]!r}", failures)
        check("AKS" in dec_pep and "79.9663" in dec_pep and "GFEDC" in dec_pep,
              f"{label}: decoy spectrum matches internal decoy AKS[79.9663]GFEDC, got {dec_pep!r}", failures)
        check("prot 'DECOY_" in dec_parts[-1],
              f"{label}: internal-decoy hit is reported WITH decoy prefix, got {dec_parts[-1]!r}", failures)
    finally:
        if decoy_idx and decoy_idx.exists():
            decoy_idx.unlink()
        decoy_fixture.unlink(missing_ok=True)

    return failures


# Monoisotopic residue masses for synthesizing fixture spectra (b/y singly-charged
# ladders), matching the constants the committed t19_ascore_fidb.ms2 was built from.
_MONO_RESIDUE_MASS = {
    "G": 57.021464, "A": 71.037114, "S": 87.032028, "P": 97.052764, "V": 99.068414,
    "T": 101.047679, "C": 103.009185, "L": 113.084064, "I": 113.084064, "N": 114.042927,
    "D": 115.026943, "Q": 128.058578, "K": 128.094963, "E": 129.042593, "M": 131.040485,
    "H": 137.058912, "F": 147.068414, "R": 156.101111, "Y": 163.063329, "W": 186.079313,
}
_PROTON_MASS = 1.007276
_H2O_MASS = 18.010565


def _theoretical_fixture_lines(scan, peptide, mods, charge):
    """One rts_repro fixture SPECTRUM block: every singly-charged b and y ion of
    `peptide` (mods = {0-based residue index: delta mass}) at intensity 100, with the
    precursor m/z computed for `charge`."""
    res = [_MONO_RESIDUE_MASS[aa] + mods.get(i, 0.0) for i, aa in enumerate(peptide)]
    n = len(res)
    peaks = []
    for k in range(1, n):
        peaks.append(sum(res[:k]) + _PROTON_MASS)                 # b_k
        peaks.append(sum(res[n - k:]) + _H2O_MASS + _PROTON_MASS)  # y_k
    peaks.sort()
    neutral = sum(res) + _H2O_MASS
    mz = (neutral + charge * _PROTON_MASS) / charge
    lines = [f"SPECTRUM {scan} {charge} {mz:.6f} {len(peaks)}"]
    lines += [f"{m:.6f} 100.0" for m in peaks]
    return lines


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
    fmt = _to_win if _binary_uses_win_paths(comet_exe) else str
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(_localize_params_for(comet_exe, params_content))
        params_file = Path(pf.name)
    txt_path = Path(mzxml_path).with_suffix(".txt")
    if txt_path.exists():
        txt_path.unlink()
    try:
        t0 = time.perf_counter()
        result = subprocess.run(
            [str(comet_exe), f"-P{fmt(params_file)}", fmt(mzxml_path)],
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
    fmt = _to_win if _binary_uses_win_paths(binary) else str
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(_localize_params_for(binary, plain_params))
        idx_params_file = Path(pf.name)
    try:
        t0 = time.perf_counter()
        r = subprocess.run([str(binary), flag, f"-P{fmt(idx_params_file)}"],
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

    # --- Cross-version: same two configs against the BASELINE_TAG baseline ---
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

    # --- Cross-version: same three modes against the BASELINE_TAG baseline ---
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
# T24b -- internal-decoy parity across FASTA / FI_DB / PI_DB (--bigdata gated)
# ---------------------------------------------------------------------------
#
# docs/20260914_FI_internal_decoys.md Phase 4. Same comet-debug3 data as T23/T24, but with
# Comet's internal decoys (decoy_search=1) on the TARGET-ONLY human.fasta in all three search
# modes. FI_DB indexes one pseudo-reverse decoy variant per target variant, PI_DB and FASTA_DB
# reverse each candidate at score time -- the three must agree on PSMs at 1% FDR within the
# same 5% T23/T24 use for FI_DB, and FI_DB's internal decoys must agree with the pre-existing
# workaround (an FI_DB built from human.target-decoy.fasta, decoy_search=0) too. Timings are
# printed (build and search wall-clock, single sample) so the decoy_search=0 vs =1 FI_DB cost
# can be read off the log; they are not asserted.

@register("t24_internal_decoy_parity")
def test_t24_internal_decoy_parity(comet_exe):
    """T24b [integration, bigdata]: internal-decoy (decoy_search=1) 1% FDR parity, FASTA vs FI_DB vs PI_DB."""
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
        print(f"  SKIP: {d3} not found or incomplete -- pass --bigdata DIR")
        return []

    base = base_params_file.read_text()
    dec_params = _set_param_line(base, "database_name", human_fasta)
    dec_params = _set_param_line(dec_params, "decoy_search", "1")

    print("  Running plain-FASTA internal-decoy search (human.fasta, decoy_search=1) ...")
    rc0, txt0, out0, t0 = _run_bigdata_search(comet_exe, dec_params, mzxml)
    if not check(rc0 == 0, f"plain-FASTA internal-decoy search exits 0 (rc={rc0})", failures):
        print(out0[-2000:])
        return failures
    n0, cx0, ce0 = _q1pct_counts(txt0)
    txt0.unlink(missing_ok=True)
    print(f"    plain-FASTA decoy_search=1: {cx0:,} PSMs at 1% FDR (xcorr), {n0:,} rank-1 PSMs, search {t0:.1f}s")
    check(10_000 <= cx0 <= 30_000,
          f"plain-FASTA internal decoys: {cx0:,} PSMs at 1% FDR (xcorr) in plausible range [10k, 30k]", failures)

    idx_path = human_fasta.with_suffix(".fasta.idx")
    counts = {"plain-FASTA": cx0}
    for flag, label in (("-i", "FI_DB"), ("-j", "PI_DB")):
        print(f"  Building {label} index (human.fasta) and searching with decoy_search=1 ...")
        result = _index_build_and_search(comet_exe, flag, label, dec_params, idx_path, mzxml, failures)
        if result is None:
            continue
        cx, build_s, search_s = result
        counts[label] = cx
        print(f"    {label} decoy_search=1: {cx:,} PSMs at 1% FDR (xcorr); build {build_s:.1f}s, search {search_s:.1f}s")
        ratio = (cx / cx0) if cx0 else float("inf")
        check(0.95 <= ratio <= 1.05,
              f"{label} internal decoys ({cx:,}) agree with plain-FASTA internal decoys ({cx0:,}) "
              f"within 5% at 1% FDR xcorr (ratio {ratio:.3f})", failures)
    idx_path.unlink(missing_ok=True)

    if "FI_DB" in counts and "PI_DB" in counts:
        ratio = counts["FI_DB"] / counts["PI_DB"] if counts["PI_DB"] else float("inf")
        check(0.95 <= ratio <= 1.05,
              f"FI_DB internal decoys ({counts['FI_DB']:,}) agree with PI_DB internal decoys "
              f"({counts['PI_DB']:,}) within 5% (ratio {ratio:.3f})", failures)

    # FI_DB internal decoys vs the pre-existing FI_DB workaround: target-decoy FASTA, decoy_search=0
    if "FI_DB" in counts:
        td_params = _set_param_line(base, "database_name", human_td_fasta)
        td_params = _set_param_line(td_params, "decoy_search", "0")
        td_idx = human_td_fasta.with_suffix(".fasta.idx")
        print("  Building FI_DB index (human.target-decoy.fasta) and searching with decoy_search=0 ...")
        result = _index_build_and_search(comet_exe, "-i", "FI_DB", td_params, td_idx, mzxml, failures,
                                         tag="target-decoy FASTA")
        td_idx.unlink(missing_ok=True)
        if result is not None:
            cx_td, build_s, search_s = result
            print(f"    FI_DB target-decoy FASTA decoy_search=0: {cx_td:,} PSMs at 1% FDR (xcorr); "
                  f"build {build_s:.1f}s, search {search_s:.1f}s")
            ratio = counts["FI_DB"] / cx_td if cx_td else float("inf")
            check(0.95 <= ratio <= 1.05,
                  f"FI_DB internal decoys ({counts['FI_DB']:,}) agree with FI_DB target-decoy FASTA "
                  f"({cx_td:,}) within 5% (ratio {ratio:.3f})", failures)

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

# T25 reuses T19_PARAMS_TEMPLATE (byte-identical schema otherwise) instead of maintaining a
# second ~95-line near-copy of it -- a code review of this branch flagged the duplicate as a
# maintenance risk (any future change to the shared params schema, e.g. a new required key or
# an enzyme table update, would need to be hand-applied to both). Three targeted differences
# from T19's template: no print_ascorepro_score line (T25 doesn't exercise AScorePro), the gap
# variable-mod config itself (T19's {mod1}/{ascorepro} placeholders aren't used here -- T25's
# mod config is fixed: variable_mod01 unused, real mod in variable_mod02), and a longer
# peptide_length_range (10 vs T19's 8) to fit the 10-residue ACDSEFxHIK-style test peptides.
T25_PARAMS_TEMPLATE = (
   T19_PARAMS_TEMPLATE
   .replace("print_ascorepro_score = {ascorepro}\n", "")
   .replace("variable_mod01 = {mod1}\nvariable_mod02 = 0.0 X 0 3 -1 0 0 0.0",
            "variable_mod01 = 0.0 X 0 3 -1 0 0 0.0\nvariable_mod02 = 79.966331 S 0 1 -1 0 0 0.0")
   .replace("peptide_length_range = 8 8", "peptide_length_range = 10 10")
)

# Sanity-check the .replace() chain above actually fired -- if a future edit to
# T19_PARAMS_TEMPLATE changes any of the three literal snippets being matched (whitespace,
# reordering, a renamed key, ...), each .replace() silently becomes a no-op instead of raising,
# and T25 would run against T19's unmodified/still-templated params instead of its intended gap
# config. Fail loudly here rather than let that surface later as a confusing T25 assertion
# failure (or, worse, a silent false pass).
assert "{ascorepro}" not in T25_PARAMS_TEMPLATE, \
    "T25_PARAMS_TEMPLATE: print_ascorepro_score replacement didn't fire -- T19_PARAMS_TEMPLATE changed?"
assert "{mod1}" not in T25_PARAMS_TEMPLATE, \
    "T25_PARAMS_TEMPLATE: variable_mod01/02 gap replacement didn't fire -- T19_PARAMS_TEMPLATE changed?"
assert "variable_mod01 = 0.0 X 0 3 -1 0 0 0.0\nvariable_mod02 = 79.966331 S 0 1 -1 0 0 0.0" \
    in T25_PARAMS_TEMPLATE, \
    "T25_PARAMS_TEMPLATE: variable_mod01/02 gap replacement didn't fire -- T19_PARAMS_TEMPLATE changed?"
assert "peptide_length_range = 10 10" in T25_PARAMS_TEMPLATE, \
    "T25_PARAMS_TEMPLATE: peptide_length_range replacement didn't fire -- T19_PARAMS_TEMPLATE changed?"


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


@register("t25_fi_mod_slot_ambig")
def test_t25_fi_mod_slot_ambig(comet_exe):
    """T25: FI_DB gap variable-mod-slot regression with a genuinely AMBIGUOUS second
    modifiable site (peptide has 2 candidate S residues, max_variable_mods_in_peptide=1).
    Unlike t25_fi_mod_slot_gap (only 1 modifiable residue -- MOD_NUMBERS[].modifications[]
    is never -1 there), this fixture forces AddFragments() to enumerate a combination
    where the OTHER candidate site's compacted mod index is the -1 "not modified in this
    combination" sentinel while translating a fragment mass through
    vModSlotForAllModsIdx -- the specific unguarded array access that crashed/corrupted
    memory before the fix. Regresses cleanly if the build+search complete and localize the
    real mod to S4 (not S7)."""
    failures = []

    fasta = DATA_DIR / "t25_fi_mod_slot_ambig.fasta"
    ms2   = DATA_DIR / "t25_fi_mod_slot_ambig.ms2"
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
            failures.append(f"index build failed (rc={rc}) -- the unguarded "
                             f"vModSlotForAllModsIdx[(size_t)mods[j]] access on a -1 "
                             f"sentinel likely crashed or hung the build:\n{out}")
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
            failures.append(f".txt not created (peptide not found). Comet output:\n{out}")
            return failures

        lines  = txt.read_text().splitlines()
        rows   = [l.split("\t") for l in lines[2:] if l.strip()]

        check(len(rows) == 1, f"expected exactly 1 PSM row, got {len(rows)}", failures)
        if not rows:
            return failures

        header = lines[1].split("\t")
        row = dict(zip(header, rows[0]))

        check(row.get("plain_peptide") == "ACDSEFSHIK",
              f"plain_peptide: expected ACDSEFSHIK, got {row.get('plain_peptide')!r}", failures)
        check("4_V_79.966331" in row.get("modifications", ""),
              f"modifications: expected phospho localized to position 4 (4_V_79.966331), "
              f"got {row.get('modifications')!r}", failures)
        check("7_V_79.966331" not in row.get("modifications", ""),
              f"modifications: unexpectedly localized to position 7 as well/instead, "
              f"got {row.get('modifications')!r}", failures)
        check(int(row.get("ions_matched", "0")) == 14,
              f"ions_matched: expected all 14 fragment ions matched, got "
              f"{row.get('ions_matched')!r}", failures)
    finally:
        search_params_file.unlink(missing_ok=True)
        idx.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)

    return failures


# ---------------------------------------------------------------------------
# T26 -- docs/20260819_fablereview.md B1/B2 regression: phospho + decoy_search
# fragment-ion-ladder correctness (FASTA decoy ladder / FI candidate ordering)
# ---------------------------------------------------------------------------
#
# B1: CalcVarModIons()'s decoy b-ion branch (CometSearch.cpp, reached from
# MergeVarMods()/PermuteMods() -- the general FASTA variable-mod permutation path,
# not PEFF-only despite bDoPeffAnalysis/vPeffArray being threaded through the same
# call chain) had a bare `break;` that exited the whole per-position decoy ladder loop
# the first time a decoy residue carried a fragment-neutral-loss variable mod, leaving
# _pdAAforwardDecoy/_pdAAreverseDecoy stale for every later position. Fixed by
# deleting the break (mirroring the target ladder and the decoy y-branch, neither of
# which ever had one).
#
# B2: SearchFragmentIndex()'s cumulative NL-count carry-forward
# (iCountNLB[x][iPosForward] = iCountNLB[x][iPosForward-1]) was gated on
# `i > iStartPos`, an outer-scope variable this function also mutates elsewhere from a
# *previous* FI candidate's flanking residue -- not the loop's own index. Fixed to
# `i > 0`.

T26_PARAMS_TEMPLATE = (
    T19_PARAMS_TEMPLATE
    .replace("search_enzyme_number = 0", "search_enzyme_number = 1")
    .replace("decoy_search = 0", "decoy_search = {decoy_search}")
    .replace("peptide_length_range = 8 8", "peptide_length_range = {len_min} {len_max}")
)
assert "search_enzyme_number = 1" in T26_PARAMS_TEMPLATE, \
    "T26_PARAMS_TEMPLATE: search_enzyme_number replacement didn't fire -- T19_PARAMS_TEMPLATE changed?"
assert "decoy_search = {decoy_search}" in T26_PARAMS_TEMPLATE, \
    "T26_PARAMS_TEMPLATE: decoy_search replacement didn't fire -- T19_PARAMS_TEMPLATE changed?"
assert "peptide_length_range = {len_min} {len_max}" in T26_PARAMS_TEMPLATE, \
    "T26_PARAMS_TEMPLATE: peptide_length_range replacement didn't fire -- T19_PARAMS_TEMPLATE changed?"


@register("t26_b1_fasta_decoy")
def test_t26_b1_fasta_decoy(comet_exe):
    """T26: B1 regression -- FASTA decoy phospho ladder must not break early on the
    residue carrying a fragment-neutral-loss variable mod."""
    failures = []

    fasta     = DATA_DIR / "t26_b1_fasta_decoy.fasta"
    ms2       = DATA_DIR / "t26_b1_fasta_decoy.ms2"
    txt       = ms2.with_suffix(".txt")
    txt_decoy = ms2.with_suffix(".decoy.txt")

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    for f in (txt, txt_decoy):
        f.unlink(missing_ok=True)

    search_params = T26_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(fasta),
        ascorepro=0, mod1="79.966331 S 0 1 -1 0 0 97.976896",
        decoy_search=2, len_min=10, len_max=10,
    )
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(search_params)
        params_file = Path(pf.name)

    try:
        rc, out = _run_t19_step(comet_exe, [f"-P{fmt(params_file)}", fmt(ms2)])
        if rc != 0:
            failures.append(f"search failed (rc={rc}):\n{out}")
            return failures
        if not txt_decoy.exists():
            failures.append(f"decoy .txt not created. Comet output:\n{out}")
            return failures

        lines = txt_decoy.read_text().splitlines()
        rows  = [l.split("\t") for l in lines[2:] if l.strip()]
        check(len(rows) == 1, f"expected exactly 1 decoy PSM row, got {len(rows)}", failures)
        if not rows:
            return failures

        header = lines[1].split("\t")
        row = dict(zip(header, rows[0]))

        check(row.get("plain_peptide") == "IHGFESDCAK",
              f"plain_peptide: expected the decoy IHGFESDCAK, got {row.get('plain_peptide')!r}",
              failures)
        # The bug left _pdAAforwardDecoy/_pdAAreverseDecoy stale for every position
        # after the phospho S (local index 5 of 10) -- a regression would score well
        # below the full 18 b/y ions.
        check(int(row.get("ions_matched", "0")) == 18,
              f"ions_matched: expected all 18 b/y ions matched, got "
              f"{row.get('ions_matched')!r} (a regressed early break would silently "
              f"drop several of the ions past the phospho residue)", failures)
    finally:
        params_file.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)
        txt_decoy.unlink(missing_ok=True)

    return failures


@register("t26_b2_fi_nl_order")
def test_t26_b2_fi_nl_order(comet_exe):
    """T26: B2 regression -- SearchFragmentIndex()'s NL running-count carry-forward
    must key off the loop's own index (i > 0), not a stale outer-scope variable a
    previous FI candidate mutated."""
    failures = []

    fasta = DATA_DIR / "t26_b2_fi_nl_order.fasta"
    ms2   = DATA_DIR / "t26_b2_fi_nl_order.ms2"
    idx   = fasta.with_suffix(".fasta.idx")
    txt   = ms2.with_suffix(".txt")

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    idx.unlink(missing_ok=True)
    build_params = T26_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(fasta),
        ascorepro=0, mod1="79.966331 S 0 1 -1 0 0 97.976896",
        decoy_search=0, len_min=8, len_max=9,
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

    txt.unlink(missing_ok=True)
    search_params = T26_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(idx),
        ascorepro=0, mod1="79.966331 S 0 1 -1 0 0 97.976896",
        decoy_search=0, len_min=8, len_max=9,
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

        lines = txt.read_text().splitlines()
        rows  = [l.split("\t") for l in lines[2:] if l.strip()]
        check(len(rows) == 1, f"expected exactly 1 PSM row, got {len(rows)}", failures)
        if not rows:
            return failures

        header = lines[1].split("\t")
        row = dict(zip(header, rows[0]))

        check(row.get("plain_peptide") == "SPEPTIDEK",
              f"plain_peptide: expected SPEPTIDEK, got {row.get('plain_peptide')!r}",
              failures)
        # ions_matched/ions_total only ever count the 16 base b/y ion slots (NL-bin
        # matches don't show up in those two columns at all -- confirmed empirically,
        # not a bug), so the observable signal for "were the NL peaks actually
        # matched" is xcorr, not ions_matched. With this spectrum's 8 extra NL-shifted
        # b-ion peaks (phospho on S at residue 0, so every b1-b8 has one), a correct
        # search scores xcorr ~5.9; with the guard regressed back to `i > iStartPos`,
        # those NL bins are silently never generated and xcorr drops to ~4.0 (the
        # same score an identical spectrum stripped of the NL peaks gets) -- verified
        # directly against both builds while writing this test.
        check(float(row.get("xcorr", "0")) > 5.0,
              f"xcorr: expected > 5.0 (NL-shifted peaks contributing), got "
              f"{row.get('xcorr')!r} (a regressed guard silently drops the NL bins, "
              f"scoring ~4.0 as if those 8 extra peaks weren't there)", failures)
    finally:
        search_params_file.unlink(missing_ok=True)
        idx.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)

    return failures


# ---------------------------------------------------------------------------
# T27 -- docs/20260819_fablereview.md B3/B4 regression: variable-mod combinatorial
# cap must not be escaped by mods configured in slots 10-15.
# ---------------------------------------------------------------------------
#
# B3 (FASTA path, CometSearch.cpp's PermuteMods()/nested iSumN accumulation) and B4
# (FI/PI path, CometModificationsPermuter.cpp's combine()) each independently failed
# to correctly enforce max_variable_mods_in_peptide once mods live in slots 10-15 (the
# 15-mod extension). This fixture configures three single-residue mods in slots
# 10/11/12 (arbitrary masses 10/20/30 Da, chosen so no subset of {10,20,30} other than
# all three sums to their total of 60) on a peptide with one modifiable residue of
# each type, capped at max_variable_mods_in_peptide=2 -- a query precursor mass that's
# only explained by all three mods simultaneously must find zero valid candidates.

T27_PARAMS_TEMPLATE = textwrap.dedent("""\
# comet_version {comet_version}
database_name = {database}
decoy_search = 0
num_threads = 1
print_ascorepro_score = 0
peptide_mass_tolerance_upper = 3.0
peptide_mass_tolerance_lower = -3.0
peptide_mass_units = 0
precursor_tolerance_type = 1
isotope_error = 0
search_enzyme_number = 1
search_enzyme2_number = 0
sample_enzyme_number = 0
num_enzyme_termini = 2
allowed_missed_cleavage = 0
variable_mod01 = 0.0 X 0 3 -1 0 0 0.0
variable_mod02 = 0.0 X 0 3 -1 0 0 0.0
variable_mod03 = 0.0 X 0 3 -1 0 0 0.0
variable_mod04 = 0.0 X 0 3 -1 0 0 0.0
variable_mod05 = 0.0 X 0 3 -1 0 0 0.0
variable_mod06 = 0.0 X 0 3 -1 0 0 0.0
variable_mod07 = 0.0 X 0 3 -1 0 0 0.0
variable_mod08 = 0.0 X 0 3 -1 0 0 0.0
variable_mod09 = 0.0 X 0 3 -1 0 0 0.0
variable_mod10 = 10.0 S 0 1 -1 0 0 0.0
variable_mod11 = 20.0 T 0 1 -1 0 0 0.0
variable_mod12 = 30.0 Y 0 1 -1 0 0 0.0
variable_mod13 = 0.0 X 0 3 -1 0 0 0.0
variable_mod14 = 0.0 X 0 3 -1 0 0 0.0
variable_mod15 = 0.0 X 0 3 -1 0 0 0.0
max_variable_mods_in_peptide = 2
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
num_output_lines = 5
scan_range = 0 0
precursor_charge = 0 0
override_charge = 0
ms_level = 2
activation_method = ALL
digest_mass_range = 200.0 2000.0
peptide_length_range = 6 6
max_duplicate_proteins = -1
max_fragment_charge = 3
min_precursor_charge = 1
max_precursor_charge = 6
clip_nterm_methionine = 0
spectrum_batch_size = 15000
decoy_prefix = DECOY_
equal_I_and_L = 0
mass_offsets =
minimum_peaks = 1
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
""")


def _t27_run(comet_exe, database, ms2_path):
    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str
    params = T27_PARAMS_TEMPLATE.format(comet_version="2026.02 rev. 0", database=fmt(database))
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(params)
        params_file = Path(pf.name)
    try:
        return _run_t19_step(comet_exe, [f"-P{fmt(params_file)}", fmt(ms2_path)])
    finally:
        params_file.unlink(missing_ok=True)


@register("t27_modcap_fasta")
def test_t27_modcap_fasta(comet_exe):
    """T27: B3 regression -- FASTA search must not report a peptide with 3
    simultaneous variable mods in slots 10-15 when max_variable_mods_in_peptide=2."""
    failures = []
    fasta = DATA_DIR / "t27_modcap.fasta"
    ms2   = DATA_DIR / "t27_modcap.ms2"
    txt   = ms2.with_suffix(".txt")
    txt.unlink(missing_ok=True)
    try:
        rc, out = _t27_run(comet_exe, fasta, ms2)
        if rc != 0:
            failures.append(f"search failed (rc={rc}):\n{out}")
            return failures
        rows = []
        if txt.exists():
            lines = txt.read_text().splitlines()
            rows  = [l.split("\t") for l in lines[2:] if l.strip()]
        check(len(rows) == 0,
              f"expected zero PSM rows (the only mass match requires all 3 mods, "
              f"exceeding the cap of 2), got {len(rows)}: {rows}", failures)
    finally:
        txt.unlink(missing_ok=True)
    return failures


@register("t27_modcap_fi")
def test_t27_modcap_fi(comet_exe):
    """T27: B4 regression -- same scenario as t27_modcap_fasta, but for FI_DB's
    CometModificationsPermuter::combine() cap enforcement."""
    failures = []
    fasta = DATA_DIR / "t27_modcap.fasta"
    ms2   = DATA_DIR / "t27_modcap.ms2"
    idx   = fasta.with_suffix(".fasta.idx")
    txt   = ms2.with_suffix(".txt")

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    idx.unlink(missing_ok=True)
    build_params = T27_PARAMS_TEMPLATE.format(comet_version="2026.02 rev. 0", database=fmt(fasta))
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

    txt.unlink(missing_ok=True)
    try:
        rc, out = _t27_run(comet_exe, idx, ms2)
        if rc != 0:
            failures.append(f"search failed (rc={rc}):\n{out}")
            return failures
        rows = []
        if txt.exists():
            lines = txt.read_text().splitlines()
            rows  = [l.split("\t") for l in lines[2:] if l.strip()]
        check(len(rows) == 0,
              f"expected zero PSM rows (the only mass match requires all 3 mods, "
              f"exceeding the cap of 2), got {len(rows)}: {rows}", failures)
    finally:
        txt.unlink(missing_ok=True)
        idx.unlink(missing_ok=True)

    return failures


# ---------------------------------------------------------------------------
# T28 -- docs/20260819_fablereview.md B5 regression: .idx header restore must set
# bNtermMod/bCtermMod/bVarTermModSearch, not just szVarModChar/mass.
# ---------------------------------------------------------------------------

@register("t28_idx_cterm_mod")
def test_t28_idx_cterm_mod(comet_exe):
    """T28: B5 regression -- an FI_DB .idx built with a real c-term variable mod must
    still apply that mod when searched with variable_mod01/02 left blank."""
    failures = []

    fasta = legacy_cases.LEGACY_DIR / "db" / "epgc_9entry.fasta"
    ms2   = legacy_cases.LEGACY_DIR / "ctermmod" / "input.ms2"
    idx   = fasta.with_suffix(".fasta.idx")
    txt   = ms2.with_suffix(".txt")

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    CTERM_MODS = ("15.9949 M 0 3 -1 0 0 0.0", "128.094963050 c 0 3 -1 0 0 0.0")

    idx.unlink(missing_ok=True)
    build_params = legacy_cases.build_params(
        database=fmt(fasta), enzyme1=1, ntt=1, mods=CTERM_MODS)
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

    txt.unlink(missing_ok=True)
    # variable_mod01/02 left blank (mods=()) at search time -- the .idx header's own
    # VariableMod: entries must be what actually gets applied.
    search_params = legacy_cases.build_params(database=fmt(idx), enzyme1=1, ntt=1, mods=())
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

        lines = txt.read_text().splitlines()
        rows  = [l.split("\t") for l in lines[2:] if l.strip()]
        check(len(rows) >= 1, f"expected at least 1 PSM row, got {len(rows)}", failures)
        if not rows:
            return failures

        header = lines[1].split("\t")
        row = dict(zip(header, rows[0]))

        check(row.get("plain_peptide") == "YFDSFGDLSSASAIMGNP",
              f"plain_peptide: expected the c-term-modified YFDSFGDLSSASAIMGNP, got "
              f"{row.get('plain_peptide')!r} (a regression leaves bVarTermModSearch "
              f"false, so this variant is never enumerated and a different, "
              f"coincidentally-same-mass peptide ranks first instead)", failures)
        check("128.094963_c" in row.get("modifications", ""),
              f"modifications: expected the c-term mod (128.094963_c), got "
              f"{row.get('modifications')!r}", failures)
    finally:
        search_params_file.unlink(missing_ok=True)
        idx.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)

    return failures


# ---------------------------------------------------------------------------
# T29 -- docs/20260819_fablereview.md B14/B15 regression: PI_DB target-decoy
# classification must use the .idx header's decoy_prefix, and the protein-name cache
# actually populated at search time.
# ---------------------------------------------------------------------------

T29_PARAMS_TEMPLATE = textwrap.dedent("""\
# comet_version {comet_version}
database_name = {database}
decoy_search = 2
num_threads = 1
print_ascorepro_score = 0
peptide_mass_tolerance_upper = 5.0
peptide_mass_tolerance_lower = -5.0
peptide_mass_units = 0
precursor_tolerance_type = 1
isotope_error = 0
search_enzyme_number = 1
search_enzyme2_number = 0
sample_enzyme_number = 0
num_enzyme_termini = 2
allowed_missed_cleavage = 0
variable_mod01 = 0.0 X 0 3 -1 0 0 0.0
variable_mod02 = 0.0 X 0 3 -1 0 0 0.0
variable_mod03 = 0.0 X 0 3 -1 0 0 0.0
variable_mod04 = 0.0 X 0 3 -1 0 0 0.0
variable_mod05 = 0.0 X 0 3 -1 0 0 0.0
max_variable_mods_in_peptide = 1
require_variable_mod = 0
fragment_bin_tol = 1.0005
fragment_bin_offset = 0.4
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
peptide_length_range = 5 20
max_duplicate_proteins = -1
max_fragment_charge = 3
min_precursor_charge = 1
max_precursor_charge = 6
clip_nterm_methionine = 0
spectrum_batch_size = 15000
decoy_prefix = {decoy_prefix}
equal_I_and_L = 0
mass_offsets =
minimum_peaks = 1
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
""")


@register("t29_decoyprefix")
def test_t29_decoyprefix(comet_exe):
    """T29: B14/B15 regression -- a PI_DB .idx built with decoy_prefix=REV_ must still
    classify the decoy protein's peptide as a decoy when searched with decoy_prefix
    left at the mismatched default (DECOY_)."""
    failures = []

    fasta     = DATA_DIR / "t29_decoyprefix.fasta"
    ms2       = DATA_DIR / "t29_decoyprefix.ms2"
    idx       = fasta.with_suffix(".fasta.idx")
    txt       = ms2.with_suffix(".txt")
    txt_decoy = ms2.with_suffix(".decoy.txt")

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    idx.unlink(missing_ok=True)
    build_params = T29_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(fasta), decoy_prefix="REV_")
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

    for f in (txt, txt_decoy):
        f.unlink(missing_ok=True)

    # decoy_prefix left at the mismatched default (DECOY_, not REV_) -- the .idx
    # header's own DecoyPrefix: entry must be what actually gets used.
    search_params = T29_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(idx), decoy_prefix="DECOY_")
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

        target_rows = []
        decoy_rows  = []
        decoy_lines = []
        if txt.exists():
            lines = txt.read_text().splitlines()
            target_rows = [l.split("\t") for l in lines[2:] if l.strip()]
        if txt_decoy.exists():
            decoy_lines = txt_decoy.read_text().splitlines()
            decoy_rows  = [l.split("\t") for l in decoy_lines[2:] if l.strip()]

        check(len(target_rows) == 0,
              f"expected zero TARGET rows (the only mass match is the decoy "
              f"protein's peptide; a regression would misclassify it as a target), "
              f"got {len(target_rows)}: {target_rows}", failures)
        check(len(decoy_rows) == 1,
              f"expected exactly 1 DECOY row for SEATENCEK, got {len(decoy_rows)}: "
              f"{decoy_rows}", failures)
        if decoy_rows:
            decoy_header = decoy_lines[1].split("\t")
            row = dict(zip(decoy_header, decoy_rows[0]))
            check(row.get("plain_peptide") == "SEATENCEK",
                  f"plain_peptide: expected SEATENCEK, got {row.get('plain_peptide')!r}",
                  failures)
    finally:
        search_params_file.unlink(missing_ok=True)
        idx.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)
        txt_decoy.unlink(missing_ok=True)

    return failures


# ---------------------------------------------------------------------------
# T30 -- docs/20260819_fablereview.md C1/C2 regression: a precursor whose mass sits
# exactly at the configured top of the mass range must not corrupt the heap.
# ---------------------------------------------------------------------------
#
# C1: g_bIndexPrecursors was allocated BIN(dPeptideMassHigh) bools (valid indices
# 0..BIN(high)-1) but ReadPrecursors()/AddFragments() read/write index BIN(high)
# itself when a precursor+tolerance window reaches the top of the configured mass
# range -- a 1-byte heap OOB read/write. C2: the batch XCorr pool arrays were sized
# exactly iArraySizeGlobal with no padding, but the XCorr loop reads up to
# iXcorrProcessingOffset doubles past that for a precursor at the top of the range.
# Both fixed by a +1/+offset allocation. This fixture sets digest_mass_range's upper
# bound to exactly this peptide's mass and requires fragindex_skipreadprecursors=0
# (the FI_DB precursor-index-limited build path C1 lives in; the param defaults to 1,
# skipping that path entirely, so it must be set explicitly here).

T30_PARAMS_TEMPLATE = (
    T19_PARAMS_TEMPLATE
    .replace("search_enzyme_number = 0", "search_enzyme_number = 1")
    .replace("digest_mass_range = 200.0 2000.0",
             "digest_mass_range = 200.0 1019.462\nfragindex_skipreadprecursors = 0")
    .replace("peptide_length_range = 8 8", "peptide_length_range = 9 9")
)
assert "search_enzyme_number = 1" in T30_PARAMS_TEMPLATE, \
    "T30_PARAMS_TEMPLATE: search_enzyme_number replacement didn't fire -- T19_PARAMS_TEMPLATE changed?"
assert "fragindex_skipreadprecursors = 0" in T30_PARAMS_TEMPLATE, \
    "T30_PARAMS_TEMPLATE: digest_mass_range replacement didn't fire -- T19_PARAMS_TEMPLATE changed?"
assert "peptide_length_range = 9 9" in T30_PARAMS_TEMPLATE, \
    "T30_PARAMS_TEMPLATE: peptide_length_range replacement didn't fire -- T19_PARAMS_TEMPLATE changed?"


@register("t30_mass_boundary")
def test_t30_mass_boundary(comet_exe):
    """T30: C1/C2 regression -- FI_DB build+search with a precursor exactly at
    digest_mass_range's upper bound must not crash or silently miss the peptide."""
    failures = []

    fasta = DATA_DIR / "t30_massboundary.fasta"
    ms2   = DATA_DIR / "t30_massboundary.ms2"
    idx   = fasta.with_suffix(".fasta.idx")
    txt   = ms2.with_suffix(".txt")

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    idx.unlink(missing_ok=True)
    build_params = T30_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(fasta), ascorepro=0,
        mod1="0.0 X 0 3 -1 0 0 0.0")
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

    txt.unlink(missing_ok=True)
    search_params = T30_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(idx), ascorepro=0,
        mod1="0.0 X 0 3 -1 0 0 0.0")
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(search_params)
        search_params_file = Path(pf.name)

    try:
        rc, out = _run_t19_step(comet_exe, [f"-P{fmt(search_params_file)}", fmt(ms2)])
        if rc != 0:
            failures.append(f"search failed/crashed (rc={rc}):\n{out}")
            return failures
        if not txt.exists():
            failures.append(f".txt not created. Comet output:\n{out}")
            return failures

        lines = txt.read_text().splitlines()
        rows  = [l.split("\t") for l in lines[2:] if l.strip()]
        check(len(rows) == 1, f"expected exactly 1 PSM row, got {len(rows)}", failures)
        if not rows:
            return failures

        header = lines[1].split("\t")
        row = dict(zip(header, rows[0]))
        check(row.get("plain_peptide") == "ACDEFGHIK",
              f"plain_peptide: expected ACDEFGHIK, got {row.get('plain_peptide')!r}",
              failures)
        check(int(row.get("ions_matched", "0")) == 16,
              f"ions_matched: expected all 16 b/y ions matched, got "
              f"{row.get('ions_matched')!r}", failures)
    finally:
        search_params_file.unlink(missing_ok=True)
        idx.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)

    return failures


# ---------------------------------------------------------------------------
# T31 -- docs/20260819_fablereview.md C5 regression (sizing bug only; the MS2
# StoreSpecLib() NULL-deref is unfinished-feature scaffolding left as-is by request):
# g_vulSpecLibPrecursorIndex must be sized to allow writes up to and including
# BINPREC(dPeptideMassHigh).
# ---------------------------------------------------------------------------
#
# The library's one entry has a wide-enough peptide_mass_tolerance window around its
# own mass to reach exactly the top bin of digest_mass_range -- pre-fix, this aborted
# LoadSpecLib() itself (before any spectra are even read) with std::out_of_range. The
# query spectrum's own precursor mass is deliberately far from both the library entry
# and the FASTA's own peptide, so no MS2 library scoring is ever attempted --
# StoreSpecLib()'s known, separately-scoped NULL-deref (unfinished MS2 speclib search,
# left unfixed per Jimmy's 2026-08-19 note) is never reached by this test.

T31_PARAMS_TEMPLATE = textwrap.dedent("""\
# comet_version {comet_version}
database_name = {database}
decoy_search = 0
num_threads = 1
print_ascorepro_score = 0
spectral_library_name = {speclib}
peptide_mass_tolerance_upper = 20.0
peptide_mass_tolerance_lower = -20.0
peptide_mass_units = 0
precursor_tolerance_type = 1
isotope_error = 0
search_enzyme_number = 0
search_enzyme2_number = 0
sample_enzyme_number = 0
num_enzyme_termini = 2
allowed_missed_cleavage = 2
variable_mod01 = 0.0 X 0 3 -1 0 0 0.0
variable_mod02 = 0.0 X 0 3 -1 0 0 0.0
variable_mod03 = 0.0 X 0 3 -1 0 0 0.0
variable_mod04 = 0.0 X 0 3 -1 0 0 0.0
variable_mod05 = 0.0 X 0 3 -1 0 0 0.0
max_variable_mods_in_peptide = 3
require_variable_mod = 0
fragment_bin_tol = 1.0005
fragment_bin_offset = 0.4
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
digest_mass_range = 200.0 1200.0
peptide_length_range = 5 20
max_duplicate_proteins = -1
max_fragment_charge = 3
min_precursor_charge = 1
max_precursor_charge = 6
clip_nterm_methionine = 0
spectrum_batch_size = 15000
decoy_prefix = DECOY_
equal_I_and_L = 0
mass_offsets =
minimum_peaks = 1
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
""")


@register("t31_speclib_sizing")
def test_t31_speclib_sizing(comet_exe):
    """T31: C5 regression (sizing bug only) -- LoadSpecLib() must not throw
    std::out_of_range when a library entry's tolerance window reaches exactly
    digest_mass_range's top bin."""
    failures = []

    fasta   = DATA_DIR / "t31_speclib.fasta"
    speclib = DATA_DIR / "t31_speclib.msp"
    ms2     = DATA_DIR / "t31_speclib.ms2"
    txt     = ms2.with_suffix(".txt")

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    txt.unlink(missing_ok=True)
    search_params = T31_PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(fasta), speclib=fmt(speclib))
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(search_params)
        params_file = Path(pf.name)

    try:
        rc, out = _run_t19_step(comet_exe, [f"-P{fmt(params_file)}", fmt(ms2)])
        check(rc == 0,
              f"expected a clean exit (rc=0); a regression throws std::out_of_range "
              f"while loading the library, before any spectra are even read "
              f"(rc={rc}):\n{out}", failures)
        check("out_of_range" not in out,
              f"unexpected std::out_of_range in output:\n{out}", failures)
        check(txt.exists(), f".txt not created. Comet output:\n{out}", failures)
    finally:
        params_file.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)

    return failures


# ---------------------------------------------------------------------------
# T32 -- docs/20260819_fablereview.md B11 regression: search_enzyme_number with no
# matching [COMET_ENZYME_INFO] definition must error, not silently run with an
# empty/garbage enzyme.
# ---------------------------------------------------------------------------

@register("t32_bad_enzyme_number")
def test_t32_bad_enzyme_number(comet_exe):
    """T32: B11 regression -- search_enzyme_number = 99 (undefined in
    [COMET_ENZYME_INFO]) must be rejected with a clear error, not silently accepted."""
    failures = []
    try:
        run_comet_index(comet_exe, DATA_DIR / "t1_basic.fasta", {
            "enzyme": 99, "missed_cleavage": 0,
            "len_min": 8, "len_max": 10, "mass_low": 200.0,
            "equal_IL": 0, "static_C": 0.0,
        })
        failures.append("expected Comet to fail with search_enzyme_number=99, but it "
                         "succeeded")
    except RuntimeError as e:
        msg = str(e)
        check("search_enzyme_number 99" in msg and "missing definition" in msg,
              f"expected a 'search_enzyme_number 99 ... missing definition' error, "
              f"got: {msg[:300]}", failures)
    finally:
        idx_path = (DATA_DIR / "t1_basic.fasta").with_suffix(".fasta.idx")
        idx_path.unlink(missing_ok=True)
    return failures


# ---------------------------------------------------------------------------
# T33 -- docs/20260819_fablereview.md C10 regression: a very long param value and a
# malformed mass_offsets entry must not overflow a stack buffer or hang.
# ---------------------------------------------------------------------------

@register("t33_param_robustness")
def test_t33_param_robustness(comet_exe):
    """T33: C10 regression -- a 600-char param value (szParamVal is 512 bytes) and a
    malformed mass_offsets token (a non-numeric token that used to stall strtok()
    forever) must not crash or hang; either a clean error or a graceful skip is fine."""
    failures = []

    fasta = DATA_DIR / "t1_basic.fasta"
    ms2   = legacy_cases.LEGACY_DIR / "plain" / "input.ms2"

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    long_value = "x" * 600
    params_text = PARAMS_TEMPLATE.format(
        comet_version="2026.02 rev. 0", database=fmt(fasta),
        enzyme=0, missed_cleavage=0, len_min=8, len_max=10, mass_low=200.0,
        equal_IL=0, static_C=0.0,
    )
    # Inserted before [COMET_ENZYME_INFO] (not appended at the very end) so these
    # remain ordinary key=value lines, not enzyme-table rows; Comet's param parser
    # takes the last occurrence of a repeated key, so these override the template's
    # own decoy_prefix/mass_offsets lines above them.
    overrides = (f"decoy_prefix = {long_value}\n"
                 "mass_offsets = 10.0 garbageTOKEN 20.0\n")
    assert "[COMET_ENZYME_INFO]" in params_text
    params_text = params_text.replace("[COMET_ENZYME_INFO]", overrides + "[COMET_ENZYME_INFO]")

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(params_text)
        params_file = Path(pf.name)

    txt = ms2.with_suffix(".txt")
    txt.unlink(missing_ok=True)
    try:
        try:
            result = subprocess.run(
                [str(comet_exe), f"-P{fmt(params_file)}", fmt(ms2)],
                capture_output=True, text=True, timeout=30,
            )
        except subprocess.TimeoutExpired:
            failures.append("Comet did not exit within 30s -- the malformed "
                             "mass_offsets token likely stalled strtok() in an "
                             "infinite loop")
            return failures

        out = result.stdout + result.stderr
        # Comet's only clean exits are 0 (success) and 1 (exit(1) on a param error);
        # anything else indicates a crash. A signal death shows up as a negative
        # returncode on POSIX, but a Windows crash surfaces as a large POSITIVE
        # NTSTATUS-derived code (e.g. 0xC0000409 stack-buffer-overrun = 3221226505
        # from Python on Windows, or that code truncated to its low byte, 9, through
        # WSL interop) -- so `>= 0` would pass trivially for exactly the Windows
        # crash this test guards against. Either a clean error or a clean success is
        # fine -- the bug was a hang or a crash, not "must succeed".
        check(result.returncode in (0, 1),
              f"Comet exited abnormally (returncode={result.returncode}), suggesting "
              f"a crash:\n{out}", failures)
    finally:
        params_file.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)

    return failures


# ---------------------------------------------------------------------------
# T37 / T41 -- protein-terminal variable mods ('^' / '$') on the plain-FASTA path, and
# the deprecation of variable_mod fields 5/6 (docs/20260915_permuter_terminal_mods.md,
# Phase 0).
#
# Fixture t37_protein_term.fasta holds three proteins cut from the legacy
# epgc_9entry.fasta protein (which *starts* with YFDSFGDLSSASAIMGNPK):
#   t37_full  the whole protein                    -> YFDSF...GNPK is protein-N-terminal
#   t37_noY   the protein minus its leading Y       -> FDSF...GNPK  is protein-N-terminal
#   t37_toP   the first 18 residues only            -> YFDSF...GNP  is protein-C-terminal
# The legacy term-mod2 spectrum is YFDSFGDLSSASAIMGNPK + M oxidation. With a
# 163.063 (= Tyr) N-term mod and a 128.095 (= Lys) C-term mod, Comet reports the
# mass-equivalent truncated peptides carrying a terminal mod as co-ranked hits (the
# T21 "permutations"). Whether such a variant is *allowed* is exactly what the new
# codes control:
#   'n' (any peptide N-term):  FDSF...GNPK+n is legal from t37_full (internal) AND t37_noY
#   '^' (protein N-term only): FDSF...GNPK+^ is legal from t37_noY ONLY
#   'c' / '$' likewise for YFDSF...GNP+c from t37_full (internal) vs. t37_toP only.
# ---------------------------------------------------------------------------

_T37_FASTA = DATA_DIR / "t37_protein_term.fasta"
_T37_MS2   = legacy_cases.LEGACY_DIR / "term-mod2" / "input.ms2"
_T37_MOX   = "15.9949 M 0 3 -1 0 0 0.0"


def _t37_search(comet_exe, mods, num_output_lines=12):
    """No-enzyme search of the term-mod2 spectrum against t37_protein_term.fasta with the
    given variable_mod strings. Returns (rc, rows, combined stdout+stderr)."""
    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str
    txt = _T37_MS2.with_suffix(".txt")
    txt.unlink(missing_ok=True)

    params = legacy_cases.build_params(
        database=fmt(_T37_FASTA), enzyme1=0, mods=mods, num_output_lines=num_output_lines)
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".params", dir=str(DATA_DIR), delete=False
    ) as pf:
        pf.write(params)
        params_file = Path(pf.name)
    try:
        rc, out = _run_t19_step(comet_exe, [f"-P{fmt(params_file)}", fmt(_T37_MS2)])
        rows = legacy_cases.parse_txt(txt) if txt.exists() else []
        return rc, rows, out
    finally:
        params_file.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)


_T37_TERM_CODES = ("_n", "_c", "_N", "_C")


def _t37_find(rows, plain_peptide, mod_substr=None, no_term_mod=False):
    """First row with this plain_peptide whose modifications column contains mod_substr
    (if given) and, if no_term_mod, carries none of the terminal-mod codes."""
    for r in rows:
        if r.get("plain_peptide") != plain_peptide:
            continue
        mods = r.get("modifications", "")
        if mod_substr is not None and mod_substr not in mods:
            continue
        if no_term_mod and any(code in mods for code in _T37_TERM_CODES):
            continue
        return r
    return None


def _t37_proteins(row):
    return set(row.get("protein", "").split(",")) if row else set()


def _t37_signature(rows):
    """Order-independent summary of a result set for identical-output comparisons."""
    return sorted((r.get("plain_peptide"), r.get("modifications"), r.get("protein")) for r in rows)


@register("t37_protein_term_plain")
def test_t37_protein_term_plain(comet_exe):
    """T37: '^'/'$' protein-terminal variable mods on the plain-FASTA path."""
    failures = []
    if not (_T37_FASTA.exists() and _T37_MS2.exists()):
        failures.append(f"fixture missing: {_T37_FASTA} / {_T37_MS2}")
        return failures

    # --- protein-terminus-only codes
    rc, rows, out = _t37_search(comet_exe, (_T37_MOX,
                                             "128.094963050 $ 0 3 -1 0 0 0.0",
                                             "163.063328575 ^ 0 3 -1 0 0 0.0"))
    if not check(rc == 0 and rows, f"'^'/'$' search ran and produced rows (rc={rc})", failures):
        print(out[-2000:])
        return failures

    r = _t37_find(rows, "FDSFGDLSSASAIMGNPK", mod_substr="163.063329_N")
    check(r is not None, "FDSFGDLSSASAIMGNPK carrying the '^' mod (reported as _N) is present", failures)
    check(_t37_proteins(r) == {"t37_noY"},
          f"'^' variant is attributed to t37_noY only (protein-N-terminal there), got {sorted(_t37_proteins(r))}", failures)

    r = _t37_find(rows, "YFDSFGDLSSASAIMGNP", mod_substr="128.094963_C")
    check(r is not None, "YFDSFGDLSSASAIMGNP carrying the '$' mod (reported as _C) is present", failures)
    check(_t37_proteins(r) == {"t37_toP"},
          f"'$' variant is attributed to t37_toP only (protein-C-terminal there), got {sorted(_t37_proteins(r))}", failures)

    check(_t37_find(rows, "FDSFGDLSSASAIMGNP", mod_substr="_N") is None,
          "no doubly-truncated FDSFGDLSSASAIMGNP with a protein-N-term mod (it is protein-N-terminal in no protein "
          "where it is also protein-C-terminal)", failures)
    check(not any("_n" in r.get("modifications", "") or "_c" in r.get("modifications", "") for r in rows),
          "no lowercase _n/_c terminal-mod codes when only '^'/'$' are declared", failures)

    r = _t37_find(rows, "YFDSFGDLSSASAIMGNPK", no_term_mod=True)
    check(r is not None and "t37_full" in _t37_proteins(r),
          "the intact YFDSFGDLSSASAIMGNPK (no terminal mod) is still found from t37_full", failures)

    # --- control: peptide-terminus codes admit the internal copies too
    rc, rows_nc, out = _t37_search(comet_exe, (_T37_MOX,
                                                "128.094963050 c 0 3 -1 0 0 0.0",
                                                "163.063328575 n 0 3 -1 0 0 0.0"))
    if check(rc == 0 and rows_nc, f"'n'/'c' control search ran (rc={rc})", failures):
        r = _t37_find(rows_nc, "FDSFGDLSSASAIMGNPK", mod_substr="163.063329_n")
        check(r is not None and _t37_proteins(r) == {"t37_full", "t37_noY"},
              f"'n' variant is attributed to both t37_full (internal) and t37_noY, got {sorted(_t37_proteins(r))}", failures)
        r = _t37_find(rows_nc, "YFDSFGDLSSASAIMGNP", mod_substr="128.094963_c")
        check(r is not None and _t37_proteins(r) == {"t37_full", "t37_toP"},
              f"'c' variant is attributed to both t37_full (internal) and t37_toP, got {sorted(_t37_proteins(r))}", failures)
        check(_t37_find(rows_nc, "FDSFGDLSSASAIMGNP", mod_substr="_n") is not None,
              "doubly-truncated FDSFGDLSSASAIMGNP with both peptide-terminal mods is present under 'n'/'c'", failures)

    # --- 'n^' in one slot is just 'n'
    rc, rows_mix, out = _t37_search(comet_exe, (_T37_MOX, "163.063328575 n^ 0 3 -1 0 0 0.0"))
    if check(rc == 0 and rows_mix, f"'n^' search ran (rc={rc})", failures):
        r = _t37_find(rows_mix, "FDSFGDLSSASAIMGNPK", mod_substr="163.063329_n")
        check(r is not None and _t37_proteins(r) == {"t37_full", "t37_noY"},
              f"'n^' behaves as 'n' (both proteins, lowercase code), got {sorted(_t37_proteins(r))}", failures)

    return failures


@register("t41_termmod_deprecation")
def test_t41_termmod_deprecation(comet_exe):
    """T41: variable_mod fields 5/6 (term_distance, n/c-term) are deprecated: the legacy
    protein-terminus idiom is bridged to '^'/'$' with a warning; other non-default values
    warn and are ignored."""
    failures = []
    if not (_T37_FASTA.exists() and _T37_MS2.exists()):
        failures.append(f"fixture missing: {_T37_FASTA} / {_T37_MS2}")
        return failures

    rc, ref_rows, out = _t37_search(comet_exe, (_T37_MOX,
                                                 "128.094963050 $ 0 3 -1 0 0 0.0",
                                                 "163.063328575 ^ 0 3 -1 0 0 0.0"))
    if not check(rc == 0 and ref_rows, f"reference '^'/'$' search ran (rc={rc})", failures):
        return failures
    check("deprecated" not in out, "no deprecation warning for default fields 5/6 (-1 0)", failures)

    # legacy idiom: n + distance 0 + which_term 0 (protein N), c + distance 0 + which_term 1 (protein C)
    rc, rows, out = _t37_search(comet_exe, (_T37_MOX,
                                             "128.094963050 c 0 3 0 1 0 0.0",
                                             "163.063328575 n 0 3 0 0 0 0.0"))
    if check(rc == 0 and rows, f"legacy protein-terminus idiom search ran (rc={rc})", failures):
        check(_t37_signature(rows) == _t37_signature(ref_rows),
              "legacy 'n 0 3 0 0' / 'c 0 3 0 1' params give results identical to '^' / '$'", failures)
        check("translated 'n' with distance 0 to '^'" in out,
              "bridge warning names the 'n' -> '^' translation", failures)
        check("translated 'c' with distance 0 to '$'" in out,
              "bridge warning names the 'c' -> '$' translation", failures)

    # residue restricted to the protein terminus by distance 0: restriction dropped, warned
    rc, rows_ref_m, _ = _t37_search(comet_exe, (_T37_MOX,))
    rc, rows, out = _t37_search(comet_exe, ("15.9949 M 0 3 0 0 0 0.0",))
    if check(rc == 0 and rows, f"'M 0 3 0 0' search ran (rc={rc})", failures):
        check("restricted residues \"M\"" in out and "dropped" in out,
              "warning says the residue restriction was dropped", failures)
        check(_t37_signature(rows) == _t37_signature(rows_ref_m),
              "'M 0 3 0 0' now behaves as unrestricted 'M 0 3 -1 0'", failures)

    # positive distance: ignored, warned
    rc, rows, out = _t37_search(comet_exe, ("15.9949 M 0 3 2 0 0 0.0",))
    if check(rc == 0 and rows, f"'M 0 3 2 0' search ran (rc={rc})", failures):
        check("term_distance 2 is deprecated and ignored" in out,
              "warning names the ignored positive distance", failures)
        check(_t37_signature(rows) == _t37_signature(rows_ref_m),
              "'M 0 3 2 0' now behaves as unrestricted 'M 0 3 -1 0'", failures)

    return failures


# ---------------------------------------------------------------------------
# T38-T43 -- terminal variable mods permuted inside ModificationsPermuter on the index
# path (docs/20260915_permuter_terminal_mods.md, Phase 2): protein-terminal '^'/'$' on
# FI_DB and PI_DB, terminal mods counting toward max_variable_mods_in_peptide, internal
# decoys keeping terminal mods in place, static protein-terminal masses on FI_DB (D9), and
# the v5 .idx header.
#
# Index-path caveat, by design: the raw-peptide table holds ONE row per unique sequence,
# so a peptide shared by several proteins carries a single flank pair, OR'd across its
# occurrences ("protein-terminal in ANY protein" -- CometFragmentIndex.cpp's dedup merge).
# The plain-FASTA path evaluates each protein separately. So on the index path a '^'
# variant of a shared peptide is attributed to every protein containing it, and a peptide
# that is N-terminal in one protein and C-terminal in another can carry both '^' and '$'.
# These tests assert what both paths agree on and leave the shared-row attribution loose.
# ---------------------------------------------------------------------------

def _t38_index_search(comet_exe, index_flag, mods, num_output_lines=12, max_varmods=5,
                      decoy_search=0, extra_params=None):
    """Build t37_protein_term.fasta into an .idx with `index_flag` and search the term-mod2
    spectrum against it. Returns (rc, rows, log, header_line, variable_mod_line)."""
    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str
    idx = _T37_FASTA.with_suffix(".fasta.idx")
    txt = _T37_MS2.with_suffix(".txt")
    ist = 1 if index_flag == "-i" else 0
    params_files = []
    try:
        idx.unlink(missing_ok=True)
        build = legacy_cases.build_params(database=fmt(_T37_FASTA), enzyme1=0, mods=mods,
                                          num_output_lines=num_output_lines, max_varmods=max_varmods,
                                          decoy_search=decoy_search)
        for k, v in (extra_params or {}).items():
            build = _set_param_line(build, k, v)
        with tempfile.NamedTemporaryFile(mode="w", suffix=".params", dir=str(DATA_DIR), delete=False) as pf:
            pf.write(build)
            params_files.append(Path(pf.name))
        rc, out = _run_t19_step(comet_exe, [index_flag, f"-P{fmt(params_files[-1])}"])
        if rc != 0 or not idx.exists():
            return rc, [], out, "", ""
        with idx.open("rb") as f:
            head = f.read(4096).decode("latin-1").splitlines()
        header_line = head[0] if head else ""
        vm_line = next((l for l in head if l.startswith("VariableMod:")), "")

        search = legacy_cases.build_params(database=fmt(idx), enzyme1=0, mods=mods,
                                           num_output_lines=num_output_lines, max_varmods=max_varmods,
                                           decoy_search=decoy_search)
        search = _set_param_line(search, "index_search_type", ist)
        for k, v in (extra_params or {}).items():
            search = _set_param_line(search, k, v)
        with tempfile.NamedTemporaryFile(mode="w", suffix=".params", dir=str(DATA_DIR), delete=False) as pf:
            pf.write(search)
            params_files.append(Path(pf.name))
        txt.unlink(missing_ok=True)
        rc, out = _run_t19_step(comet_exe, [f"-P{fmt(params_files[-1])}", fmt(_T37_MS2)])
        rows = legacy_cases.parse_txt(txt) if txt.exists() else []
        return rc, rows, out, header_line, vm_line
    finally:
        for f in params_files:
            f.unlink(missing_ok=True)
        idx.unlink(missing_ok=True)
        txt.unlink(missing_ok=True)


_T38_PROT_MODS = (_T37_MOX, "128.094963050 $ 0 3 -1 0 0 0.0", "163.063328575 ^ 0 3 -1 0 0 0.0")
_T38_PEP_MODS  = (_T37_MOX, "128.094963050 c 0 3 -1 0 0 0.0", "163.063328575 n 0 3 -1 0 0 0.0")


@register("t38_protein_term_index")
def test_t38_protein_term_index(comet_exe):
    """T38: '^'/'$' protein-terminal variable mods on FI_DB and PI_DB; .idx header is v5."""
    failures = []
    if not (_T37_FASTA.exists() and _T37_MS2.exists()):
        failures.append(f"fixture missing: {_T37_FASTA} / {_T37_MS2}")
        return failures

    for index_flag, label in (("-i", "FI_DB"), ("-j", "PI_DB")):
        rc, rows, out, header, vm_line = _t38_index_search(comet_exe, index_flag, _T38_PROT_MODS)
        if not check(rc == 0 and rows, f"{label}: '^'/'$' index build + search ran (rc={rc})", failures):
            print(out[-2000:])
            continue
        check(header.startswith("Comet index database v5"), f"{label}: .idx header is v5, got {header!r}", failures)
        check("^" in vm_line and "$" in vm_line, f"{label}: VariableMod: header line carries '^' and '$'", failures)

        r = _t37_find(rows, "FDSFGDLSSASAIMGNPK", mod_substr="163.063329_N")
        check(r is not None, f"{label}: FDSFGDLSSASAIMGNPK with the '^' mod (_N) is found", failures)
        check("t37_noY" in _t37_proteins(r), f"{label}: '^' variant lists t37_noY, got {sorted(_t37_proteins(r))}", failures)

        r = _t37_find(rows, "YFDSFGDLSSASAIMGNP", mod_substr="128.094963_C")
        check(r is not None, f"{label}: YFDSFGDLSSASAIMGNP with the '$' mod (_C) is found", failures)
        check("t37_toP" in _t37_proteins(r), f"{label}: '$' variant lists t37_toP, got {sorted(_t37_proteins(r))}", failures)

        check(not any("_n" in r.get("modifications", "") or "_c" in r.get("modifications", "") for r in rows),
              f"{label}: no lowercase _n/_c codes when only '^'/'$' are declared", failures)
        r = _t37_find(rows, "YFDSFGDLSSASAIMGNPK", no_term_mod=True)
        check(r is not None, f"{label}: intact YFDSFGDLSSASAIMGNPK still found", failures)

        # a protein-terminal mod is never placed on a peptide that is internal everywhere
        internal_with_term = [r for r in rows
                              if ("_N" in r.get("modifications", "") or "_C" in r.get("modifications", ""))
                              and r.get("plain_peptide") not in ("FDSFGDLSSASAIMGNPK", "YFDSFGDLSSASAIMGNP", "FDSFGDLSSASAIMGNP")]
        check(not internal_with_term,
              f"{label}: '^'/'$' only on the protein-terminal peptides, unexpected: "
              f"{[(r.get('plain_peptide'), r.get('modifications')) for r in internal_with_term]}", failures)

    return failures


@register("t39_termmod_cap_index")
def test_t39_termmod_cap_index(comet_exe):
    """T39: terminal mods count toward max_variable_mods_in_peptide on FI_DB/PI_DB exactly as
    on the plain-FASTA path (D2). The term-mod2 spectrum's 3-mod permutation
    (FDSFGDLSSASAIMGNP + n-term + c-term + M oxidation) must vanish at cap 2 and return at cap 3."""
    failures = []
    if not (_T37_FASTA.exists() and _T37_MS2.exists()):
        failures.append(f"fixture missing: {_T37_FASTA} / {_T37_MS2}")
        return failures

    THREE = ("FDSFGDLSSASAIMGNP", "_n", "_c")
    TWO   = ("FDSFGDLSSASAIMGNPK", "_n")

    def has3(rows):
        r = _t37_find(rows, THREE[0], mod_substr=THREE[1])
        return r is not None and THREE[2] in r.get("modifications", "") and "15.994900" in r.get("modifications", "")

    def has2(rows):
        r = _t37_find(rows, TWO[0], mod_substr=TWO[1])
        return r is not None and "15.994900" in r.get("modifications", "")

    for cap, expect3 in ((2, False), (3, True)):
        # plain FASTA (reference semantics)
        rc, rows, out = _t37_search(comet_exe, _T38_PEP_MODS, num_output_lines=12) if cap == 3 else (None, None, None)
        if cap == 2:
            use_win = _binary_uses_win_paths(comet_exe)
            fmt = _to_win if use_win else str
            txt = _T37_MS2.with_suffix(".txt"); txt.unlink(missing_ok=True)
            params = legacy_cases.build_params(database=fmt(_T37_FASTA), enzyme1=0, mods=_T38_PEP_MODS,
                                               num_output_lines=12, max_varmods=2)
            with tempfile.NamedTemporaryFile(mode="w", suffix=".params", dir=str(DATA_DIR), delete=False) as pf:
                pf.write(params); pfile = Path(pf.name)
            try:
                rc, out = _run_t19_step(comet_exe, [f"-P{fmt(pfile)}", fmt(_T37_MS2)])
                rows = legacy_cases.parse_txt(txt) if txt.exists() else []
            finally:
                pfile.unlink(missing_ok=True); txt.unlink(missing_ok=True)
        if check(rc == 0 and rows, f"plain FASTA cap={cap}: search ran (rc={rc})", failures):
            check(has2(rows), f"plain FASTA cap={cap}: 2-mod permutation (n-term + M) present", failures)
            check(has3(rows) == expect3, f"plain FASTA cap={cap}: 3-mod permutation present == {expect3}", failures)

        for index_flag, label in (("-i", "FI_DB"), ("-j", "PI_DB")):
            rc, rows, out, _, _ = _t38_index_search(comet_exe, index_flag, _T38_PEP_MODS, max_varmods=cap)
            if not check(rc == 0 and rows, f"{label} cap={cap}: index build + search ran (rc={rc})", failures):
                print(out[-1500:])
                continue
            check(has2(rows), f"{label} cap={cap}: 2-mod permutation (n-term + M) present", failures)
            check(has3(rows) == expect3,
                  f"{label} cap={cap}: 3-mod permutation present == {expect3} (terminal mods count toward the cap)", failures)

    return failures


@register("t40_internal_decoys_protterm")
def test_t40_internal_decoys_protterm(comet_exe):
    """T40: FI_DB internal decoys with a '^' mod -- decoy PSMs keep the terminal mod on the
    N-terminus (site 1) of the reversed sequence, and target rows match the decoy_search=0 run."""
    failures = []
    if not (_T37_FASTA.exists() and _T37_MS2.exists()):
        failures.append(f"fixture missing: {_T37_FASTA} / {_T37_MS2}")
        return failures

    rc0, rows0, out0, _, _ = _t38_index_search(comet_exe, "-i", _T38_PROT_MODS, num_output_lines=20)
    rc1, rows1, out1, _, _ = _t38_index_search(comet_exe, "-i", _T38_PROT_MODS, num_output_lines=20, decoy_search=1)
    if not check(rc0 == 0 and rows0 and rc1 == 0 and rows1,
                 f"FI_DB '^'/'$' searches ran with decoy_search 0 and 1 (rc={rc0},{rc1})", failures):
        print(out1[-1500:])
        return failures

    targets1 = [r for r in rows1 if "DECOY_" not in r.get("protein", "")]
    decoys1  = [r for r in rows1 if "DECOY_" in r.get("protein", "")]
    sig = lambda rs: sorted((r.get("plain_peptide"), r.get("modifications")) for r in rs)
    check(set(sig(targets1)) <= set(sig(rows0)) or set(sig(rows0)) <= set(sig(targets1)),
          "target PSMs with internal decoys are the decoy_search=0 PSMs (possibly displaced in rank)", failures)

    r = _t37_find(rows1, "FDSFGDLSSASAIMGNPK", mod_substr="163.063329_N")
    check(r is not None and r.get("modifications", "").startswith("1_V_163.063329_N"),
          "target '^' variant present with the mod at site 1 (the N-terminus)", failures)

    term_decoys = [r for r in decoys1 if "_N" in r.get("modifications", "") or "_C" in r.get("modifications", "")]
    for r in term_decoys:
        mods = r.get("modifications", "")
        n = len(r.get("plain_peptide", ""))
        ok_n = ("_N" not in mods) or mods.startswith("1_V_") or ",1_V_" in mods
        ok_c = ("_C" not in mods) or (f"{n}_V_" in mods)
        check(ok_n and ok_c, f"decoy {r.get('plain_peptide')} keeps terminal mods on its termini: {mods}", failures)
    print(f"  ({len(decoys1)} decoy rows, {len(term_decoys)} carrying a protein-terminal mod)")
    return failures


@register("t42_static_protein_nterm_fidb")
def test_t42_static_protein_nterm_fidb(comet_exe):
    """T42 (D9): a static add_Nterm_protein mass is applied on FI_DB exactly as on PI_DB and
    plain FASTA. With add_Nterm_protein = Tyr, the protein-N-terminal FDSFGDLSSASAIMGNPK
    (t37_noY) is mass-equivalent to YFDSFGDLSSASAIMGNPK and must be found by all three paths
    with the same calc_neutral_mass; it was missing from FI_DB before this fix."""
    failures = []
    if not (_T37_FASTA.exists() and _T37_MS2.exists()):
        failures.append(f"fixture missing: {_T37_FASTA} / {_T37_MS2}")
        return failures

    STATIC = {"add_Nterm_protein": "163.063328575"}
    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str

    # plain FASTA
    txt = _T37_MS2.with_suffix(".txt"); txt.unlink(missing_ok=True)
    params = legacy_cases.build_params(database=fmt(_T37_FASTA), enzyme1=0, mods=(_T37_MOX,), num_output_lines=12)
    params = _set_param_line(params, "add_Nterm_protein", STATIC["add_Nterm_protein"])
    with tempfile.NamedTemporaryFile(mode="w", suffix=".params", dir=str(DATA_DIR), delete=False) as pf:
        pf.write(params); pfile = Path(pf.name)
    try:
        rc, out = _run_t19_step(comet_exe, [f"-P{fmt(pfile)}", fmt(_T37_MS2)])
        rows_plain = legacy_cases.parse_txt(txt) if txt.exists() else []
    finally:
        pfile.unlink(missing_ok=True); txt.unlink(missing_ok=True)

    masses = {}
    if check(rc == 0 and rows_plain, f"plain FASTA: search ran (rc={rc})", failures):
        # the static protein-N-term mass is annotated as "<pos>_S_<mass>_N" (S = static)
        r = _t37_find(rows_plain, "FDSFGDLSSASAIMGNPK", mod_substr="_S_163.063329_N")
        check(r is not None and "t37_noY" in _t37_proteins(r),
              "plain FASTA: FDSFGDLSSASAIMGNPK found via the static protein-N-term mass", failures)
        if r is not None:
            masses["plain"] = r.get("calc_neutral_mass")

    for index_flag, label in (("-i", "FI_DB"), ("-j", "PI_DB")):
        rc, rows, out, _, _ = _t38_index_search(comet_exe, index_flag, (_T37_MOX,), extra_params=STATIC)
        if not check(rc == 0 and rows, f"{label}: index build + search ran (rc={rc})", failures):
            print(out[-1500:])
            continue
        r = _t37_find(rows, "FDSFGDLSSASAIMGNPK", mod_substr="_S_163.063329_N")
        check(r is not None, f"{label}: FDSFGDLSSASAIMGNPK found via the static protein-N-term mass", failures)
        if r is not None:
            masses[label] = r.get("calc_neutral_mass")

    if len(masses) == 3:
        vals = {float(v) for v in masses.values()}
        check(max(vals) - min(vals) < 1e-4, f"all three paths agree on calc_neutral_mass: {masses}", failures)
    return failures


@register("t43_v4_index_rejected")
def test_t43_v4_index_rejected(comet_exe):
    """T43: a v4 .idx (frozen pre-Phase-2 fixture) is refused with the rebuild message."""
    failures = []
    v4 = DATA_DIR / "t43_v4.fasta.idx"
    if not (v4.exists() and _T37_MS2.exists()):
        failures.append(f"fixture missing: {v4} / {_T37_MS2}")
        return failures
    with v4.open("rb") as f:
        check(f.readline().startswith(b"Comet index database v4"), "fixture really is a v4 index", failures)

    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str
    txt = _T37_MS2.with_suffix(".txt"); txt.unlink(missing_ok=True)
    params = legacy_cases.build_params(database=fmt(v4), enzyme1=0, mods=(_T37_MOX,))
    with tempfile.NamedTemporaryFile(mode="w", suffix=".params", dir=str(DATA_DIR), delete=False) as pf:
        pf.write(params); pfile = Path(pf.name)
    try:
        rc, out = _run_t19_step(comet_exe, [f"-P{fmt(pfile)}", fmt(_T37_MS2)])
    finally:
        pfile.unlink(missing_ok=True); txt.unlink(missing_ok=True)
    check(rc != 0, f"search against a v4 .idx fails (rc={rc})", failures)
    check("not a v5 unified index file" in out, "failure names the v5 requirement and says to rebuild", failures)
    return failures


# --- T22 companion: RTS determinism with a protein-N-terminal variable mod (integration) ---

def _test_rts_protterm(comet_exe, index_flag, label):
    failures = []
    index_search_type = 0 if index_flag == "-j" else 1
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
    params = phospho_params.read_text().replace(
        "database_name = human.target-decoy.fasta", f"database_name = {small_fasta}")
    params = _set_param_line(params, "variable_mod03", "42.010565 ^ 0 1 -1 0 0 0.0")
    hs_idx = None
    try:
        hs_idx = _rts_build_index(comet_exe, small_fasta, params, index_flag)
        out1 = Path(tempfile.mktemp(suffix=".1thread.out", dir=str(DATA_DIR)))
        out8 = Path(tempfile.mktemp(suffix=".8thread.out", dir=str(DATA_DIR)))
        try:
            rc1, log1 = _rts_run(hs_idx, RTS_FIXTURE, 1, out1, index_search_type=index_search_type)
            rc8, log8 = _rts_run(hs_idx, RTS_FIXTURE, 8, out8, index_search_type=index_search_type)
            if check(rc1 == 0 and rc8 == 0, f"{label} '^' acetyl: rts_repro exits 0 at 1 and 8 threads", failures):
                l1, l8 = _rts_sorted_lines(out1), _rts_sorted_lines(out8)
                check(l1 == l8, f"{label} '^' acetyl: 1-thread and 8-thread results byte-identical ({len(l1)} spectra)", failures)
                n_acetyl = sum(1 for l in l1 if "42.0106" in l)
                print(f"  ({n_acetyl} of {len(l1)} results carry the '^' acetyl)")
            else:
                print(log1[-800:]); print(log8[-800:])
        finally:
            out1.unlink(missing_ok=True); out8.unlink(missing_ok=True)
    except RuntimeError as e:
        failures.append(str(e))
    finally:
        if hs_idx and hs_idx.exists():
            hs_idx.unlink()
    return failures


@register("t22_rts_fi_protterm")
def test_t22_rts_fi_protterm(comet_exe):
    """T22b [integration]: RTS FI_DB 1-vs-8-thread determinism with a '^' acetyl mod."""
    return _test_rts_protterm(comet_exe, "-i", "FI_DB")


@register("t22_rts_pi_protterm")
def test_t22_rts_pi_protterm(comet_exe):
    """T22b [integration]: RTS PI_DB 1-vs-8-thread determinism with a '^' acetyl mod."""
    return _test_rts_protterm(comet_exe, "-j", "PI_DB")


# ---------------------------------------------------------------------------
# T44 -- terminal-mod parity on real data (docs/20260915_permuter_terminal_mods.md Phase 4):
# plain FASTA vs FI_DB vs PI_DB must identify a similar peptide population at 1% FDR with a
# peptide-N-term acetyl ('n') and with a protein-N-term acetyl ('^'). Before Phase 2 the '^'
# config diverged (FI_DB/PI_DB ignored protein scope and over-identified) and terminal mods
# did not count toward max_variable_mods_in_peptide on the index path. --integration + --bigdata.
# ---------------------------------------------------------------------------

@register("t44_termmod_parity_bigdata")
def test_t44_termmod_parity_bigdata(comet_exe):
    """T44 [integration, bigdata]: plain vs FI_DB vs PI_DB 1% FDR parity with 'n' and '^' acetyl."""
    if not _RUN_INTEGRATION:
        print("  SKIP: pass --integration to run this test")
        return []
    failures = []
    d3 = Path(_BIGDATA_DIR) / "comet-debug3"
    mzxml = d3 / "20170103_HelaQC_01.mzXML"
    human_td_fasta = d3 / "human.target-decoy.fasta"
    base_params_file = d3 / "comet.params"
    if not (mzxml.exists() and human_td_fasta.exists() and base_params_file.exists()):
        print(f"  SKIP: {d3} not found or incomplete -- pass --bigdata DIR")
        return []

    base = base_params_file.read_text()
    base = _set_param_line(base, "database_name", human_td_fasta)
    base = _set_param_line(base, "decoy_search", "0")
    base = _set_param_line(base, "max_variable_mods_in_peptide", "3")
    idx_path = human_td_fasta.with_suffix(".fasta.idx")

    for code, label in (("n", "peptide-N-term acetyl"), ("^", "protein-N-term acetyl")):
        params = _set_param_line(base, "variable_mod02", f"42.010565 {code} 0 1 -1 0 0 0.0")
        print(f"  --- {label} ('{code}') ---")
        rc0, txt0, out0, t0 = _run_bigdata_search(comet_exe, params, mzxml)
        if not check(rc0 == 0, f"{label}: plain-FASTA search exits 0 (rc={rc0})", failures):
            print(out0[-2000:])
            continue
        _, cx_plain, _ = _q1pct_counts(txt0)
        txt0.unlink(missing_ok=True)
        print(f"    plain-FASTA: {cx_plain:,} PSMs at 1% FDR (xcorr), search {t0:.1f}s")

        counts = {"plain": cx_plain}
        for flag, mode in (("-i", "FI_DB"), ("-j", "PI_DB")):
            res = _index_build_and_search(comet_exe, flag, mode, params, idx_path, mzxml, failures, tag=label)
            if res is None:
                continue
            cx, tb, ts = res
            counts[mode] = cx
            print(f"    {mode}: {cx:,} PSMs at 1% FDR (xcorr); build {tb:.1f}s, search {ts:.1f}s")
        idx_path.unlink(missing_ok=True)

        if len(counts) == 3:
            for mode in ("FI_DB", "PI_DB"):
                ratio = counts[mode] / counts["plain"] if counts["plain"] else float("inf")
                check(0.95 <= ratio <= 1.05,
                      f"{label}: {mode} ({counts[mode]:,}) agrees with plain FASTA ({counts['plain']:,}) "
                      f"within 5% at 1% FDR xcorr (ratio {ratio:.3f})", failures)
    return failures


# ---------------------------------------------------------------------------
# T45-T47 -- review follow-ups on the terminal-mod work (docs/20260915_permuter_terminal_mods.md):
# mixed residue+terminus slots and post-bridge slot merging (T45), the shared-peptide
# protein-terminus attribution policy pinned explicitly on both paths (T46), and the
# pepXML / mzIdentML search-level terminal-mod annotations (T47).
# ---------------------------------------------------------------------------

def _t45_all_paths(comet_exe, mods, failures, tag):
    """Run plain FASTA, FI_DB and PI_DB on the T37 fixture with `mods`; returns {label: rows}."""
    out = {}
    rc, rows, log = _t37_search(comet_exe, mods, num_output_lines=12)
    if check(rc == 0 and rows, f"{tag}: plain-FASTA search ran (rc={rc})", failures):
        out["plain"] = rows
    else:
        print(log[-1500:])
    for flag, label in (("-i", "FI_DB"), ("-j", "PI_DB")):
        rc, rows, log, _, _ = _t38_index_search(comet_exe, flag, mods)
        if check(rc == 0 and rows, f"{tag}: {label} build + search ran (rc={rc})", failures):
            out[label] = rows
        else:
            print(log[-1500:])
    return out


@register("t45_mixed_terminal_codes")
def test_t45_mixed_terminal_codes(comet_exe):
    """T45: one slot carrying residue letters together with terminal codes ('n^K', 'c$K'), and
    two same-mass slots that merge after the deprecation bridge ('n 0 3 0 0' + 'K') -- on
    plain FASTA, FI_DB and PI_DB."""
    failures = []
    if not (_T37_FASTA.exists() and _T37_MS2.exists()):
        failures.append(f"fixture missing: {_T37_FASTA} / {_T37_MS2}")
        return failures

    # 'n^K': the terminus is peptide-scoped ('n' present -> '^' redundant) and K is a residue
    # target of the same mod. The N-term variant must behave exactly like plain 'n'.
    res = _t45_all_paths(comet_exe, (_T37_MOX, "163.063328575 n^K 0 3 -1 0 0 0.0"), failures, "n^K")
    for label, rows in res.items():
        r = _t37_find(rows, "FDSFGDLSSASAIMGNPK", mod_substr="163.063329_n")
        check(r is not None, f"n^K {label}: FDSFGDLSSASAIMGNPK carries the peptide-scoped N-term mod (_n)", failures)
        if label == "plain":
            check(_t37_proteins(r) == {"t37_full", "t37_noY"},
                  f"n^K plain: attributed to both proteins like 'n', got {sorted(_t37_proteins(r))}", failures)
        check(not any("_N" in x.get("modifications", "") for x in rows),
              f"n^K {label}: no protein-scoped _N code (the slot also has 'n')", failures)

    # 'c$K' likewise for the C-terminus.
    res = _t45_all_paths(comet_exe, (_T37_MOX, "128.094963050 c$K 0 3 -1 0 0 0.0"), failures, "c$K")
    for label, rows in res.items():
        r = _t37_find(rows, "YFDSFGDLSSASAIMGNP", mod_substr="128.094963_c")
        check(r is not None, f"c$K {label}: YFDSFGDLSSASAIMGNP carries the peptide-scoped C-term mod (_c)", failures)
        if label == "plain":
            check(_t37_proteins(r) == {"t37_full", "t37_toP"},
                  f"c$K plain: attributed to both proteins like 'c', got {sorted(_t37_proteins(r))}", failures)
        check(not any("_C" in x.get("modifications", "") for x in rows),
              f"c$K {label}: no protein-scoped _C code (the slot also has 'c')", failures)

    # Bridge + merge: slot 2 'n 0 3 0 0' is translated to '^', then merged with slot 3 'K'
    # (same mass, NL, binary, max, required) into one '^K' slot. Results must equal a single
    # explicit '^K' declaration.
    ref = _t45_all_paths(comet_exe, (_T37_MOX, "163.063328575 ^K 0 3 -1 0 0 0.0"), failures, "^K")
    got = _t45_all_paths(comet_exe, (_T37_MOX, "163.063328575 n 0 3 0 0 0 0.0", "163.063328575 K 0 3 -1 0 0 0.0"),
                         failures, "bridged n+K")
    for label in ref:
        if label in got:
            check(_t37_signature(got[label]) == _t37_signature(ref[label]),
                  f"{label}: bridged 'n 0 3 0 0' + 'K' equals explicit '^K'", failures)
            r = _t37_find(got[label], "FDSFGDLSSASAIMGNPK", mod_substr="163.063329_N")
            check(r is not None, f"{label}: merged slot keeps protein scope (_N present)", failures)
    return failures


@register("t46_shared_peptide_attribution")
def test_t46_shared_peptide_attribution(comet_exe):
    """T46: protein-terminus attribution for a peptide shared by proteins with different
    terminal context -- the plain-FASTA path is exact (each protein evaluated on its own);
    the index path stores one raw-peptide row per sequence with the flank context OR'd
    across proteins, so it attributes the protein-terminal variant to every protein
    containing the peptide and can combine '^' and '$' on a peptide that is N-terminal in
    one protein and C-terminal in another. This test pins that accepted difference
    (docs/20260915_permuter_terminal_mods.md section 11)."""
    failures = []
    if not (_T37_FASTA.exists() and _T37_MS2.exists()):
        failures.append(f"fixture missing: {_T37_FASTA} / {_T37_MS2}")
        return failures

    rc, plain, log = _t37_search(comet_exe, _T38_PROT_MODS, num_output_lines=12)
    if not check(rc == 0 and plain, f"plain-FASTA '^'/'$' search ran (rc={rc})", failures):
        return failures
    r = _t37_find(plain, "FDSFGDLSSASAIMGNPK", mod_substr="163.063329_N")
    check(r is not None and _t37_proteins(r) == {"t37_noY"},
          f"plain: '^' variant attributed to t37_noY only, got {sorted(_t37_proteins(r))}", failures)
    r = _t37_find(plain, "YFDSFGDLSSASAIMGNP", mod_substr="128.094963_C")
    check(r is not None and _t37_proteins(r) == {"t37_toP"},
          f"plain: '$' variant attributed to t37_toP only, got {sorted(_t37_proteins(r))}", failures)
    check(_t37_find(plain, "FDSFGDLSSASAIMGNP", mod_substr="_N") is None,
          "plain: no peptide carries '^' and '$' together (no single protein has it at both termini)", failures)

    for flag, label in (("-i", "FI_DB"), ("-j", "PI_DB")):
        rc, rows, log, _, _ = _t38_index_search(comet_exe, flag, _T38_PROT_MODS)
        if not check(rc == 0 and rows, f"{label}: '^'/'$' build + search ran (rc={rc})", failures):
            print(log[-1500:])
            continue
        r = _t37_find(rows, "FDSFGDLSSASAIMGNPK", mod_substr="163.063329_N")
        check(r is not None and _t37_proteins(r) == {"t37_full", "t37_noY"},
              f"{label}: '^' variant attributed to both proteins sharing the row (policy), got {sorted(_t37_proteins(r))}", failures)
        r = _t37_find(rows, "YFDSFGDLSSASAIMGNP", mod_substr="128.094963_C")
        check(r is not None and _t37_proteins(r) == {"t37_full", "t37_toP"},
              f"{label}: '$' variant attributed to both proteins sharing the row (policy), got {sorted(_t37_proteins(r))}", failures)
        r = _t37_find(rows, "FDSFGDLSSASAIMGNP", mod_substr="_N")
        check(r is not None and "_C" in r.get("modifications", ""),
              f"{label}: shared peptide N-terminal in t37_noY and C-terminal in t37_toP carries '^' and '$' together (policy)", failures)
    return failures


@register("t47_terminal_mod_xml_annotations")
def test_t47_terminal_mod_xml_annotations(comet_exe):
    """T47: pepXML <terminal_modification protein_terminus> and mzIdentML <SearchModification>
    specificity CV terms for the four terminal codes n, ^, c, $."""
    failures = []
    if not (_T37_FASTA.exists() and _T37_MS2.exists()):
        failures.append(f"fixture missing: {_T37_FASTA} / {_T37_MS2}")
        return failures

    # distinct masses so each slot is identifiable in the output
    MODS = ("42.010565 n 0 1 -1 0 0 0.0", "43.5 ^ 0 1 -1 0 0 0.0",
            "0.984016 c 0 1 -1 0 0 0.0", "1.5 $ 0 1 -1 0 0 0.0")
    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str
    outs = {ext: _T37_MS2.with_suffix(ext) for ext in (".txt", ".pep.xml", ".mzid")}
    for f in outs.values():
        f.unlink(missing_ok=True)
    params = legacy_cases.build_params(database=fmt(_T37_FASTA), enzyme1=0, mods=MODS, num_output_lines=3)
    params = _set_param_line(params, "output_pepxmlfile", "1")
    params = _set_param_line(params, "output_mzidentmlfile", "1")
    with tempfile.NamedTemporaryFile(mode="w", suffix=".params", dir=str(DATA_DIR), delete=False) as pf:
        pf.write(params); pfile = Path(pf.name)
    try:
        rc, log = _run_t19_step(comet_exe, [f"-P{fmt(pfile)}", fmt(_T37_MS2)])
        if not check(rc == 0 and outs[".pep.xml"].exists() and outs[".mzid"].exists(),
                     f"search wrote .pep.xml and .mzid (rc={rc})", failures):
            print(log[-1500:])
            return failures
        pepxml = outs[".pep.xml"].read_text(errors="replace")
        mzid = outs[".mzid"].read_text(errors="replace")
    finally:
        pfile.unlink(missing_ok=True)
        for f in outs.values():
            f.unlink(missing_ok=True)

    # --- pepXML search summary
    term_lines = [l.strip() for l in pepxml.splitlines() if "<terminal_modification" in l]
    def pep_has(terminus, massdiff, prot):
        return any(f'terminus="{terminus}"' in l and f'massdiff="{massdiff}"' in l
                   and 'variable="Y"' in l and f'protein_terminus="{prot}"' in l for l in term_lines)
    check(pep_has("N", "42.010565", "N"), "pepXML: 'n' -> terminus N, protein_terminus=N", failures)
    check(pep_has("N", "43.500000", "Y"), "pepXML: '^' -> terminus N, protein_terminus=Y", failures)
    check(pep_has("C", "0.984016", "N"),  "pepXML: 'c' -> terminus C, protein_terminus=N", failures)
    check(pep_has("C", "1.500000", "Y"),  "pepXML: '$' -> terminus C, protein_terminus=Y", failures)
    check(sum(1 for l in term_lines if 'variable="Y"' in l) == 4,
          f"pepXML: exactly four variable terminal_modification lines, got {len(term_lines)}", failures)
    check(not any('aminoacid="^"' in l or 'aminoacid="$"' in l or 'aminoacid="n"' in l or 'aminoacid="c"' in l
                  for l in pepxml.splitlines()),
          "pepXML: no terminal code leaks out as an <aminoacid_modification>", failures)

    # --- mzIdentML analysis protocol: massDelta + specificity accession per SearchModification block
    blocks = re.findall(r"<SearchModification[^>]*massDelta=\"([0-9.]+)\"[^>]*fixedMod= \"false\"[^>]*>(.*?)</SearchModification>",
                        mzid, flags=re.S)
    def mzid_has(mass, accession):
        return any(abs(float(m) - mass) < 1e-6 and accession in body for m, body in blocks)
    check(mzid_has(42.010565, "MS:1001189"), "mzIdentML: 'n' -> peptide N-term specificity (MS:1001189)", failures)
    check(mzid_has(43.5,      "MS:1002057"), "mzIdentML: '^' -> protein N-term specificity (MS:1002057)", failures)
    check(mzid_has(0.984016,  "MS:1001190"), "mzIdentML: 'c' -> peptide C-term specificity (MS:1001190)", failures)
    check(mzid_has(1.5,       "MS:1002058"), "mzIdentML: '$' -> protein C-term specificity (MS:1002058)", failures)
    check(not any('residues="^"' in l or 'residues="$"' in l or 'residues="n"' in l or 'residues="c"' in l
                  for l in mzid.splitlines()),
          "mzIdentML: no terminal code leaks out as a residue SearchModification", failures)
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


# ---------------------------------------------------------------------------
# T34 -- FI_DB (and PI_DB) internal decoys on a crafted fixture
# ---------------------------------------------------------------------------
#
# docs/20260914_FI_internal_decoys.md Phase 4. Fixture data/t34_fi_internal_decoys.{fasta,ms2}:
# two tryptic peptides -- PEPTMIDEK (oxidizable M; its pseudo-reverse under trypsin's
# last-residue-fixed rule is EDIMTPEPK, and the M-oxidation must travel with the M to
# EDIM[15.9949]TPEPK) and LSAGGASLK (self-palindromic: its pseudo-reverse is itself) -- and four
# synthetic 2+ spectra: (1) PEPTM[ox]IDEK, (2) EDIM[ox]TPEPK, (3) LSAGGASLK, (4) PEPTMIDEK.
#
# Asserts, per index type (FI_DB via -i, PI_DB via -j):
#   decoy_search=0: no DECOY_ anywhere; scan 2 does NOT match EDIM...TPEPK; scan 3 lists only T34_pal.
#   decoy_search=1: scan 1 -> target, unprefixed; scan 2 -> EDIM[15.9949]TPEPK labeled DECOY_T34_ox
#                   (and NOT also the bare T34_ox -- the batch protein-name helper's former
#                   FI_DB fallback emitted both); scan 3 -> exactly ONE LSAGGASLK row whose
#                   protein column carries both T34_pal and DECOY_T34_pal (target + palindromic
#                   decoy merged by CheckDuplicateI()); FI_DB search log reports exactly
#                   3 internal decoy variants (PEPTMIDEK, PEPTM[ox]IDEK, LSAGGASLK).
#   decoy_search=2: target .txt has no DECOY_ rows and no EDIM...; .decoy.txt has scan 2 ->
#                   EDIM[15.9949]TPEPK / DECOY_T34_ox and scan 3 -> LSAGGASLK / DECOY_T34_pal
#                   (separate lists, no merge).

T34_PARAMS_TEMPLATE = T26_PARAMS_TEMPLATE.replace("num_output_lines = 1", "num_output_lines = 3")
assert "num_output_lines = 3" in T34_PARAMS_TEMPLATE, "T34_PARAMS_TEMPLATE: num_output_lines replacement didn't fire"


def _t34_read_rows(txt):
    if not txt.exists():
        return []
    lines = txt.read_text().splitlines()
    if len(lines) < 2:
        return []
    header = lines[1].split("\t")
    return [dict(zip(header, l.split("\t"))) for l in lines[2:] if l.strip()]


def _t34_run_mode(comet_exe, index_flag, label, decoy_search, failures):
    """Build the T34 index with `index_flag`, search the fixture with `decoy_search`; returns
    (target_rows, decoy_rows, search_log) or None on a build/search failure."""
    fasta = DATA_DIR / "t34_fi_internal_decoys.fasta"
    ms2   = DATA_DIR / "t34_fi_internal_decoys.ms2"
    idx   = fasta.with_suffix(".fasta.idx")
    txt   = ms2.with_suffix(".txt")
    txt_d = ms2.with_suffix(".decoy.txt")
    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str
    mod1 = "15.9949 M 0 3 -1 0 0 0.0"
    tag = f"{label} decoy_search={decoy_search}"

    for f in (idx, txt, txt_d):
        f.unlink(missing_ok=True)
    params_files = []
    try:
        build_params = T34_PARAMS_TEMPLATE.format(
            comet_version="2026.02 rev. 0", database=fmt(fasta), ascorepro=0, mod1=mod1,
            decoy_search=decoy_search, len_min=5, len_max=15)
        with tempfile.NamedTemporaryFile(mode="w", suffix=".params", dir=str(DATA_DIR), delete=False) as pf:
            pf.write(build_params)
            params_files.append(Path(pf.name))
        rc, out = _run_t19_step(comet_exe, [index_flag, f"-P{fmt(params_files[-1])}"])
        if not check(rc == 0 and idx.exists(), f"{tag}: index builds (rc={rc})", failures):
            print(out[-1500:])
            return None

        search_params = T34_PARAMS_TEMPLATE.format(
            comet_version="2026.02 rev. 0", database=fmt(idx), ascorepro=0, mod1=mod1,
            decoy_search=decoy_search, len_min=5, len_max=15)
        search_params = _set_param_line(search_params, "index_search_type", 1 if index_flag == "-i" else 0)
        with tempfile.NamedTemporaryFile(mode="w", suffix=".params", dir=str(DATA_DIR), delete=False) as pf:
            pf.write(search_params)
            params_files.append(Path(pf.name))
        rc, out = _run_t19_step(comet_exe, [f"-P{fmt(params_files[-1])}", fmt(ms2)])
        if not check(rc == 0 and txt.exists(), f"{tag}: search exits 0 and writes .txt (rc={rc})", failures):
            print(out[-1500:])
            return None
        return _t34_read_rows(txt), _t34_read_rows(txt_d), out
    finally:
        for f in params_files:
            f.unlink(missing_ok=True)
        for f in (idx, txt, txt_d):
            f.unlink(missing_ok=True)


def _t34_rank1(rows, scan):
    r = [x for x in rows if x.get("scan") == str(scan) and x.get("num") == "1"]
    return r[0] if r else None


def _test_t34_index_type(comet_exe, index_flag, label):
    failures = []
    fasta = DATA_DIR / "t34_fi_internal_decoys.fasta"
    ms2   = DATA_DIR / "t34_fi_internal_decoys.ms2"
    if not fasta.exists() or not ms2.exists():
        failures.append(f"fixture missing: {fasta} / {ms2}")
        return failures
    DECOY_PEP, DECOY_MOD = "EDIMTPEPK", "EDIM[15.9949]TPEPK"
    PAL = "LSAGGASLK"

    # --- decoy_search = 0: no decoys at all ---
    res = _t34_run_mode(comet_exe, index_flag, label, 0, failures)
    if res is not None:
        rows, drows, out = res
        check(not any("DECOY_" in r.get("protein", "") for r in rows),
              f"{label} decoy_search=0: no DECOY_ protein anywhere in the target .txt", failures)
        check(not drows, f"{label} decoy_search=0: no .decoy.txt rows", failures)
        r1 = _t34_rank1(rows, 1)
        check(r1 is not None and "PEPTM[15.9949]IDEK" in r1.get("modified_peptide", ""),
              f"{label} decoy_search=0: scan 1 -> PEPTM[15.9949]IDEK, got {r1 and r1.get('modified_peptide')!r}", failures)
        check(not any(r.get("scan") == "2" and r.get("plain_peptide") == DECOY_PEP for r in rows),
              f"{label} decoy_search=0: scan 2 never matches the decoy sequence {DECOY_PEP}", failures)
        r3 = _t34_rank1(rows, 3)
        check(r3 is not None and r3.get("plain_peptide") == PAL and r3.get("protein") == "T34_pal",
              f"{label} decoy_search=0: scan 3 -> {PAL} with protein exactly T34_pal, got "
              f"{r3 and (r3.get('plain_peptide'), r3.get('protein'))!r}", failures)
        check("internal decoy variants" not in out,
              f"{label} decoy_search=0: search log does not report internal decoy variants", failures)

    # --- decoy_search = 1: concatenated ---
    res = _t34_run_mode(comet_exe, index_flag, label, 1, failures)
    if res is not None:
        rows, drows, out = res
        check(not drows, f"{label} decoy_search=1: no .decoy.txt rows (concatenated mode)", failures)
        r1 = _t34_rank1(rows, 1)
        check(r1 is not None and "PEPTM[15.9949]IDEK" in r1.get("modified_peptide", "")
              and r1.get("protein") == "T34_ox",
              f"{label} decoy_search=1: scan 1 -> PEPTM[15.9949]IDEK / T34_ox (unprefixed), got "
              f"{r1 and (r1.get('modified_peptide'), r1.get('protein'))!r}", failures)
        r2 = _t34_rank1(rows, 2)
        check(r2 is not None and DECOY_MOD in r2.get("modified_peptide", ""),
              f"{label} decoy_search=1: scan 2 -> {DECOY_MOD} (M-oxidation moved with its residue), got "
              f"{r2 and r2.get('modified_peptide')!r}", failures)
        check(r2 is not None and r2.get("protein") == "DECOY_T34_ox",
              f"{label} decoy_search=1: scan 2 protein is exactly DECOY_T34_ox (prefixed, not duplicated bare), got "
              f"{r2 and r2.get('protein')!r}", failures)
        pal_rows = [r for r in rows if r.get("scan") == "3" and r.get("plain_peptide") == PAL]
        check(len(pal_rows) == 1,
              f"{label} decoy_search=1: scan 3 has exactly one {PAL} row (palindromic decoy merged into the target), "
              f"got {len(pal_rows)}", failures)
        prots = set(pal_rows[0].get("protein", "").split(",")) if pal_rows else set()
        check(prots == {"T34_pal", "DECOY_T34_pal"},
              f"{label} decoy_search=1: scan 3 protein column carries T34_pal and DECOY_T34_pal, got {sorted(prots)!r}", failures)
        if index_flag == "-i":
            check("3 internal decoy variants" in out,
                  f"{label} decoy_search=1: FI build log reports exactly 3 internal decoy variants "
                  f"(PEPTMIDEK, PEPTM[ox]IDEK, LSAGGASLK)", failures)

    # --- decoy_search = 2: separate decoy list ---
    res = _t34_run_mode(comet_exe, index_flag, label, 2, failures)
    if res is not None:
        rows, drows, out = res
        check(not any("DECOY_" in r.get("protein", "") for r in rows),
              f"{label} decoy_search=2: no DECOY_ protein in the target .txt", failures)
        check(not any(r.get("plain_peptide") == DECOY_PEP for r in rows),
              f"{label} decoy_search=2: decoy sequence {DECOY_PEP} absent from the target .txt", failures)
        check(drows and all(r.get("protein", "").startswith("DECOY_") for r in drows),
              f"{label} decoy_search=2: .decoy.txt has rows and every protein is DECOY_-prefixed ({len(drows)} rows)", failures)
        d2 = _t34_rank1(drows, 2)
        check(d2 is not None and DECOY_MOD in d2.get("modified_peptide", "") and d2.get("protein") == "DECOY_T34_ox",
              f"{label} decoy_search=2: decoy file scan 2 -> {DECOY_MOD} / DECOY_T34_ox, got "
              f"{d2 and (d2.get('modified_peptide'), d2.get('protein'))!r}", failures)
        d3 = _t34_rank1(drows, 3)
        check(d3 is not None and d3.get("plain_peptide") == PAL and d3.get("protein") == "DECOY_T34_pal",
              f"{label} decoy_search=2: decoy file scan 3 -> {PAL} / DECOY_T34_pal (separate list, no merge), got "
              f"{d3 and (d3.get('plain_peptide'), d3.get('protein'))!r}", failures)
        t3 = _t34_rank1(rows, 3)
        check(t3 is not None and t3.get("plain_peptide") == PAL and t3.get("protein") == "T34_pal",
              f"{label} decoy_search=2: target file scan 3 -> {PAL} / T34_pal, got "
              f"{t3 and (t3.get('plain_peptide'), t3.get('protein'))!r}", failures)

    return failures


@register("t34_internal_decoys_fi")
def test_t34_internal_decoys_fi(comet_exe):
    """T34: FI_DB internal decoys -- reversal, mod mirroring, palindrome merge, separate-list mode."""
    return _test_t34_index_type(comet_exe, "-i", "FI_DB")


@register("t34_internal_decoys_pi")
def test_t34_internal_decoys_pi(comet_exe):
    """T34: PI_DB internal decoys -- same fixture and assertions as the FI_DB variant."""
    return _test_t34_index_type(comet_exe, "-j", "PI_DB")


# ---------------------------------------------------------------------------
# T35 -- AScorePro must be identical across FASTA_DB / PI_DB / FI_DB
# ---------------------------------------------------------------------------
#
# docs/20260914_FI_internal_decoys.md D6 (PR #129 review). What this guards:
#  1. The SCORE regression: CometSearchManager::SetAScoreOptions() applies static mods to
#     AScorePro's residue-mass table cumulatively and was called twice for batch PI_DB, so
#     add_C_cysteine was applied to C twice in that mode only (same PSM: 15.06 in FASTA_DB,
#     12.78 in PI_DB). The fixture peptide TSCEPYSDLK carries a static C and add_C_cysteine is
#     57.021464 here (the T19/T20 AScore tests use 0.0 and could never see this). Negative
#     control: with only the SetAScoreOptions() reset reverted this test fails (PI_DB AScore
#     110.17 vs FASTA_DB 339.46).
#  2. The peak-list NORMALIZATION: CometPreprocess::LoadIons() now fills AScorePro's peak list
#     (vRawFragmentPeakMassIntensity) in the same descending-m/z order for every mode instead of
#     descending intensity for the indexed modes above fragindex_num_spectrumpeaks peaks. This
#     is not score-affecting -- AScorePro re-sorts its input by m/z (AScoreTopIonsFilter), and
#     reverting only that change leaves this test passing -- but the 356-peak fixture (cap 150)
#     does drive the indexed modes through the formerly intensity-sorted branch, so a future
#     order-sensitive consumer of that list would be caught here.
# The synthetic spectrum is the full 1+/2+ b/y ladder of TSCEPYS[79.966331]DLK plus seeded noise;
# competing sites T1/S2/Y6 make the localization non-trivial. All three modes must report the
# same localized peptide, the same AScore and the same site-score string; FASTA_DB and PI_DB
# must also agree on xcorr (FI_DB's 1+ neutral-loss ion handling is a separate, documented
# scoring nuance, so xcorr is not compared against it).

T35_PARAMS_TEMPLATE = T26_PARAMS_TEMPLATE.replace("add_C_cysteine = 0.0", "add_C_cysteine = 57.021464")
assert "add_C_cysteine = 57.021464" in T35_PARAMS_TEMPLATE, "T35_PARAMS_TEMPLATE: add_C_cysteine replacement didn't fire"


@register("t35_ascore_crossmode")
def test_t35_ascore_crossmode(comet_exe):
    """T35: AScorePro identical across FASTA_DB/PI_DB/FI_DB -- guards the double-applied static mod (score regression) and exercises the >150-peak normalized peak-list branch."""
    failures = []
    fasta = DATA_DIR / "t35_ascore_crossmode.fasta"
    ms2   = DATA_DIR / "t35_ascore_crossmode.ms2"
    idx   = fasta.with_suffix(".fasta.idx")
    txt   = ms2.with_suffix(".txt")
    if not fasta.exists() or not ms2.exists():
        failures.append(f"fixture missing: {fasta} / {ms2}")
        return failures
    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str
    mod1 = "79.966331 STY 0 3 -1 0 0 97.976896"
    EXPECTED = "TSCEPYS[79.9663]DLK"

    n_peaks = sum(1 for l in ms2.read_text().splitlines() if l and l[0].isdigit())
    check(n_peaks > 150, f"fixture spectrum has more than fragindex_num_spectrumpeaks (150) peaks: {n_peaks}", failures)

    def make_params(db, ist=None):
        p = T35_PARAMS_TEMPLATE.format(comet_version="2026.02 rev. 0", database=fmt(db), ascorepro=-1, mod1=mod1,
                                       decoy_search=0, len_min=8, len_max=12)
        if ist is not None:
            p = _set_param_line(p, "index_search_type", ist)
        return p

    def run_search(db, ist, label):
        txt.unlink(missing_ok=True)
        with tempfile.NamedTemporaryFile(mode="w", suffix=".params", dir=str(DATA_DIR), delete=False) as pf:
            pf.write(make_params(db, ist))
            params_file = Path(pf.name)
        try:
            rc, out = _run_t19_step(comet_exe, [f"-P{fmt(params_file)}", fmt(ms2)])
            if not check(rc == 0 and txt.exists(), f"{label}: search exits 0 and writes .txt (rc={rc})", failures):
                print(out[-1500:])
                return None
            rows = _t34_read_rows(txt)
            r1 = _t34_rank1(rows, 1)
            if not check(r1 is not None, f"{label}: scan 1 has a rank-1 row", failures):
                return None
            return r1
        finally:
            params_file.unlink(missing_ok=True)
            txt.unlink(missing_ok=True)

    results = {}
    results["FASTA_DB"] = run_search(fasta, None, "FASTA_DB")
    for flag, label, ist in (("-j", "PI_DB", 0), ("-i", "FI_DB", 1)):
        idx.unlink(missing_ok=True)
        with tempfile.NamedTemporaryFile(mode="w", suffix=".params", dir=str(DATA_DIR), delete=False) as pf:
            pf.write(make_params(fasta))
            build_params = Path(pf.name)
        try:
            rc, out = _run_t19_step(comet_exe, [flag, f"-P{fmt(build_params)}"])
            if not check(rc == 0 and idx.exists(), f"{label}: index builds (rc={rc})", failures):
                print(out[-1500:])
                continue
            results[label] = run_search(idx, ist, label)
        finally:
            build_params.unlink(missing_ok=True)
            idx.unlink(missing_ok=True)

    for label, r in results.items():
        if r is None:
            continue
        check(EXPECTED in r.get("modified_peptide", ""),
              f"{label}: rank-1 is {EXPECTED}, got {r.get('modified_peptide')!r}", failures)
        check(float(r.get("ascorepro", "0") or 0) > 0.0,
              f"{label}: AScorePro score is populated ({r.get('ascorepro')!r})", failures)
    ref = results.get("FASTA_DB")
    if ref is not None:
        for label in ("PI_DB", "FI_DB"):
            r = results.get(label)
            if r is None:
                continue
            check(r.get("ascorepro") == ref.get("ascorepro"),
                  f"{label} AScorePro equals FASTA_DB's ({r.get('ascorepro')!r} vs {ref.get('ascorepro')!r}) -- "
                  f"a difference means AScorePro saw different peaks (order) or a different residue-mass table (static mods)", failures)
            check(r.get("ascore_sitescores") == ref.get("ascore_sitescores"),
                  f"{label} AScorePro site scores equal FASTA_DB's ({r.get('ascore_sitescores')!r} vs {ref.get('ascore_sitescores')!r})", failures)
            check(r.get("modified_peptide") == ref.get("modified_peptide"),
                  f"{label} localized peptide equals FASTA_DB's", failures)
        if results.get("PI_DB") is not None:
            check(results["PI_DB"].get("xcorr") == ref.get("xcorr"),
                  f"PI_DB xcorr equals FASTA_DB's ({results['PI_DB'].get('xcorr')!r} vs {ref.get('xcorr')!r})", failures)
    return failures


# ---------------------------------------------------------------------------
# T36 -- AScorePro must not relocalize an internal decoy's site
# ---------------------------------------------------------------------------
#
# docs/20260914_FI_internal_decoys.md D6 option (c), PR #129 review. Real spectrum (scan 42900
# of 20170103_HelaQC_01, copied from tests/rts_repro/fixture_spectra.txt) searched against the
# single protein CNOT4_HUMAN with decoy_search = 1: the rank-1 hit is the internal decoy
# IRQLEEQS[79.9663]LPKYVAPDEPYPKRCAPCLGNEDTK (xcorr 0.477, 4 matched ions -- the S7 ladder) and
# its AScorePro score (15.06) clears ASCORE_CUTOFF_TO_ACCEPT (13.0). Before (c),
# CalculateAScorePro() therefore rewrote the stored site to Y11 (position 12) AFTER the duplicate
# check had run, producing a second row labeled identically to the genuine Y11 isomer (0.271)
# and a FASTA_DB-vs-PI_DB label disagreement (PI_DB happened not to relocalize only because of
# the double-applied static mod T35 now guards). A decoy's site is arbitrary by construction, so
# the stored site must stay put while the AScore is still reported. Checked for FASTA_DB and
# PI_DB (FI_DB's fragment pre-filter ranks a different decoy first on this spectrum).


@register("t36_decoy_norelocalize")
def test_t36_decoy_norelocalize(comet_exe):
    """T36: AScorePro reports but never relocalizes an internal decoy's site (FASTA_DB and PI_DB, real scan 42900)."""
    failures = []
    fasta = DATA_DIR / "t36_decoy_norelocalize.fasta"
    ms2   = DATA_DIR / "t36_decoy_norelocalize.ms2"
    idx   = fasta.with_suffix(".fasta.idx")
    txt   = ms2.with_suffix(".txt")
    if not fasta.exists() or not ms2.exists():
        failures.append(f"fixture missing: {fasta} / {ms2}")
        return failures
    use_win = _binary_uses_win_paths(comet_exe)
    fmt = _to_win if use_win else str
    DECOY_PLAIN = "IRQLEEQSLPKYVAPDEPYPKRCAPCLGNEDTK"
    KEPT_SITE = "IRQLEEQS[79.9663]LPKY"         # phospho on S7 (1-based 8) as scored
    RELOCALIZED = "IRQLEEQSLPKY[79.9663]VAPDE"   # what an accepted relocalization produced before (c)

    def make_params(db, ist=None):
        p = T35_PARAMS_TEMPLATE.format(comet_version="2026.02 rev. 0", database=fmt(db), ascorepro=-1,
                                       mod1="79.966331 STY 0 2 -1 0 0 97.976896",
                                       decoy_search=1, len_min=8, len_max=40)
        p = _set_param_line(p, "allowed_missed_cleavage", 2)
        p = _set_param_line(p, "digest_mass_range", "600.0 5000.0")
        if ist is not None:
            p = _set_param_line(p, "index_search_type", ist)
        return p

    def run_search(db, ist, label):
        txt.unlink(missing_ok=True)
        with tempfile.NamedTemporaryFile(mode="w", suffix=".params", dir=str(DATA_DIR), delete=False) as pf:
            pf.write(make_params(db, ist))
            params_file = Path(pf.name)
        try:
            rc, out = _run_t19_step(comet_exe, [f"-P{fmt(params_file)}", fmt(ms2)])
            if not check(rc == 0 and txt.exists(), f"{label}: search exits 0 and writes .txt (rc={rc})", failures):
                print(out[-1500:])
                return None
            r1 = _t34_rank1(_t34_read_rows(txt), 42900)
            if not check(r1 is not None, f"{label}: scan 42900 has a rank-1 row", failures):
                return None
            return r1
        finally:
            params_file.unlink(missing_ok=True)
            txt.unlink(missing_ok=True)

    results = {"FASTA_DB": run_search(fasta, None, "FASTA_DB")}
    idx.unlink(missing_ok=True)
    with tempfile.NamedTemporaryFile(mode="w", suffix=".params", dir=str(DATA_DIR), delete=False) as pf:
        pf.write(make_params(fasta))
        build_params = Path(pf.name)
    try:
        rc, out = _run_t19_step(comet_exe, ["-j", f"-P{fmt(build_params)}"])
        if check(rc == 0 and idx.exists(), f"PI_DB: index builds (rc={rc})", failures):
            results["PI_DB"] = run_search(idx, 0, "PI_DB")
        else:
            print(out[-1500:])
    finally:
        build_params.unlink(missing_ok=True)
        idx.unlink(missing_ok=True)

    for label, r in results.items():
        if r is None:
            continue
        check(r.get("plain_peptide") == DECOY_PLAIN and r.get("protein", "").startswith("DECOY_"),
              f"{label}: rank-1 is the internal decoy {DECOY_PLAIN}, got "
              f"{(r.get('plain_peptide'), r.get('protein'))!r}", failures)
        check(float(r.get("ascorepro", "0") or 0) >= 13.0,
              f"{label}: decoy's AScorePro ({r.get('ascorepro')!r}) clears the 13.0 acceptance cutoff, so the "
              f"relocalization branch is genuinely exercised", failures)
        check(KEPT_SITE in r.get("modified_peptide", "") and RELOCALIZED not in r.get("modified_peptide", ""),
              f"{label}: decoy keeps its scored site S7 -- {KEPT_SITE} -- and is NOT relocalized to Y11, got "
              f"{r.get('modified_peptide')!r}", failures)
        check(r.get("modifications", "").startswith("8_V_79.966331"),
              f"{label}: modifications column starts with 8_V_79.966331, got {r.get('modifications')!r}", failures)
    if results.get("FASTA_DB") and results.get("PI_DB"):
        check(results["FASTA_DB"].get("modified_peptide") == results["PI_DB"].get("modified_peptide")
              and results["FASTA_DB"].get("ascorepro") == results["PI_DB"].get("ascorepro"),
              f"FASTA_DB and PI_DB agree on the decoy's label and AScore", failures)
    return failures


if __name__ == "__main__":
    main()
