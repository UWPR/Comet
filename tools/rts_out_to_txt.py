#!/usr/bin/env python3
"""Convert a RealtimeSearch.exe rts.out file into a Comet-txt-compatible tab-delimited
file so tools/qvalue.py can consume it directly. Only MS2 PSM lines are converted
(MS1-only/histogram/summary lines are skipped). One row per scan (RTS only ever reports
rank-1, matching qvalue.py's rank-1-only requirement).

rts.out MS2 line format (RealtimeSearch/SearchMS1MS2.cs):
 MS2 {scan}\t{peptide}  {xcorr:F4}  {evalue:0.##E+00}  z {charge}  exp {expMass:F4}  calc {calcMass:F4}  AScore {ascore:F2}  Sites '{sites}'  {ms} ms  prot '{protein}'
"""
import re
import sys

LINE_RE = re.compile(
    r"^ MS2 (?P<scan>\d+)\t(?P<peptide>\S+)\s+"
    r"(?P<xcorr>[\d.eE+-]+)\s+(?P<evalue>[\d.eE+-]+)\s+z (?P<charge>\d+)\s+"
    r"exp (?P<exp>[\d.eE+-]+)\s+calc (?P<calc>[\d.eE+-]+)\s+"
    r"AScore (?P<ascore>[\d.eE+-]+)\s+Sites '(?P<sites>[^']*)'\s+"
    r"(?P<ms>\d+) ms\s+prot '(?P<prot>[^']*)'"
)

COLS = ("scan", "num", "charge", "exp_neutral_mass", "calc_neutral_mass", "e-value",
        "xcorr", "delta_cn", "sp_score", "ions_matched", "ions_total", "plain_peptide",
        "modified_peptide", "prev_aa", "next_aa", "protein", "protein_count",
        "modifications", "retention_time_sec", "sp_rank")


def convert(in_path, out_path, tag=""):
    n_ms2 = 0
    with open(in_path, "r", errors="replace") as fin, open(out_path, "w", newline="") as fout:
        fout.write(f"CometVersion (converted from rts.out){tag}\t\t\n")
        fout.write("\t".join(COLS) + "\n")
        for line in fin:
            m = LINE_RE.match(line.rstrip("\n"))
            if not m:
                continue
            n_ms2 += 1
            row = {
                "scan": m["scan"], "num": "1", "charge": m["charge"],
                "exp_neutral_mass": m["exp"], "calc_neutral_mass": m["calc"],
                "e-value": m["evalue"], "xcorr": m["xcorr"], "delta_cn": "0",
                "sp_score": "0", "ions_matched": "0", "ions_total": "0",
                "plain_peptide": "", "modified_peptide": m["peptide"],
                "prev_aa": "", "next_aa": "", "protein": m["prot"],
                "protein_count": "1", "modifications": m["sites"],
                "retention_time_sec": "0", "sp_rank": "1",
            }
            fout.write("\t".join(row[c] for c in COLS) + "\n")
    return n_ms2


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} rts.out out.txt", file=sys.stderr)
        sys.exit(1)
    n = convert(sys.argv[1], sys.argv[2])
    print(f"wrote {n} MS2 PSM rows to {sys.argv[2]}")
