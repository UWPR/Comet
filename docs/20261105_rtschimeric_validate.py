#!/usr/bin/env python3
"""
Removal-validation metrics for the RTS chimeric search (docs/20261105_RTSchimeric.md 7.3).

Inputs are rts_repro outputs:
  --baseline FILE   mode-0 output (rank-1 per spectrum)        MS2 <scan>\t<pep>\t<xcorr>\t<expect>\tz <z>\tprot '<p>'
  NAME=FILE ...     chimeric outputs (mode 1/2), one per setting:
                    MS2 <scan>\titer <k>\tz <z>\t<pep>\txcorr <x>\tdcn <d>\texpect <e>\tmass <m>\tions <a>/<b>\tprot '<p>'
  --trace NAME=FILE stderr of a single-threaded RTS_CHIMERIC_DEBUG=2 run of that setting (ghost analysis)
"""
import argparse
import re
import sys
from collections import defaultdict

DECOY = ("DECOY_", "rev_")


def is_decoy(prot):
    return prot.upper().startswith(tuple(p.upper() for p in DECOY))


def read_baseline(path):
    rows = {}
    for line in open(path):
        f = line.rstrip("\n").split("\t")
        scan = int(f[0].split()[1])
        if len(f) < 3:
            rows[scan] = None
            continue
        rows[scan] = {"pep": f[1], "xcorr": float(f[2]), "expect": float(f[3]), "z": int(f[4].split()[1]),
                      "prot": f[5][6:-1]}
    return rows


def read_chimeric(path):
    rows = defaultdict(list)
    for line in open(path):
        f = line.rstrip("\n").split("\t")
        scan = int(f[0].split()[1])
        rows.setdefault(scan, [])
        if len(f) < 4:
            continue
        d = {"iter": int(f[1].split()[1]), "z": int(f[2].split()[1]), "pep": f[3]}
        for tok in f[4:]:
            k, _, v = tok.partition(" ")
            d[k] = v
        d["xcorr"] = float(d["xcorr"])
        d["expect"] = float(d["expect"])
        d["prot"] = d["prot"][1:-1]
        d["decoy"] = is_decoy(d["prot"])
        rows[scan].append(d)
    return rows


def q_count(psms, thresholds=(0.01,)):
    """PSMs at q <= t, sorting by expect ascending (ties xcorr desc); FDR = decoys / targets."""
    out = {}
    s = sorted(psms, key=lambda r: (r["expect"], -r["xcorr"]))
    nt = nd = 0
    fdr = []
    for r in s:
        if r["decoy"]:
            nd += 1
        else:
            nt += 1
        fdr.append(nd / nt if nt else 1.0)
    q = [0.0] * len(fdr)
    m = 1.0
    for i in range(len(fdr) - 1, -1, -1):
        m = min(m, fdr[i])
        q[i] = m
    for t in thresholds:
        out[t] = sum(1 for i, r in enumerate(s) if q[i] <= t and not r["decoy"])
    return out


def ghost_stats(trace_path, window=2):
    """Per call: DETAIL lines in iteration order; for iteration k >= 1 count matched bins within
    +/-window of any bin removed in iterations < k."""
    calls = []
    cur = None
    for line in open(trace_path):
        if line.startswith("RTS_CHIMERIC iter=0 candidates="):   # one per call; "iter=0 removed ..." lines are not
            cur = []
            calls.append(cur)
        elif line.startswith("RTS_CHIMERIC_DETAIL") and cur is not None:
            m = re.match(r"RTS_CHIMERIC_DETAIL iter=(\d+) z=(\d+) peptide=(\S+) matched=([\d,]*) removed=([\d,]*)", line)
            if m:
                matched = [int(x) for x in m.group(4).split(",") if x]
                removed = set(int(x) for x in m.group(5).split(",") if x)
                cur.append((int(m.group(1)), m.group(3), matched, removed))
    n_later = ghost_bins = total_bins = heavy = 0
    dist_hist = defaultdict(int)   # min |matched bin - earlier removed bin| per later matched ion (capped at 10)
    for call in calls:
        removed_so_far = set()
        for it, pep, matched, removed in call:
            if it >= 1 and matched and removed_so_far:
                n_later += 1
                g = 0
                for b in matched:
                    dmin = min((abs(b - r) for r in removed_so_far), default=99)
                    dist_hist[min(dmin, 10)] += 1
                    if dmin <= window:
                        g += 1
                ghost_bins += g
                total_bins += len(matched)
                if g * 2 >= len(matched):
                    heavy += 1
            removed_so_far |= removed
    return {"calls": len(calls), "later_psms": n_later, "ghost_frac": (ghost_bins / total_bins) if total_bins else 0.0,
            "heavy_frac": (heavy / n_later) if n_later else 0.0, "dist_hist": dict(dist_hist), "total_bins": total_bins}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--baseline")
    ap.add_argument("--trace", action="append", default=[])
    ap.add_argument("settings", nargs="+", help="NAME=FILE")
    a = ap.parse_args()
    base = read_baseline(a.baseline) if a.baseline else None
    traces = dict(t.split("=", 1) for t in a.trace)

    if base:
        b_ids = [{"expect": v["expect"], "xcorr": v["xcorr"], "decoy": is_decoy(v["prot"])} for v in base.values() if v]
        print("baseline (mode 0): %d spectra, %d with a rank-1 PSM, %d targets at q<=0.01" % (
            len(base), len(b_ids), q_count(b_ids)[0.01]))
        print()

    hdr = ["setting", "spectra", "0/1/2/3+", "rows", "iter>=1", "decoy%% it0", "decoy%% it>=1", "q01 it0", "q01 it>=1",
           "q01 all", "repeat", "it0=base", "z=file"]
    print("\t".join(hdr).replace("%%", "%"))
    for s in a.settings:
        name, path = s.split("=", 1)
        rows = read_chimeric(path)
        hist = [0, 0, 0, 0]
        it0, later, allr = [], [], []
        repeats = agree = zfile = withbase = 0
        for scan, rs in rows.items():
            hist[min(len(rs), 3)] += 1
            seen = set()
            for r in rs:
                allr.append(r)
                (it0 if r["iter"] == 0 else later).append(r)
                if r["pep"] in seen:
                    repeats += 1
                seen.add(r["pep"])
            if base and scan in base and base[scan] and rs:
                withbase += 1
                if rs[0]["pep"] == base[scan]["pep"]:
                    agree += 1
                if rs[0]["z"] == base[scan]["z"]:
                    zfile += 1
        d0 = 100.0 * sum(r["decoy"] for r in it0) / len(it0) if it0 else 0.0
        d1 = 100.0 * sum(r["decoy"] for r in later) / len(later) if later else 0.0
        print("\t".join(str(x) for x in [
            name, len(rows), "/".join(map(str, hist)), len(allr), len(later), "%.1f" % d0, "%.1f" % d1,
            q_count(it0)[0.01] if it0 else 0, q_count(later)[0.01] if later else 0, q_count(allr)[0.01] if allr else 0,
            repeats, ("%.1f%%" % (100.0 * agree / withbase)) if withbase else "-",
            ("%.1f%%" % (100.0 * zfile / withbase)) if withbase else "-"]))
        if name in traces:
            g = ghost_stats(traces[name])
            print("   trace %s: %d calls, %d later PSMs, %.1f%% of their matched bins within +/-2 of an earlier removed bin, %.1f%% of them with >= half ghost bins" % (
                name, g["calls"], g["later_psms"], 100.0 * g["ghost_frac"], 100.0 * g["heavy_frac"]))


if __name__ == "__main__":
    main()
