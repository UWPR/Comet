#!/usr/bin/env python3
"""
Comet regression tests — compare current build against baseline release binaries.

Runs three search modes on real MS data and compares results + performance:
  fasta  — regular FASTA database search
  fi     — fragment ion index search  (index built fresh by each binary)
  pi     — peptide index search       (index built fresh by each binary)

Comparison metrics per mode:
  - Wall-clock search time (seconds); index build time reported separately
  - PSM count (number of lines in .txt output above xcorr threshold)
  - PSM overlap: fraction of scans where both binaries agree on the top peptide

Usage:
    # 1. Fetch baseline binary first:
    python setup_baselines.py

    # 2. Run all three modes against default baseline tag(s):
    python run_regression.py

    # 3. Restrict modes or tags:
    python run_regression.py --modes fasta fi
    python run_regression.py --tags v2026.01.1

    # 4. Point at non-default binaries or data:
    python run_regression.py --current  ../../x64/Release/Comet.exe
    python run_regression.py --data     ../../data

Output:
    results/<timestamp>_<tag>/report.txt   human-readable summary
    results/<timestamp>_<tag>/report.json  machine-readable metrics
"""

import argparse
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
REGRESSION_DIR = Path(__file__).parent.resolve()
BASELINES_DIR  = REGRESSION_DIR / "baselines"
RESULTS_DIR    = REGRESSION_DIR / "results"
REPO_ROOT      = REGRESSION_DIR.parent.parent
DATA_DIR       = REPO_ROOT / "data"

IS_WINDOWS = (platform.system() == "Windows") or (os.name == "nt")
CURRENT_EXE = (REPO_ROOT / "x64" / "Release" / "Comet.exe") if IS_WINDOWS \
              else (REPO_ROOT / "comet.exe")

# ---------------------------------------------------------------------------
# Test configuration
# ---------------------------------------------------------------------------
FASTA_FILE   = DATA_DIR / "human.small.fasta"
MZXML_FILE   = DATA_DIR / "20250520_Hela_60min_06.mzXML"
PARAMS_FILE  = DATA_DIR / "comet_phospho.params"

DEFAULT_TAGS = ["v2026.01.1"]
MODES        = ["fasta", "fi", "pi"]

XCORR_THRESHOLD = 2.5   # minimum xcorr to count a PSM


# ---------------------------------------------------------------------------
# Params handling
# ---------------------------------------------------------------------------

def load_params(path: Path) -> list[str]:
    return path.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)


def patch_params(lines: list[str], overrides: dict[str, str]) -> list[str]:
    """Replace or append 'key = value' lines in a params line list."""
    result   = []
    replaced = set()
    for line in lines:
        matched = False
        for key, val in overrides.items():
            if re.match(rf"^\s*{re.escape(key)}\s*=", line):
                result.append(f"{key} = {val}\n")
                replaced.add(key)
                matched = True
                break
        if not matched:
            result.append(line)
    for key, val in overrides.items():
        if key not in replaced:
            result.append(f"{key} = {val}\n")
    return result


def write_params(lines: list[str], dest: Path) -> None:
    dest.write_text("".join(lines), encoding="utf-8")


# ---------------------------------------------------------------------------
# Path conversion (WSL <-> Windows)
# ---------------------------------------------------------------------------

def to_win(p: Path) -> str:
    s = str(p)
    if s.startswith("/mnt/"):
        parts = s[5:].split("/", 1)
        drive = parts[0].upper() + ":"
        rest  = parts[1].replace("/", "\\") if len(parts) > 1 else ""
        return drive + "\\" + rest
    return s


def comet_path(p: Path) -> str:
    """Return the path string appropriate for the running binary."""
    return to_win(p) if IS_WINDOWS else str(p)


# ---------------------------------------------------------------------------
# Running Comet
# ---------------------------------------------------------------------------

def run_comet(binary: Path, extra_args: list[str], work_dir: Path) -> tuple[float, str]:
    """Run Comet; return (wall_seconds, combined stdout+stderr)."""
    cmd    = [str(binary)] + extra_args
    t0     = time.perf_counter()
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=str(work_dir))
    elapsed = time.perf_counter() - t0
    output  = result.stdout + result.stderr
    if result.returncode != 0:
        raise RuntimeError(f"Comet exited {result.returncode}:\n{output[-3000:]}")
    return elapsed, output


