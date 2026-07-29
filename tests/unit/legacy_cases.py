#!/usr/bin/env python3
"""
Legacy functional-correctness fixtures migrated from
/mnt/c/Work/20130226-comet-tests/runall.sh.

That directory judged correctness by eye: output_sqtstream=1 dumped raw SQT
records to stdout, and a human compared them against prose in each case's
README. Here the same 21 cases run with output_txtfile=1 and are checked by
parsing the tab-delimited .txt output -- see PSM_HEADER / parse_txt() below,
and each entry's "check" callback for the actual assertion.

Fixtures live in tests/unit/data/legacy/<case>/ (input.ms2 + tmp.db, or tmp1.db
for the commandline case). Three groups of cases share a byte-identical
database; those are deduplicated into tests/unit/data/legacy/db/ and
referenced from the case's own directory instead of duplicated:
  - ctermmod, term-mod, term-mod2       -> db/epgc_9entry.fasta
  - tryptic, semi-tryptic,
    peptide_mass_tolerance              -> db/yfds_68entry.fasta

No assertion here checks an absolute xcorr/e-value/deltaCn/ion-count -- those
legitimately drift across Comet versions (the original READMEs' pasted hit
tables are stale proof of that). Assertions are limited to peptide identity,
protein identity, modification presence, hit counts, and relative score
comparisons (fragmentNL/fragmentNL2), matching the count-stability philosophy
already used for T17.
"""

import textwrap
from pathlib import Path


LEGACY_DIR = Path(__file__).parent / "data" / "legacy"


ENZYME_INFO_BLOCK = textwrap.dedent("""\
    [COMET_ENZYME_INFO]
    0.  Cut_everywhere         0      -           -
    1.  Trypsin                1      KR          P
    2.  Trypsin/P              1      KR          -
    3.  Lys_C                  1      K           P
    4.  Lys_N                  0      K           -
    5.  Arg_C                  1      R           P
    6.  Asp_N                  0      D           -
    7.  CNBr                   1      M           -
    8.  Glu_C                  1      DE          P
    9.  PepsinA                1      FL          P
    10. Chymotrypsin           1      FWYL        P
    11. No_cut                 1      @           @
    """)

NULL_MOD = "0.0 X 0 3 -1 0 0 0.0"


