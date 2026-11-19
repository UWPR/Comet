#!/usr/bin/env python3
"""
Generate the T68 fragment-index histogram-fill fixtures
(docs/20261106_FI_evalue_histogram_fill.md section 5.1).

Writes, into the directory given as the first argument (default: this script's):
  t68_histfill.fasta    the target peptide, N2 "2-hit" and N1 "1-hit" fillers (composition-identical
                        permutations of the target, so every one of them is inside the precursor mass
                        window), and 0-hit random fillers, each as a tryptic peptide in its own protein
  t68_histfill.txt      an rts_repro SPECTRUM fixture: one scan, the target's 1+ b/y ions, no noise
  t68_histfill.json     the counts the test asserts against

"Hits" is the fragment-index candidate count: the number of a peptide's 1+ b/y ions, with
m/z in [fragindex_min_fragmentmass, fragindex_max_fragmentmass] = [200, 2000], whose 0.02 Da
bin (offset 0) holds a target ion. The target (LVNELTEFAK, 2+) clears fragindex_min_ions_score
(3) with 16 hits. For a composition-identical peptide b(n-k) = M - y(k), so a shared prefix
or suffix residue multiset shares two ions, except at the termini where b1 and y1 fall below
200: every permutation ending in K shares b9 (M - K) -- 1 hit -- and one that also starts
with L shares y9 -- 2 hits -- as long as no other prefix/suffix multiset coincides (a shared
"AK" suffix would add y2 and b8). The model counts all of that and only permutations with
exactly the intended count are kept. With fragindex_histogram_fill = 0 the E-value
histogram holds the target alone; with fill >= N2 + N1 + 1 it holds them all.
Deterministic: random.seed(68).
"""

import json
import os
import random
import sys

# The shared masses and b/y helpers live in tests/unit/fixture_peptides.py; "python -I" leaves
# this script's directory off sys.path, so tests/unit/ is added here.
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from fixture_peptides import PROTON, by_ions_1plus, neutral_mass  # noqa: E402

POOL = "GASPVTLNDQEHFY"
BIN_WIDTH = 0.02
FRAG_MIN, FRAG_MAX = 200.0, 2000.0
TARGET = "LVNELTEFAK"
N2, N1, N0 = 20, 30, 150


def bin_of(mz):
    return int(mz / BIN_WIDTH + 1.0)   # fragment_bin_offset 0: BIN = mass * inverse_width + (1 - offset)


def index_hits(pep, target_bins):
    return sum(1 for mz in by_ions_1plus(pep) if FRAG_MIN <= mz <= FRAG_MAX and bin_of(mz) in target_bins)


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    rng = random.Random(68)
    target_bins = set(bin_of(mz) for mz in by_ions_1plus(TARGET) if FRAG_MIN <= mz <= FRAG_MAX)
    target_hits = index_hits(TARGET, target_bins)

    # random permutations of the first nine residues, C-terminal K kept (same mass as the target),
    # bucketed by their exact index-hit count: 1 (b9 only) or 2 (b9 + y9 from a leading L)
    head = list(TARGET[:9])
    two, one = [], []
    seen = {TARGET}
    for _ in range(200000):
        rng.shuffle(head)
        pep = "".join(head) + "K"
        if pep in seen:
            continue
        seen.add(pep)
        h = index_hits(pep, target_bins)
        if h == 2 and len(two) < N2:
            two.append(pep)
        elif h == 1 and len(one) < N1:
            one.append(pep)
        if len(two) >= N2 and len(one) >= N1:
            break
    assert len(two) == N2 and len(one) == N1, (len(two), len(one))

    zero = []
    while len(zero) < N0:
        pep = "".join(rng.choice(POOL) for _ in range(rng.randint(8, 14))) + rng.choice("KR")
        if pep not in seen and index_hits(pep, target_bins) == 0:
            zero.append(pep); seen.add(pep)

    with open(os.path.join(outdir, "t68_histfill.fasta"), "w", newline="\r\n") as f:
        f.write(">target %s\n%s\n" % (TARGET, "MAR" + TARGET + "AGAR"))
        for i, pep in enumerate(two):
            f.write(">hit2_%02d %s\n%s\n" % (i, pep, "MAR" + pep + "AGAR"))
        for i, pep in enumerate(one):
            f.write(">hit1_%02d %s\n%s\n" % (i, pep, "MAR" + pep + "AGAR"))
        for i, pep in enumerate(zero):
            f.write(">hit0_%03d\n%s\n" % (i, "MAR" + pep + "AGAR"))

    peaks = sorted((mz, 100.0 if k % 2 else 60.0) for k, mz in enumerate(by_ions_1plus(TARGET)))
    mz2 = (neutral_mass(TARGET) + 2 * PROTON) / 2
    with open(os.path.join(outdir, "t68_histfill.txt"), "w", newline="\r\n") as f:
        f.write("SPECTRUM 1 2 %.6f %d\n" % (mz2, len(peaks)))
        for m, i in peaks:
            f.write("%.5f %.3f\n" % (m, i))

    info = {"target": TARGET, "target_hits": target_hits, "mz2": mz2, "n2": N2, "n1": N1, "n0": N0,
            "hit2": two, "hit1": one}
    with open(os.path.join(outdir, "t68_histfill.json"), "w", newline="\r\n") as f:
        json.dump(info, f, indent=1)
    print(json.dumps({k: v for k, v in info.items() if k not in ("hit2", "hit1")}, indent=1))


if __name__ == "__main__":
    main()