# ---------------------------------------------------------------------------
# Index + search helpers
# ---------------------------------------------------------------------------

def symlink_fasta(fasta: Path, dest_dir: Path) -> Path:
    """Create a symlink to fasta inside dest_dir; return the link path."""
    link = dest_dir / fasta.name
    if link.exists() or link.is_symlink():
        link.unlink()
    link.symlink_to(fasta)
    return link


def build_index(binary: Path, params_path: Path, mode: str, work_dir: Path) -> float:
    """Create FI or PI index in work_dir; return elapsed seconds."""
    if mode == "fi":
        args = ["-i", f"-P{comet_path(params_path)}"]
    else:   # pi
        args = ["-j", f"-P{comet_path(params_path)}"]
    elapsed, _ = run_comet(binary, args, work_dir)
    return elapsed


def run_search(binary: Path, params_path: Path, mzxml: Path,
               work_dir: Path) -> tuple[float, Path]:
    """Run a search; return (elapsed_seconds, path_to_.txt_output)."""
    # Comet writes output next to the mzXML; remove stale file first.
    out_txt = mzxml.with_suffix(".txt")
    if out_txt.exists():
        out_txt.unlink()
    elapsed, _ = run_comet(
        binary,
        [f"-P{comet_path(params_path)}", comet_path(mzxml)],
        work_dir,
    )
    return elapsed, out_txt


# ---------------------------------------------------------------------------
# .txt result parsing
# ---------------------------------------------------------------------------

def parse_txt(path: Path) -> dict[str, dict]:
    """
    Parse a Comet tab-delimited .txt output file.

    File structure:
        # optional comment lines
        CometVersion X.XX   <basename>   <date>   <database>   <- metadata, no #
        scan  num  charge  ...  xcorr  ...  plain_peptide  ...  <- column header
        <data rows>

    Returns {scan_charge_key -> {"peptide": str, "xcorr": float, "evalue": float}}
    where scan_charge_key = "<scan>_<charge>".
    Only the highest-xcorr hit per (scan, charge) is kept.
    """
    results = {}
    if not path.exists():
        return results

    header = None
    with open(path, encoding="utf-8", errors="replace") as fh:
        for raw in fh:
            line = raw.rstrip("\n")

            # Skip comment lines
            if line.startswith("#"):
                continue

            fields = line.split("\t")

            # Skip the CometVersion metadata line (first field is "CometVersion ...")
            if fields[0].startswith("CometVersion"):
                continue

            # The header row starts with "scan"
            if header is None:
                if fields[0].strip().lower() == "scan":
                    header = [f.strip().lower() for f in fields]
                continue

            if len(fields) < len(header):
                continue

            row = dict(zip(header, fields))
            try:
                scan   = row.get("scan", "").strip()
                charge = row.get("charge", "").strip()
                pep    = row.get("plain_peptide", row.get("sequence", "")).strip()
                xcorr  = float(row.get("xcorr", row.get("xcorr score", 0)))
                evalue = float(row.get("e-value", 1.0))
            except (ValueError, KeyError):
                continue

            key = f"{scan}_{charge}"
            if key not in results or xcorr > results[key]["xcorr"]:
                results[key] = {"peptide": pep, "xcorr": xcorr, "evalue": evalue}

    return results


def compare_results(base: dict, curr: dict) -> dict:
    """Compare two parse_txt dicts; return a metrics dict."""
    def above(d):
        return {k: v for k, v in d.items() if v["xcorr"] >= XCORR_THRESHOLD}

    base_hi = above(base)
    curr_hi = above(curr)
    common  = set(base_hi) & set(curr_hi)
    def _norm(s): return s.replace("L", "I")
    agree   = sum(1 for k in common
                  if _norm(base_hi[k]["peptide"]) == _norm(curr_hi[k]["peptide"]))

    return {
        "base_psm_count":    len(base_hi),
        "curr_psm_count":    len(curr_hi),
        "common_scans":      len(common),
        "agree_top_peptide": agree,
        "agree_frac":        round(agree / len(common), 4) if common else None,
        "only_in_baseline":  len(set(base_hi) - set(curr_hi)),
        "only_in_current":   len(set(curr_hi) - set(base_hi)),
    }


# ---------------------------------------------------------------------------
# One mode run
# ---------------------------------------------------------------------------

