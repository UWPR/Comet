#!/usr/bin/env python3
"""
Compute q-values from Comet tab-delimited output files.

FDR is computed twice per file — once sorted by xcorr (descending) and once by
e-value (ascending) — so results from both scoring methods are reported together.

FDR calculation uses **rank-1 PSMs only** (the top-ranked peptide per spectrum).
Including lower ranks inflates counts and produces incorrect FDR estimates because
each spectrum contributes at most one true positive.

FDR convention: standard target-decoy approach (TDA), no +1 correction, no 2x scaling.
    FDR(i) = n_decoy(i) / n_target(i)    (at rank i in the sorted list)
    q(i)   = min FDR over all j >= i     (monotone running minimum from the tail)
Decoys are identified by the protein column starting with "DECOY_" or "rev_"
(case-insensitive).

Usage
-----
    python tools/qvalue.py results.txt
    python tools/qvalue.py results_a.txt results_b.txt
    python tools/qvalue.py --diff results_a.txt results_b.txt
    python tools/qvalue.py --threshold 0.01 --threshold 0.05 results.txt

Column indices in Comet tab-delimited output (0-based, after 2 header lines):
    0  scan
    1  num       (hit rank; 1 = top hit)
    2  charge
    5  e-value
    6  xcorr
    12 modified_peptide
    15 protein
"""

import argparse
import math
from pathlib import Path


DECOY_PREFIXES = ("decoy_", "rev_")

# 0-based column indices in Comet txt body rows (after 2 header lines)
COL_SCAN   = 0
COL_NUM    = 1
COL_CHARGE = 2
COL_EVALUE = 5
COL_XCORR  = 6
COL_MODPEP = 12
COL_PROT   = 15

# Minimum column count needed to parse a row
_REQUIRED_COL = max(COL_SCAN, COL_NUM, COL_CHARGE, COL_EVALUE, COL_XCORR,
                    COL_MODPEP, COL_PROT)

# Field indices within each PSM tuple: (xcorr, evalue, is_decoy, scan, charge, pep, prot)
_F_XCORR  = 0
_F_EVALUE = 1
_F_DECOY  = 2
_F_SCAN   = 3
_F_CHARGE = 4
_F_PEP    = 5
_F_PROT   = 6


def is_decoy(protein: str) -> bool:
    return any(protein.lower().startswith(p) for p in DECOY_PREFIXES)


def load_rank1(path: str) -> list[tuple[float, float, bool, int, int, str, str]]:
    """
    Return a list of (xcorr, evalue, is_decoy, scan, charge, modified_peptide, protein)
    for rank-1 PSMs from a Comet txt file.  List is unsorted.
    """
    psms = []
    with open(path) as fh:
        for lineno, line in enumerate(fh):
            if lineno < 2:          # skip two header lines
                continue
            cols = line.rstrip("\n").split("\t")
            if len(cols) <= _REQUIRED_COL:
                continue
            try:
                num    = int(cols[COL_NUM])
                xcorr  = float(cols[COL_XCORR])
                evalue = float(cols[COL_EVALUE])
                scan   = int(cols[COL_SCAN])
                charge = int(cols[COL_CHARGE])
                pep    = cols[COL_MODPEP]
                prot   = cols[COL_PROT]
            except ValueError:
                continue
            if num != 1:
                continue
            if not math.isfinite(xcorr) or not math.isfinite(evalue):
                continue
            psms.append((xcorr, evalue, is_decoy(prot), scan, charge, pep, prot))
    return psms


def _sort_psms(psms: list[tuple], by: str) -> list[tuple]:
    """Return a copy of psms sorted best-first by 'xcorr' or 'evalue'."""
    if by == "xcorr":
        return sorted(psms, key=lambda r: r[_F_XCORR], reverse=True)
    return sorted(psms, key=lambda r: r[_F_EVALUE], reverse=False)


