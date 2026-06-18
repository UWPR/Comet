#!/usr/bin/env python3
"""
Windows .raw file support test -- compares the same Windows Comet binary
searching the identical Hela run via .mzXML vs .raw, across all 5 output
formats (txt, sqt, pep.xml, mzid, pin).

Only the Windows release reads .raw files directly (Thermo vendor library);
this test is SKIPPED (exit 0, not a failure) when given a non-Windows binary
or when the .raw fixture is absent, since both are expected/documented
conditions rather than test failures.

Goal: confirm (a) .raw file reading works correctly -- the .mzXML and .raw
searches should agree "near exactly" (same underlying spectra, two different
encodings, so tiny floating-point/centroiding differences are tolerated but
not large disagreements) -- and (b) every enabled output format is valid and
non-empty for both input formats, not just the default .txt.

Usage:
    python test_raw_vs_mzxml.py
    python test_raw_vs_mzxml.py --comet ../../x64/Release/Comet.exe
    python test_raw_vs_mzxml.py --data  ../../data
"""

import argparse
import re
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.resolve()))
import run_regression as rr  # reuse run_comet/parse_txt/compare_results/patch_params/etc.

REGRESSION_DIR    = Path(__file__).parent.resolve()
REPO_ROOT         = REGRESSION_DIR.parent.parent
DATA_DIR          = REPO_ROOT / "data"
DEFAULT_COMET_WIN = REPO_ROOT / "x64" / "Release" / "Comet.exe"

FASTA_FILE  = DATA_DIR / "human.small.fasta"
MZXML_FILE  = DATA_DIR / "20250520_Hela_60min_06.mzXML"
RAW_FILE    = DATA_DIR / "20250520_Hela_60min_06.raw"
PARAMS_FILE = DATA_DIR / "comet_phospho.params"

XCORR_THRESHOLD = 2.5    # same bar used by run_regression.py's .txt PSM comparison
MIN_AGREE_FRAC  = 0.99   # "near exact" bar -- not byte-exact, since .raw and .mzXML
                         # are two different encodings of the same underlying spectra
MAX_COUNT_DRIFT = 0.01   # 1% tolerance on record counts for the non-txt formats (these
                         # are spectrum-processed counts, not scoring-threshold-sensitive,
                         # so they should track each other tightly)
MAX_PSM_COUNT_DRIFT = 0.05  # 5% tolerance on the xcorr>=threshold PSM count itself --
                         # looser than MAX_COUNT_DRIFT because this count is sensitive to
                         # borderline scores flipping across the threshold from the tiny
                         # numeric differences between vendor-raw and converted-mzXML peaks

# format label -> (params flag to enable it, output file extension)
OUTPUT_FORMATS = {
    "txt":        ("output_txtfile",        ".txt"),
    "sqt":        ("output_sqtfile",        ".sqt"),
    "pepxml":     ("output_pepxmlfile",     ".pep.xml"),
    "mzidentml":  ("output_mzidentmlfile",  ".mzid"),
    "percolator": ("output_percolatorfile", ".pin"),
}


# ---------------------------------------------------------------------------
# Windows-binary / path helpers (binary-driven, not host-OS-driven -- this
# script is meant to invoke a Windows .exe from any host, e.g. via WSL interop)
# ---------------------------------------------------------------------------

def is_windows_binary(path: Path) -> bool:
    try:
        with open(path, "rb") as f:
            return f.read(2) == b"MZ"
    except Exception:
        return False


def to_win_path(p: Path) -> str:
    s = str(p)
    if s.startswith("/mnt/"):
        parts = s[5:].split("/", 1)
        drive = parts[0].upper() + ":"
        rest  = parts[1].replace("/", "\\") if len(parts) > 1 else ""
        return drive + "\\" + rest
    return s


# ---------------------------------------------------------------------------
# Lightweight per-format record counters (just enough to confirm "valid and
# not blank", plus a count to compare between the mzXML and .raw runs).
# Full peptide-level comparison is only done for .txt, via run_regression's
# already-proven parse_txt()/compare_results().
# ---------------------------------------------------------------------------

def count_sqt_spectra(path: Path) -> int:
    if not path.exists():
        return 0
    n = 0
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            if line.startswith("S\t"):
                n += 1
    return n