def run_mode(mode: str, current_bin: Path, baseline_bin: Path,
             base_params: list[str], run_dir: Path) -> dict:
    """
    Build index (fi/pi) if needed, run both binaries, compare results.
    Returns a metrics dict for this mode.
    """
    run_dir.mkdir(parents=True, exist_ok=True)
    metrics = {"mode": mode}

    if mode == "fasta":
        # ---- FASTA search: one shared params file ----
        overrides = {
            "database_name": comet_path(FASTA_FILE),
            "output_txtfile": "1",
        }
        params_path = run_dir / "search.params"
        write_params(patch_params(base_params, overrides), params_path)

        metrics["index_build_time_baseline_s"] = None
        metrics["index_build_time_current_s"]  = None

        for label, binary in [("baseline", baseline_bin), ("current", current_bin)]:
            print(f"  [fasta] running {label} ...")
            try:
                t, txt_src = run_search(binary, params_path, MZXML_FILE, run_dir)
                metrics[f"search_time_{label}_s"] = round(t, 2)
            except RuntimeError as e:
                print(f"    ERROR: {e}", file=sys.stderr)
                metrics[f"search_time_{label}_s"] = None
                continue
            dest = run_dir / f"{label}.txt"
            if txt_src.exists():
                shutil.copy(txt_src, dest)

    else:
        # ---- FI / PI: build index per binary in its own subdirectory ----
        for label, binary in [("baseline", baseline_bin), ("current", current_bin)]:
            sub = run_dir / label
            sub.mkdir(exist_ok=True)

            # Symlink the FASTA into the subdir so the .idx lands there too
            fasta_link = symlink_fasta(FASTA_FILE, sub)
            idx_path   = sub / (FASTA_FILE.name + ".idx")

            build_overrides: dict[str, str] = {
                "database_name": comet_path(fasta_link),
                "output_txtfile": "1",
            }
            if mode == "pi":
                build_overrides["create_peptide_index"] = "1"

            build_params_path = sub / "build.params"
            write_params(patch_params(base_params, build_overrides), build_params_path)

            print(f"  [{mode}] building index ({label}) ...")
            try:
                bt = build_index(binary, build_params_path, mode, sub)
                metrics[f"index_build_time_{label}_s"] = round(bt, 2)
            except RuntimeError as e:
                print(f"    ERROR building index: {e}", file=sys.stderr)
                metrics[f"index_build_time_{label}_s"] = None
                metrics[f"search_time_{label}_s"]      = None
                continue

            if not idx_path.exists():
                print(f"    ERROR: expected index not found at {idx_path}", file=sys.stderr)
                metrics[f"search_time_{label}_s"] = None
                continue

            search_overrides = {
                "database_name": comet_path(idx_path),
                "output_txtfile": "1",
            }
            search_params_path = sub / "search.params"
            write_params(patch_params(base_params, search_overrides), search_params_path)

            print(f"  [{mode}] running {label} search ...")
            try:
                t, txt_src = run_search(binary, search_params_path, MZXML_FILE, sub)
                metrics[f"search_time_{label}_s"] = round(t, 2)
            except RuntimeError as e:
                print(f"    ERROR in search: {e}", file=sys.stderr)
                metrics[f"search_time_{label}_s"] = None
                continue

            dest = run_dir / f"{label}.txt"
            if txt_src.exists():
                shutil.copy(txt_src, dest)

    # ---- Compare ----
    print(f"  [{mode}] comparing results ...")
    base_psms = parse_txt(run_dir / "baseline.txt")
    curr_psms = parse_txt(run_dir / "current.txt")
    metrics.update(compare_results(base_psms, curr_psms))
    return metrics


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def fmt(val, width=8, decimals=None) -> str:
    """Format a metric value; return right-aligned string or 'N/A'."""
    if val is None:
        return "N/A".rjust(width)
    if decimals is not None and isinstance(val, float):
        return f"{val:.{decimals}f}".rjust(width)
    return str(val).rjust(width)