def build_params(*, database, enzyme1=0, enzyme2=0, ntt=2, missed_cleavage=2,
                  mods=(), static_C=57.021464, static_R=0.0, static_M=0.0,
                  decoy_search=0, num_output_lines=5, max_varmods=5,
                  clip_nterm_met=0, legacy_pmt=False, num_threads=4,
                  comet_version="2026.02 rev. 0"):
    """Build a comet.params string for a legacy case. `mods` is a tuple of up
    to 5 literal `variable_modNN` value strings (mass residues binary
    max_mods term_distance n/c-term required neutral_loss), taken verbatim
    from the original comet.params.2025030 files -- padded with NULL_MOD."""
    mods = list(mods) + [NULL_MOD] * (5 - len(mods))

    lines = [
        f"# comet_version {comet_version}",
        f"database_name = {database}",
        f"decoy_search = {decoy_search}",
        f"num_threads = {num_threads}",
    ]
    if legacy_pmt:
        lines.append("peptide_mass_tolerance = 3.0")
    else:
        lines.append("peptide_mass_tolerance_upper = 3.0")
        lines.append("peptide_mass_tolerance_lower = -3.0")
    lines += [
        "peptide_mass_units = 0",
        "precursor_tolerance_type = 0",
        "isotope_error = 0",
        f"search_enzyme_number = {enzyme1}",
        f"search_enzyme2_number = {enzyme2}",
        "sample_enzyme_number = 0",
        f"num_enzyme_termini = {ntt}",
        f"allowed_missed_cleavage = {missed_cleavage}",
        f"variable_mod01 = {mods[0]}",
        f"variable_mod02 = {mods[1]}",
        f"variable_mod03 = {mods[2]}",
        f"variable_mod04 = {mods[3]}",
        f"variable_mod05 = {mods[4]}",
        f"max_variable_mods_in_peptide = {max_varmods}",
        "require_variable_mod = 0",
        "fragment_bin_tol = 1.0005",
        "fragment_bin_offset = 0.4",
        "theoretical_fragment_ions = 0",
        "use_A_ions = 0",
        "use_B_ions = 1",
        "use_C_ions = 0",
        "use_X_ions = 0",
        "use_Y_ions = 1",
        "use_Z_ions = 0",
        "use_Z1_ions = 0",
        "use_NL_ions = 0",
        "output_sqtfile = 0",
        "output_txtfile = 1",
        "output_pepxmlfile = 0",
        "output_mzidentmlfile = 0",
        "output_percolatorfile = 0",
        f"num_output_lines = {num_output_lines}",
        "scan_range = 0 0",
        "precursor_charge = 0 0",
        "override_charge = 0",
        "ms_level = 2",
        "activation_method = ALL",
        "digest_mass_range = 600.0 5000.0",
        "peptide_length_range = 5 50",
        "max_duplicate_proteins = -1",
        "max_fragment_charge = 3",
        "min_precursor_charge = 1",
        "max_precursor_charge = 6",
        f"clip_nterm_methionine = {clip_nterm_met}",
        "spectrum_batch_size = 15000",
        "decoy_prefix = DECOY_",
        "equal_I_and_L = 1",
        "mass_offsets =",
        "minimum_peaks = 10",
        "minimum_intensity = 0",
        "remove_precursor_peak = 0",
        "remove_precursor_tolerance = 1.5",
        "clear_mz_range = 0.0 0.0",
        "percentage_base_peak = 0.0",
        "add_Cterm_peptide = 0.0",
        "add_Nterm_peptide = 0.0",
        "add_Cterm_protein = 0.0",
        "add_Nterm_protein = 0.0",
        "add_G_glycine = 0.0",
        "add_A_alanine = 0.0",
        "add_S_serine = 0.0",
        "add_P_proline = 0.0",
        "add_V_valine = 0.0",
        "add_T_threonine = 0.0",
        f"add_C_cysteine = {static_C}",
        "add_L_leucine = 0.0",
        "add_I_isoleucine = 0.0",
        "add_N_asparagine = 0.0",
        "add_D_aspartic_acid = 0.0",
        "add_Q_glutamine = 0.0",
        "add_K_lysine = 0.0",
        "add_E_glutamic_acid = 0.0",
        f"add_M_methionine = {static_M}",
        "add_H_histidine = 0.0",
        "add_F_phenylalanine = 0.0",
        "add_U_selenocysteine = 0.0",
        f"add_R_arginine = {static_R}",
        "add_Y_tyrosine = 0.0",
        "add_W_tryptophan = 0.0",
        "add_O_pyrrolysine = 0.0",
        "add_B_user_amino_acid = 0.0",
        "add_J_user_amino_acid = 0.0",
        "add_X_user_amino_acid = 0.0",
        "add_Z_user_amino_acid = 0.0",
        ENZYME_INFO_BLOCK.rstrip("\n"),
    ]
    return "\n".join(lines) + "\n"


def parse_txt(txt_path):
    """Parse a Comet output_txtfile into a list of row dicts, rank order
    (line order == 'num' ascending, as Comet writes it)."""
    lines = Path(txt_path).read_text().splitlines()
    header = lines[1].split("\t")
    return [dict(zip(header, l.split("\t"))) for l in lines[2:] if l.strip()]


def mod_mass_count(row, mass_str):
    """Count how many modifications on this row carry the given mass
    (e.g. '15.994900' or '57.021464'), regardless of position/type/terminus."""
    mods = row.get("modifications", "")
    if not mods or mods == "-":
        return 0
    return sum(1 for entry in mods.split(",") if entry.split("_")[-1].startswith(mass_str)
               or (len(entry.split("_")) > 3 and entry.split("_")[2] == mass_str))


# ---------------------------------------------------------------------------
# Case table
# ---------------------------------------------------------------------------
# Each case: fixture dir name, ms2 filename, database arg (relative to the
# case dir unless it starts with "../", meaning tests/unit/data/legacy/db/),
# extra command-line args, build_params() kwargs, and a check(rows, failures)
# callback. `rows` is the parsed .txt output (empty list if no PSMs).