def count_pepxml_spectra(path: Path) -> int:
    if not path.exists():
        return 0
    text = path.read_text(encoding="utf-8", errors="replace")
    return text.count("<spectrum_query ")


def count_mzid_results(path: Path) -> int:
    if not path.exists():
        return 0
    text = path.read_text(encoding="utf-8", errors="replace")
    return len(re.findall(r"<SpectrumIdentificationResult id=", text))


def count_pin_rows(path: Path) -> int:
    if not path.exists():
        return 0
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    return max(0, len(lines) - 1)   # minus header line


RECORD_COUNTERS = {
    "sqt":        count_sqt_spectra,
    "pepxml":     count_pepxml_spectra,
    "mzidentml":  count_mzid_results,
    "percolator": count_pin_rows,
}


def close_enough(a: int, b: int, tol: float = MAX_COUNT_DRIFT) -> bool:
    if a == b:
        return True
    denom = max(a, b, 1)
    return abs(a - b) / denom <= tol


# ---------------------------------------------------------------------------
# Search execution
# ---------------------------------------------------------------------------

def run_one_search(comet: Path, params_path: Path, input_file: Path, work_dir: Path):
    """Run comet against input_file; return elapsed seconds."""
    elapsed, _ = rr.run_comet(
        comet,
        [f"-P{to_win_path(params_path)}", to_win_path(input_file)],
        work_dir,
    )
    return elapsed