def print_report(all_metrics: list[dict], current_bin: Path, baseline_tag: str):
    sep = "-" * 72
    print(sep)
    print(f"Comet regression report")
    print(f"  baseline : {baseline_tag}")
    print(f"  current  : {current_bin}")
    print(f"  xcorr threshold for PSM count: >= {XCORR_THRESHOLD}")
    print(sep)
    for m in all_metrics:
        mode = m["mode"]
        print(f"\nMode: {mode.upper()}")
        if m.get("index_build_time_baseline_s", "absent") != "absent":
            print(f"  index build (baseline) : {fmt(m['index_build_time_baseline_s'], decimals=1)} s")
            print(f"  index build (current)  : {fmt(m.get('index_build_time_current_s'), decimals=1)} s")
        print(f"  search time (baseline) : {fmt(m.get('search_time_baseline_s'), decimals=1)} s")
        print(f"  search time (current)  : {fmt(m.get('search_time_current_s'),  decimals=1)} s")
        print(f"  PSMs >= {XCORR_THRESHOLD} (baseline) : {fmt(m.get('base_psm_count'))}")
        print(f"  PSMs >= {XCORR_THRESHOLD} (current)  : {fmt(m.get('curr_psm_count'))}")
        af = m.get("agree_frac")
        if af is not None:
            pct = af * 100
            print(f"  top-peptide agreement  : {m['agree_top_peptide']:>8} / "
                  f"{m['common_scans']} common scans ({pct:.2f}%)")
            print(f"  only in baseline       : {fmt(m.get('only_in_baseline'))}")
            print(f"  only in current        : {fmt(m.get('only_in_current'))}")
        else:
            print(f"  top-peptide agreement  : {'N/A':>8}")
    print(sep)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--current",  type=Path, default=CURRENT_EXE,
                        help=f"current Comet binary (default: {CURRENT_EXE})")
    parser.add_argument("--tags",     nargs="+", default=DEFAULT_TAGS,
                        help=f"baseline release tags (default: {DEFAULT_TAGS})")
    parser.add_argument("--modes",    nargs="+", default=MODES, choices=MODES,
                        help=f"search modes to run (default: all)")
    parser.add_argument("--data",     type=Path, default=DATA_DIR,
                        help=f"directory with FASTA, mzXML, and params (default: {DATA_DIR})")
    args = parser.parse_args()

    # Allow --data to override global file paths
    global FASTA_FILE, MZXML_FILE, PARAMS_FILE
    if args.data != DATA_DIR:
        FASTA_FILE  = args.data / FASTA_FILE.name
        MZXML_FILE  = args.data / MZXML_FILE.name
        PARAMS_FILE = args.data / PARAMS_FILE.name

    for req, label in [(FASTA_FILE, "FASTA"), (MZXML_FILE, "mzXML"), (PARAMS_FILE, "params")]:
        if not req.exists():
            print(f"ERROR: {label} file not found: {req}", file=sys.stderr)
            sys.exit(1)

    if not args.current.exists():
        print(f"ERROR: current binary not found: {args.current}", file=sys.stderr)
        sys.exit(1)

    base_params = load_params(PARAMS_FILE)
    timestamp   = datetime.now().strftime("%Y%m%d_%H%M%S")
    had_error   = False

    for tag in args.tags:
        baseline_bin = BASELINES_DIR / tag / ("Comet.exe" if IS_WINDOWS else "comet")
        if not baseline_bin.exists():
            print(f"ERROR: baseline binary missing for {tag}. "
                  f"Run setup_baselines.py first.", file=sys.stderr)
            sys.exit(1)

        print(f"\n{'='*72}")
        print(f"Baseline: {tag}  vs  current: {args.current.name}")
        print(f"{'='*72}")

        run_root    = RESULTS_DIR / f"{timestamp}_{tag}"
        tag_metrics = []

        for mode in args.modes:
            try:
                m = run_mode(mode, args.current, baseline_bin,
                             base_params, run_root / mode)
            except Exception as e:
                print(f"  [{mode}] FAILED: {e}", file=sys.stderr)
                m = {"mode": mode, "error": str(e)}
                had_error = True
            tag_metrics.append(m)

        print_report(tag_metrics, args.current, tag)

        report_data = {
            "timestamp":       timestamp,
            "baseline_tag":    tag,
            "current_bin":     str(args.current),
            "xcorr_threshold": XCORR_THRESHOLD,
            "modes":           tag_metrics,
        }

        run_root.mkdir(parents=True, exist_ok=True)
        (run_root / "report.json").write_text(json.dumps(report_data, indent=2))

        import io, contextlib
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            print_report(tag_metrics, args.current, tag)
        (run_root / "report.txt").write_text(buf.getvalue())

        print(f"\nReports written to {run_root}/")

    sys.exit(1 if had_error else 0)


if __name__ == "__main__":
    main()