def _check_top1_peptide(peptide, mods_mass=None, protein=None):
    def check(rows, failures, check_fn):
        if not check_fn(len(rows) >= 1, "at least 1 PSM row reported", failures):
            return
        row = rows[0]
        check_fn(row.get("plain_peptide") == peptide,
                  f"rank-1 plain_peptide: expected {peptide}, got {row.get('plain_peptide')!r}", failures)
        if mods_mass is not None:
            check_fn(mod_mass_count(row, mods_mass) >= 1,
                      f"rank-1 modifications contains mass {mods_mass}: got {row.get('modifications')!r}", failures)
        if protein is not None:
            check_fn(row.get("protein") == protein,
                      f"rank-1 protein: expected {protein}, got {row.get('protein')!r}", failures)
    return check


LEGACY_CASES = {}


def _register(name, **kwargs):
    LEGACY_CASES[name] = kwargs


# --- plain / commandline / binarymod2: same 1-entry EPGCGCCMTCALAEGQSCGVYTER DB ---
_EPGC_MODS = ("15.9949 M 0 3 -1 0 0 0.0",)

_register("plain",
    dir="plain", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=0, mods=_EPGC_MODS),
    check=_check_top1_peptide("EPGCGCCMTCALAEGQSCGVYTER"))

_register("commandline",
    dir="commandline", ms2="input.ms2", database="tmp.db",  # deliberately absent; -D overrides
    extra_args=["-D{tmp1_db}"],
    params=dict(enzyme1=0, mods=_EPGC_MODS),
    check=_check_top1_peptide("EPGCGCCMTCALAEGQSCGVYTER"))

_register("binarymod2",
    dir="binarymod2", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=0, mods=("15.9949 M 0 3 -1 0 0 0.0", "55.0 R 1 3 -1 0 0 0.0"),
                static_R=-55.00, decoy_search=1),
    check=_check_top1_peptide("EPGCGCCMTCALAEGQSCGVYTER"))


# --- 2enzyme / 2enzyme_fail: KEPGCGCCMTCALAEGQSCGVYTERS, K/S flanks ---
def _check_2enzyme(rows, failures, check_fn):
    if not check_fn(len(rows) >= 1, "at least 1 PSM row reported", failures):
        return
    row = rows[0]
    check_fn(row.get("plain_peptide") == "EPGCGCCMTCALAEGQSCGVYTER",
              f"rank-1 plain_peptide: got {row.get('plain_peptide')!r}", failures)
    check_fn(row.get("prev_aa") == "K", f"prev_aa == K, got {row.get('prev_aa')!r}", failures)
    check_fn(row.get("next_aa") == "S", f"next_aa == S, got {row.get('next_aa')!r}", failures)


_register("2enzyme",
    dir="2enzyme", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=5, enzyme2=3, mods=_EPGC_MODS),
    check=_check_2enzyme)

_register("2enzyme_fail",
    dir="2enzyme_fail", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=5, enzyme2=0, mods=_EPGC_MODS),
    check=lambda rows, failures, check_fn: check_fn(
        len(rows) == 0, f"expected 0 PSM rows (2nd enzyme unset), got {len(rows)}", failures))


# --- requirevarmod: every reported peptide has a +15.9949 mod ---
def _check_requirevarmod(rows, failures, check_fn):
    check_fn(len(rows) >= 1, "at least 1 PSM row reported", failures)
    for row in rows:
        check_fn(mod_mass_count(row, "15.9949") >= 1 or mod_mass_count(row, "15.994900") >= 1,
                  f"scan {row.get('scan')} rank {row.get('num')} carries +15.9949 mod "
                  f"(modifications={row.get('modifications')!r})", failures)


_register("requirevarmod",
    dir="requirevarmod", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=1, ntt=1, num_output_lines=10,
                mods=("15.9949 M 0 3 -1 0 1 0.0",)),  # required=1 in the mod's own 7th field
    check=_check_requirevarmod)


# --- autodecoy / autodecoy1: decoy peptide must win ---
_register("autodecoy",
    dir="autodecoy", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=1, mods=_EPGC_MODS, decoy_search=1),
    check=_check_top1_peptide("YVVESTGVFTTMEK", protein="DECOY_tmp2"))

_register("autodecoy1",
    dir="autodecoy1", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=1, mods=_EPGC_MODS, static_M=-15.9949, decoy_search=1),
    check=_check_top1_peptide("YVVESTGVFTTMEK", mods_mass="15.9949", protein="DECOY_tmp2"))