def compute_qvalues(sorted_psms: list[tuple]) -> list[float]:
    """
    Compute q-values for a list already sorted best-first.
    Returns a parallel list of q-values (one per PSM, including decoys).
    """
    if not sorted_psms:
        return []

    n = len(sorted_psms)
    fdrs = [0.0] * n
    n_target = n_decoy = 0
    for i, psm in enumerate(sorted_psms):
        if psm[_F_DECOY]:
            n_decoy += 1
        else:
            n_target += 1
        fdrs[i] = n_decoy / n_target if n_target > 0 else 1.0

    qvals = fdrs[:]
    running_min = qvals[-1]
    for i in range(n - 2, -1, -1):
        if qvals[i] > running_min:
            qvals[i] = running_min
        else:
            running_min = qvals[i]
    return qvals


def _count_passing(sorted_psms: list[tuple], qvals: list[float],
                   thresh: float, score_field: int) -> tuple[int, float | None]:
    """Count passing target PSMs at a q-value threshold and return the score cutoff."""
    count = 0
    cutoff = None
    for psm, qv in zip(sorted_psms, qvals):
        if not psm[_F_DECOY] and qv <= thresh:
            count += 1
            cutoff = psm[score_field]
    return count, cutoff


def summarise(path: str,
              psms_x: list[tuple], qvals_x: list[float],
              psms_e: list[tuple], qvals_e: list[float],
              thresholds: list[float]) -> dict:
    """
    Print a side-by-side xcorr / evalue result table.
    Returns {thresh: {'xcorr': (count, cutoff), 'evalue': (count, cutoff)}}.
    """
    results = {}
    for thresh in thresholds:
        cx, cut_x = _count_passing(psms_x, qvals_x, thresh, _F_XCORR)
        ce, cut_e = _count_passing(psms_e, qvals_e, thresh, _F_EVALUE)
        results[thresh] = {"xcorr": (cx, cut_x), "evalue": (ce, cut_e)}

    print(f"\n{'='*76}")
    print(f"  {Path(path).name}")
    print(f"{'='*76}")
    print(f"  {'Q-value':>10}  {'xcorr PSMs':>12}  {'xcorr cut':>11}"
          f"  {'evalue PSMs':>12}  {'evalue cut':>12}")
    print(f"  {'-'*10}  {'-'*12}  {'-'*11}  {'-'*12}  {'-'*12}")
    for thresh in thresholds:
        cx, cut_x = results[thresh]["xcorr"]
        ce, cut_e = results[thresh]["evalue"]
        sx = f"{cut_x:.4f}" if cut_x is not None else "n/a"
        se = f"{cut_e:.2e}" if cut_e is not None else "n/a"
        print(f"  {thresh*100:>9.1f}%  {cx:>12,}  {sx:>11}  {ce:>12,}  {se:>12}")

    return results


def _passing_set(sorted_psms: list[tuple], qvals: list[float],
                 thresh: float) -> dict[tuple[int, int], tuple]:
    """Return {(scan, charge): psm} for target PSMs passing the q-value threshold."""
    return {(psm[_F_SCAN], psm[_F_CHARGE]): psm
            for psm, qv in zip(sorted_psms, qvals)
            if not psm[_F_DECOY] and qv <= thresh}


def _diff_for_method(by: str, score_field: int, score_fmt,
                     label_a: str, sorted_a: list[tuple], qvals_a: list[float],
                     label_b: str, sorted_b: list[tuple], qvals_b: list[float],
                     thresh: float):
    pa = _passing_set(sorted_a, qvals_a, thresh)
    pb = _passing_set(sorted_b, qvals_b, thresh)
    only_a = {k: v for k, v in pa.items() if k not in pb}
    only_b = {k: v for k, v in pb.items() if k not in pa}

    label = f"{thresh*100:.0f}%"
    print(f"\n{'='*60}")
    print(f"  Diff at {label} q-value  ({by} sort)")
    print(f"{'='*60}")

    for tag, entries in ((label_a, only_a), (label_b, only_b)):
        if entries:
            print(f"\n  Entries in [{tag}] only  (n={len(entries)}):")
            for (scan, charge) in sorted(entries):
                psm = entries[(scan, charge)]
                print(f"    scan {scan:6d}  z={charge}"
                      f"  {by}={score_fmt(psm[score_field])}  {psm[_F_PEP][:55]}")
        else:
            print(f"\n  No entries exclusively in [{tag}]")


