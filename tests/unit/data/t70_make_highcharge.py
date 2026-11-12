#!/usr/bin/env python3
"""
Generate the T70 fixtures: a 3+ spectrum made only of 2+ fragment ions, for the fragment-index
charge conversion in SearchFragmentIndex().

The fragment index holds 1+ b/y masses (one proton). A peak observed at charge z is looked up
as the 1+ mass mz * z - (z - 1) * proton. Until 2026-11-08 the lookup subtracted (z - 1) * 1.0,
putting every 2+ probe 0.0073 Da high -- about a third of a 0.02 Da bin -- so roughly a third
of 2+ fragments were looked up in the wrong bin.

Writes, into the directory given as the first argument (default: this script's):
  t70_highcharge.fasta  the target peptide in its own protein, plus random tryptic fillers
  t70_highcharge.ms2    one 3+ scan holding only those of the target's 2+ b/y ions that the old
                        conversion looks up in the wrong bin (and that the new one gets right)
  t70_highcharge.json   the target and the hit counts the model predicts for both conversions

The model mirrors the search at fragment_bin_tol 0.02, fragment_bin_offset 0 and
fragindex_min/max_fragmentmass 200/2000: index bins are BIN(1+ mass) for b/y ions in that
range; each peak is probed at charge 1 and 2 (3+ precursor -> max fragment charge 2). Peaks
are kept only when both conversions are at least 0.1 bin from a bin edge (the search reads
peak m/z as float), the old 2+ probe hits no target bin, the new one hits the right bin, and
the 1+ probe hits none. So the old build counts 0 index hits (below fragindex_min_ions_score
3: no candidate, no match) and the new build counts one per peak. Deterministic: seed 70.
"""

import json
import os
import random
import sys

# The shared masses and b/y helpers live in tests/unit/fixture_peptides.py; "python -I" leaves
# this script's directory off sys.path, so tests/unit/ is added here.
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from fixture_peptides import H2O, MONO, PROTON, by_ions_1plus  # noqa: E402

POOL = "GASPVTLNDQEHFYW"   # no C (no static mod needed), no K/R inside, no M (no oxidation question)
INV_WIDTH = 50.0           # 1 / 0.02
FRAG_MIN, FRAG_MAX = 200.0, 2000.0
MIN_PEAKS = 12


def bin_pos(mass):
    """BIN(mass) = (int)(mass * 50 + 1.0) and the fractional position inside that bin."""
    x = mass * INV_WIDTH + 1.0
    return int(x), x - int(x)


def pick(pep):
    ions = by_ions_1plus(pep)
    index_bins = set(bin_pos(m)[0] for m in ions if FRAG_MIN <= m <= FRAG_MAX)
    kept = []
    for m1 in ions:
        if not (FRAG_MIN <= m1 <= FRAG_MAX):
            continue
        mz2 = (m1 + PROTON) / 2.0
        b_new, f_new = bin_pos(mz2 * 2 - PROTON)
        b_old, f_old = bin_pos(mz2 * 2 - 1.0)
        b_one, f_one = bin_pos(mz2)
        if min(f_new, 1 - f_new, f_old, 1 - f_old) < 0.1:
            continue
        if b_new in index_bins and b_old not in index_bins and b_one not in index_bins:
            kept.append(mz2)
    return kept


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    rng = random.Random(70)
    for _ in range(10000):
        pep = "".join(rng.choice(POOL) for _ in range(22)) + "K"
        mh = sum(MONO[c] for c in pep) + H2O + PROTON
        if not 2000.0 < mh < 3500.0:
            continue
        peaks = pick(pep)
        if len(peaks) >= MIN_PEAKS:
            break
    else:
        raise SystemExit("no peptide met the conditions")

    with open(os.path.join(outdir, "t70_highcharge.fasta"), "w", newline="\r\n") as f:
        f.write(">target\nMAR%sAGAR\n" % pep)
        for i in range(30):
            body = "".join(rng.choice(POOL) for _ in range(rng.randint(10, 20))) + rng.choice("KR")
            f.write(">filler_%02d\nMAR%sAGAR\n" % (i, body))

    mz3 = (mh + 2 * PROTON) / 3.0
    with open(os.path.join(outdir, "t70_highcharge.ms2"), "w", newline="\r\n") as f:
        f.write("S\t1\t1\t%.6f\nZ\t3\t%.6f\n" % (mz3, mh))
        for k, mz in enumerate(sorted(peaks)):
            f.write("%.5f\t%.1f\n" % (mz, 100.0 if k % 2 else 60.0))

    info = {"target": pep, "mh": mh, "n_peaks": len(peaks), "old_hits": 0, "new_hits": len(peaks)}
    with open(os.path.join(outdir, "t70_highcharge.json"), "w", newline="\r\n") as f:
        json.dump(info, f, indent=1)
    print(json.dumps(info, indent=1))


if __name__ == "__main__":
    main()