# --- binarymods: all-or-nothing binary mod group -> all 5 C's modified ---
def _check_binarymods(rows, failures, check_fn):
    if not check_fn(len(rows) >= 1, "at least 1 PSM row reported", failures):
        return
    row = rows[0]
    check_fn(row.get("plain_peptide") == "EPGCGCCMTCALAEGQSCGVYTER",
              f"rank-1 plain_peptide: got {row.get('plain_peptide')!r}", failures)
    n_cys_mods = mod_mass_count(row, "57.021464")
    check_fn(n_cys_mods == 5,
              f"rank-1 has all 5 cysteines modified (binary group), got {n_cys_mods}", failures)


_register("binarymods",
    dir="binarymods", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=0, num_output_lines=10, static_C=0.0,
                mods=("15.9949 M 0 3 -1 0 0 0.0",
                      "57.021464 C 1 5 -1 0 0 0.0",
                      "57.021464 C 2 5 -1 0 0 0.0")),
    check=_check_binarymods)


# --- clipntermmet: natural tmp3 hit (I/L-equivalent) and clip-Met tmp hit merge
# into one row under equal_I_and_L=1 -- both proteins listed, protein_count==2.
def _check_clipntermmet(rows, failures, check_fn):
    if not check_fn(len(rows) >= 1, "at least 1 PSM row reported", failures):
        return
    row = rows[0]
    check_fn(row.get("plain_peptide") == "YFDSFGDLSSASAIMGNPK",
              f"rank-1 plain_peptide: got {row.get('plain_peptide')!r}", failures)
    proteins = set(row.get("protein", "").split(","))
    check_fn(proteins == {"tmp", "tmp3"},
              f"rank-1 merges clip-Met protein 'tmp' and I/L-equivalent natural protein "
              f"'tmp3' (equal_I_and_L=1), got protein={row.get('protein')!r}", failures)
    check_fn(row.get("protein_count") == "2",
              f"protein_count == 2, got {row.get('protein_count')!r}", failures)


_register("clipntermmet",
    dir="clipntermmet", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=1, mods=_EPGC_MODS, clip_nterm_met=1),
    check=_check_clipntermmet)


# --- fragmentNL / fragmentNL2: handled specially (two runs) in run_tests.py ---
_register("fragmentNL",
    dir="fragmentNL", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=1, decoy_search=1, mods=("15.9949 M 0 3 -1 0 0 0.0",)),
    nl_mod_index=0, nl_value=10.0,
    check=None)  # asserted specially: xcorr(NL) > xcorr(base)

_register("fragmentNL2",
    dir="fragmentNL2", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=1, mods=("15.9949 M 0 3 -1 0 0 0.0", "79.966331 STY 0 3 -1 0 0 0.0")),
    nl_mod_index=1, nl_value=97.976896,
    check=None)


# --- multiplemods: rank-1 peptide with >=1 cysteine modified ---
def _check_multiplemods(rows, failures, check_fn):
    if not check_fn(len(rows) >= 1, "at least 1 PSM row reported", failures):
        return
    row = rows[0]
    check_fn(row.get("plain_peptide") == "EPGCGCCMTCALAEGQSCGVYTER",
              f"rank-1 plain_peptide: got {row.get('plain_peptide')!r}", failures)
    n_cys_mods = mod_mass_count(row, "57.021464") + mod_mass_count(row, "114.042928")
    check_fn(n_cys_mods >= 1, f"rank-1 has >=1 modified cysteine, got {n_cys_mods}", failures)


_register("multiplemods",
    dir="multiplemods", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=1, num_output_lines=50, max_varmods=6, static_C=0.0,
                mods=("15.9949 M 0 3 -1 0 0 0.0",
                      "57.021464 C 0 6 -1 0 0 0.0",
                      "114.042928 C 0 6 -1 0 0 0.0")),
    check=_check_multiplemods)


# --- noenzyme / term-mod2: 4 terminal-mod permutations of YFDSFGDLSSASAIMGNPK ---
_TERM_MODS = ("15.9949 M 0 3 -1 0 0 0.0",
              "128.094963050 c 0 3 -1 0 0 0.0",
              "163.063328575 n 0 3 -1 0 0 0.0")


def _permutation_variants(target_peptide):
    """A c-term or n-term differential mod exactly compensates for a missing
    terminal residue's mass, so Comet reports the truncated core peptide
    (with the compensating mod) as mass-indistinguishable from the intact
    peptide -- these are the "permutations" the original READMEs describe
    (e.g. 'Y]FDSFGDLSSASAIM*GNP[K' == a peptide missing its leading Y,
    compensated by an n-term diff mod of ~Y's residue mass)."""
    return {target_peptide, target_peptide[1:], target_peptide[:-1], target_peptide[1:-1]}