def collect_outputs(input_file: Path, dest_dir: Path, label: str) -> dict:
    """
    Move (not copy) every produced output file from next to input_file into
    dest_dir/<label><ext>, so the next search (same basename, different
    source format) can't clobber it, and so data/ doesn't accumulate the
    side-effect output files Comet writes next to its input.
    """
    dest_dir.mkdir(parents=True, exist_ok=True)
    paths = {}
    stem = input_file.stem
    for fmt, (_flag, ext) in OUTPUT_FORMATS.items():
        src = input_file.parent / (stem + ext)
        dest = dest_dir / f"{label}{ext}"
        if src.exists():
            src.replace(dest)
            paths[fmt] = dest
        else:
            paths[fmt] = dest  # nonexistent; callers check .exists()
    return paths


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--comet", type=Path, default=DEFAULT_COMET_WIN,
                        help=f"Windows Comet binary (default: {DEFAULT_COMET_WIN})")
    parser.add_argument("--data", type=Path, default=DATA_DIR,
                        help=f"directory with FASTA, mzXML, raw, and params (default: {DATA_DIR})")
    args = parser.parse_args()

    fasta  = args.data / FASTA_FILE.name
    mzxml  = args.data / MZXML_FILE.name
    raw    = args.data / RAW_FILE.name
    params = args.data / PARAMS_FILE.name

    if not args.comet.exists():
        print(f"ERROR: Comet binary not found: {args.comet}", file=sys.stderr)
        sys.exit(1)

    if not is_windows_binary(args.comet):
        print(f"SKIP: {args.comet} is not a Windows binary (no 'MZ' PE header). "
              f"Only the Windows release reads .raw files directly; this test "
              f"only applies to a Windows Comet.exe.")
        sys.exit(0)

    if not raw.exists():
        print(f"SKIP: {raw} not found. The .raw fixture is gitignored / not "
              f"always present; this test requires it to compare against the "
              f".mzXML search.")
        sys.exit(0)

    for req, label in [(fasta, "FASTA"), (mzxml, "mzXML"), (params, "params")]:
        if not req.exists():
            print(f"ERROR: {label} file not found: {req}", file=sys.stderr)
            sys.exit(1)

    timestamp = time.strftime("%Y%m%d_%H%M%S")
    run_root  = REGRESSION_DIR / "results" / f"{timestamp}_raw_vs_mzxml"

    base_params = rr.load_params(params)
    overrides = {"database_name": to_win_path(fasta)}
    for _fmt, (flag, _ext) in OUTPUT_FORMATS.items():
        overrides[flag] = "1"
    search_params = rr.patch_params(base_params, overrides)
    params_path = run_root / "search.params"
    run_root.mkdir(parents=True, exist_ok=True)
    rr.write_params(search_params, params_path)

    print(f"Comet binary : {args.comet}")
    print(f"FASTA        : {fasta}")
    print(f"mzXML        : {mzxml}")
    print(f"raw          : {raw}")
    print()

    failures = []

    print("Running search against .mzXML ...")
    t_mzxml = run_one_search(args.comet, params_path, mzxml, run_root)
    mzxml_outputs = collect_outputs(mzxml, run_root, "mzxml")

    print("Running search against .raw ...")
    t_raw = run_one_search(args.comet, params_path, raw, run_root)
    raw_outputs = collect_outputs(raw, run_root, "raw")

    print()
    print(f"search time (.mzXML) : {t_mzxml:.1f} s")
    print(f"search time (.raw)   : {t_raw:.1f} s")
    print()

    # ---- .txt: full PSM-level comparison (peptide identity, xcorr) ----
    mzxml_psms = rr.parse_txt(mzxml_outputs["txt"])
    raw_psms   = rr.parse_txt(raw_outputs["txt"])
    cmp = rr.compare_results(mzxml_psms, raw_psms)

    print("Format: TXT (full PSM-level comparison)")
    print(f"  PSMs >= {XCORR_THRESHOLD} (.mzXML) : {cmp['base_psm_count']}")
    print(f"  PSMs >= {XCORR_THRESHOLD} (.raw)   : {cmp['curr_psm_count']}")
    if cmp["agree_frac"] is not None:
        print(f"  top-peptide agreement      : {cmp['agree_top_peptide']} / "
              f"{cmp['common_scans']} common scans ({cmp['agree_frac']*100:.2f}%)")
    else:
        print("  top-peptide agreement      : N/A (no common scans above threshold)")
    print(f"  only in .mzXML             : {cmp['only_in_baseline']}")
    print(f"  only in .raw               : {cmp['only_in_current']}")

    if cmp["base_psm_count"] == 0 or cmp["curr_psm_count"] == 0:
        failures.append("txt: one or both runs produced zero PSMs above threshold (blank output?)")
    if cmp["agree_frac"] is None or cmp["agree_frac"] < MIN_AGREE_FRAC:
        failures.append(f"txt: top-peptide agreement {cmp['agree_frac']} below "
                        f"the near-exact bar of {MIN_AGREE_FRAC}")
    if not close_enough(cmp["base_psm_count"], cmp["curr_psm_count"], MAX_PSM_COUNT_DRIFT):
        failures.append(f"txt: PSM counts not near-exact "
                        f"(.mzXML={cmp['base_psm_count']}, .raw={cmp['curr_psm_count']}, "
                        f"tolerance={MAX_PSM_COUNT_DRIFT*100:.0f}%)")

    # ---- sqt / pepxml / mzidentml / percolator: existence + record-count comparison ----
    for fmt, counter in RECORD_COUNTERS.items():
        mzxml_path = mzxml_outputs[fmt]
        raw_path   = raw_outputs[fmt]
        mzxml_n    = counter(mzxml_path)
        raw_n      = counter(raw_path)

        print(f"\nFormat: {fmt.upper()}")
        print(f"  {mzxml_path.name}: exists={mzxml_path.exists()} records={mzxml_n}")
        print(f"  {raw_path.name}: exists={raw_path.exists()} records={raw_n}")

        if not mzxml_path.exists() or mzxml_path.stat().st_size == 0:
            failures.append(f"{fmt}: .mzXML output missing or blank")
        if not raw_path.exists() or raw_path.stat().st_size == 0:
            failures.append(f"{fmt}: .raw output missing or blank")
        if mzxml_n == 0 or raw_n == 0:
            failures.append(f"{fmt}: zero records parsed (blank or malformed output?)")
        if not close_enough(mzxml_n, raw_n):
            failures.append(f"{fmt}: record counts not near-exact "
                            f"(.mzXML={mzxml_n}, .raw={raw_n})")

    print(f"\nResults written to {run_root}/")
    print()

    if failures:
        print(f"FAILED ({len(failures)} check(s)):")
        for f in failures:
            print(f"  - {f}")
        sys.exit(1)

    print("PASSED: .raw file support confirmed; all 5 output formats valid and "
          "near-exact vs .mzXML.")
    sys.exit(0)


if __name__ == "__main__":
    main()