def diff_passing(label_a: str,
                 psms_x_a: list[tuple], qvals_x_a: list[float],
                 psms_e_a: list[tuple], qvals_e_a: list[float],
                 label_b: str,
                 psms_x_b: list[tuple], qvals_x_b: list[float],
                 psms_e_b: list[tuple], qvals_e_b: list[float],
                 thresh: float):
    """Print per-method diff (xcorr then evalue) at the given threshold."""
    _diff_for_method("xcorr",  _F_XCORR,  lambda s: f"{s:.4f}",
                     label_a, psms_x_a, qvals_x_a,
                     label_b, psms_x_b, qvals_x_b, thresh)
    _diff_for_method("evalue", _F_EVALUE, lambda s: f"{s:.2e}",
                     label_a, psms_e_a, qvals_e_a,
                     label_b, psms_e_b, qvals_e_b, thresh)


def main():
    parser = argparse.ArgumentParser(
        description="Compute q-values from Comet txt output (rank-1 PSMs only)."
                    " Both xcorr and e-value sorting are reported.")
    parser.add_argument("files", nargs="+", metavar="FILE",
                        help="One or more Comet tab-delimited output files.")
    parser.add_argument("--threshold", type=float, action="append",
                        dest="thresholds", metavar="Q",
                        help="Q-value threshold(s) to report (default: 0.01 and 0.05).")
    parser.add_argument("--diff", action="store_true",
                        help="When two files are given, print PSMs unique to each file.")
    args = parser.parse_args()

    thresholds = sorted(args.thresholds) if args.thresholds else [0.01, 0.05]

    all_data = []   # (path, psms_x, qvals_x, psms_e, qvals_e, res)

    for path in args.files:
        psms   = load_rank1(path)
        psms_x = _sort_psms(psms, "xcorr")
        psms_e = _sort_psms(psms, "evalue")
        qvals_x = compute_qvalues(psms_x)
        qvals_e = compute_qvalues(psms_e)
        res = summarise(path, psms_x, qvals_x, psms_e, qvals_e, thresholds)
        all_data.append((path, psms_x, qvals_x, psms_e, qvals_e, res))

    # Side-by-side summary table when multiple files given
    if len(args.files) > 1:
        col_headers = [f"xcorr {t*100:.0f}%" for t in thresholds] \
                    + [f"eval  {t*100:.0f}%" for t in thresholds]
        print(f"\n{'='*90}")
        print("  Summary table")
        print(f"{'='*90}")
        print(f"  {'File':<35}" + "".join(f"  {h:>10}" for h in col_headers))
        print(f"  {'-'*35}" + "".join(f"  {'-'*10}" for _ in col_headers))
        for path, _px, _qx, _pe, _qe, res in all_data:
            name = Path(path).name[:35]
            xcorr_vals = "".join(f"  {res[t]['xcorr'][0]:>10,}" for t in thresholds)
            eval_vals  = "".join(f"  {res[t]['evalue'][0]:>10,}" for t in thresholds)
            print(f"  {name:<35}{xcorr_vals}{eval_vals}")

    # Diff output for exactly two files
    if args.diff and len(args.files) == 2:
        (pa, px_a, qx_a, pe_a, qe_a, _), (pb, px_b, qx_b, pe_b, qe_b, _) = all_data
        for thresh in thresholds:
            diff_passing(Path(pa).name, px_a, qx_a, pe_a, qe_a,
                         Path(pb).name, px_b, qx_b, pe_b, qe_b,
                         thresh)

    print()


if __name__ == "__main__":
    main()