def _check_permutations(target_peptide, n_top, both_termini=False):
    def check(rows, failures, check_fn):
        if not check_fn(len(rows) >= n_top, f"at least {n_top} PSM rows reported", failures):
            return
        top = rows[:n_top]
        variants = _permutation_variants(target_peptide)
        peptides = [r.get("plain_peptide") for r in top]
        check_fn(all(p in variants for p in peptides),
                  f"top {n_top} hits are all permutations of {target_peptide} "
                  f"(allowed: {sorted(variants)}), got {peptides}", failures)
        masses = {r.get("calc_neutral_mass") for r in top}
        check_fn(len(masses) == 1,
                  f"top {n_top} hits share one calc_neutral_mass (mass-equivalent "
                  f"permutations), got {sorted(masses)}", failures)
        if both_termini:
            has_both = any("_n" in r.get("modifications", "") and "_c" in r.get("modifications", "")
                            for r in top)
            check_fn(has_both,
                      "at least one top hit carries both an n-term and a c-term mod", failures)
    return check


_register("noenzyme",
    dir="noenzyme", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=0, mods=_TERM_MODS),
    check=_check_permutations("YFDSFGDLSSASAIMGNPK", 4))

_register("term-mod2",
    dir="term-mod2", ms2="input.ms2", database="../db/epgc_9entry.fasta",
    params=dict(enzyme1=0, mods=_TERM_MODS),
    check=_check_permutations("YFDSFGDLSSASAIMGNPK", 4, both_termini=True))

_register("term-mod",
    dir="term-mod", ms2="input.ms2", database="../db/epgc_9entry.fasta",
    params=dict(enzyme1=1, ntt=1, mods=_TERM_MODS),
    check=_check_permutations("YFDSFGDLSSASAIMGNPK", 3))

_register("ctermmod",
    dir="ctermmod", ms2="input.ms2", database="../db/epgc_9entry.fasta",
    params=dict(enzyme1=1, ntt=1, mods=("15.9949 M 0 3 -1 0 0 0.0",
                                        "128.094963050 c 0 3 -1 0 0 0.0")),
    check=_check_top1_peptide("YFDSFGDLSSASAIMGNP", mods_mass="128.094963"))


# --- stop-codon: correct hit, and the deliberately-scrambled protein never appears ---
def _check_stop_codon(rows, failures, check_fn):
    if not check_fn(len(rows) >= 1, "at least 1 PSM row reported", failures):
        return
    check_fn(rows[0].get("plain_peptide") == "EPGCGCCMTCAALEGQSCGVYTER",
              f"rank-1 plain_peptide: got {rows[0].get('plain_peptide')!r}", failures)
    proteins = {r.get("protein") for r in rows}
    check_fn("should_not_find" not in proteins,
              "protein 'should_not_find' never appears in results", failures)


_register("stop-codon",
    dir="stop-codon", ms2="input.ms2", database="tmp.db",
    params=dict(enzyme1=3, mods=_EPGC_MODS, num_threads=1),
    check=_check_stop_codon)


# --- semi-tryptic / tryptic / peptide_mass_tolerance: shared 68-entry DB ---
_register("semi-tryptic",
    dir="semi-tryptic", ms2="input.ms2", database="../db/yfds_68entry.fasta",
    params=dict(enzyme1=1, ntt=1, mods=_TERM_MODS),
    check=_check_permutations("YFDSFGDLSSASAIMGNPK", 3))

_register("tryptic",
    dir="tryptic", ms2="input.ms2", database="../db/yfds_68entry.fasta",
    params=dict(enzyme1=1, ntt=2, mods=_TERM_MODS),
    check=_check_permutations("YFDSFGDLSSASAIMGNPK", 3))

_register("peptide_mass_tolerance",
    dir="peptide_mass_tolerance", ms2="input.ms2", database="../db/yfds_68entry.fasta",
    params=dict(enzyme1=1, ntt=2, mods=("15.9949 M 0 3 -1 0 0 0.0",), legacy_pmt=True),
    check=_check_top1_peptide("YFDSFGDLSSASAIMGNPK", mods_mass="15.9949"))
