#!/usr/bin/env python3
"""
Generator for test T90 (graded-intensity chimeric spectra): writes t90_graded.txt, an rts_repro
fixture, next to the T67 fixtures it builds on (t67_chimeric.json and t67_chimeric.fasta must
already be in the output directory: run t67_make_chimeric.py first).

T67's fixtures give every fragment of a peptide one of two intensities (b 60, y 100) and put the
second peptide at 1/3 of the first. Real fragment ions span a couple of orders of magnitude and a
co-isolated peptide can be far weaker than the main one. Here:

  fragment intensities  log-uniform over a 30-fold range below the peptide's base peak
                        (b ions on average lower than y), the peptide's base peak set by its scale
  detection limit       fragment peaks below 5 (0.5% of the strong base peak) are dropped, as an
                        instrument's noise cut-off would: a weak peptide loses its weakest ions first
  noise                 300 peaks (a typical centroided Orbitrap MS/MS), m/z uniform 150-1500,
                        intensity log-uniform 1-30, i.e. 0.1-3% of the strong peptide's base peak
                        (1000), the same in every scan
  strong peptide        P1 = LVNELTEFAK at 2+, base peak 1000
  weak peptide          P2 (3+, a different mass whose m/z lies within 1 of P1's) or P3 (2+, the
                        same mass as P1, mostly different ions), base peak 1000 / ratio

Scans (precursor m/z = P1's 2+ m/z in every one):
  scan 1                              P1 alone
  scans 10 + k, k = 0..4              P1 + P2 at ratios 1, 3, 10, 30, 100 : 1
  scans 20 + k                        P1 + P3 at the same ratios
  scans 30 + k                        P2 alone at base peak 1000 / ratio (does the weak peptide
                                      show up without the strong one in the way?)
  scans 40 + k                        P3 alone at 1000 / ratio
  scan 50                             noise only

Deterministic: random.seed(90).
"""

import json
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from fixture_peptides import by_ions, mz  # noqa: E402

RATIOS = (1, 3, 10, 30, 100)
BASE_PEAK = 1000.0
SPAN = 30.0          # fragment intensities run from base peak down to base peak / SPAN
NOISE_N = 300
NOISE_LO, NOISE_HI = 1.0, 30.0
DETECTION_LIMIT = 5.0   # a fragment peak weaker than this is not recorded (0.5% of the strong base peak)


def log_uniform(rng, lo, hi):
    return math.exp(rng.uniform(math.log(lo), math.log(hi)))


def graded_ions(rng, pep, precursor_z, base_peak):
    """1+..(z-1)+ b/y ions with log-uniform intensities in [base_peak/SPAN, base_peak]; b ions at
    0.6x the y-ion draw, so b is weaker on average; ions under DETECTION_LIMIT are left out."""
    maxz = 1 if precursor_z == 1 else min(precursor_z - 1, 3)
    out = []
    for m, s in by_ions(pep, maxz):
        inten = log_uniform(rng, base_peak / SPAN, base_peak) * (1.0 if s == "y" else 0.6)
        if inten >= DETECTION_LIMIT:     # below the instrument's detection limit the peak is not in the spectrum
            out.append((m, inten))
    return out


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    info = json.load(open(os.path.join(outdir, "t67_chimeric.json")))
    p1, p2, p3 = info["P1"]["seq"], info["P2"]["seq"], info["P3"]["seq"]
    rng = random.Random(90)

    noise = [(rng.uniform(150.0, 1500.0), log_uniform(rng, NOISE_LO, NOISE_HI)) for _ in range(NOISE_N)]
    strong = graded_ions(rng, p1, 2, BASE_PEAK)

    scans = {1: strong + noise, 50: list(noise)}
    for k, ratio in enumerate(RATIOS):
        weak2 = graded_ions(rng, p2, 3, BASE_PEAK / ratio)
        weak3 = graded_ions(rng, p3, 2, BASE_PEAK / ratio)
        scans[10 + k] = strong + weak2 + noise
        scans[20 + k] = strong + weak3 + noise
        scans[30 + k] = weak2 + noise
        scans[40 + k] = weak3 + noise

    precursor = mz(p1, 2)
    with open(os.path.join(outdir, "t90_graded.txt"), "w", newline="\r\n") as f:
        for scan in sorted(scans):
            peaks = sorted(scans[scan])
            f.write("SPECTRUM %d 2 %.6f %d\n" % (scan, precursor, len(peaks)))
            for pm, pi in peaks:
                f.write("%.5f %.3f\n" % (pm, pi))

    meta = {"ratios": list(RATIOS), "base_peak": BASE_PEAK, "span": SPAN,
            "P1": p1, "P2": p2, "P3": p3,
            "scans": {"P1_alone": 1, "P1_P2": 10, "P1_P3": 20, "P2_alone": 30, "P3_alone": 40, "noise": 50}}
    with open(os.path.join(outdir, "t90_graded.json"), "w", newline="\r\n") as f:
        json.dump(meta, f, indent=1)
    print(json.dumps(meta))


if __name__ == "__main__":
    main()
