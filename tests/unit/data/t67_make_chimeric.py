#!/usr/bin/env python3
"""
Generate the T67 chimeric-spectrum fixtures (docs/20261105_RTSchimeric.md section 7.1).

Writes, into the directory given as the first argument (default: this script's):
  t67_chimeric.fasta        P1, P2, P3 as tryptic peptides inside small proteins, plus fillers
  t67_chimeric_range.txt    rts_repro SPECTRUM fixture for the charge-range form (one block per scan)
  t67_chimeric_list.txt     the same scans for the explicit-list form (one block per candidate)
  t67_chimeric.ms2          the same scans as a multi-Z-line .ms2 for RealtimeSearch.exe chimeric
                            (S line: P1's 2+ m/z; Z lines: the explicit-list candidates as MH+)
  t67_chimeric.json         the peptides, masses and m/z values the test asserts against

Peptides (monoisotopic, no modifications):
  P1  LVNELTEFAK           searched at 2+
  P2  found by search      3+; chosen so that (M2 + 3p)/3 is within 1 m/z of (M1 + 2p)/2, so the
                           charge-range form reaches it from P1's m/z with a +/- 2 m/z window
  P3  FVNELTELAK           a composition-identical shuffle of P1 (same mass, mostly different ions):
                           the "same charge twice" case
Scans (synthetic b/y ions, y 100 / b 60 per peptide, a fixed-seed noise floor):
  1  P1 only                                 -> exactly one row, P1 2+
  2  P1 (x3) + P2 (x1), precursor m/z = P1's -> iteration 0 P1 2+, iteration 1 P2 3+
  3  P1 (x3) + P3 (x1), same m/z             -> two rows, both 2+ (P1 then P3)
  4  P1 only, candidates 2+ and 3+ at P1 m/z -> one row at 2+ (list form; range form = scan 1)
  5  noise only                              -> no rows
  6  P1 plus a water-loss peak under every ion -> one row, P1 2+; with use_NL_ions on, a PI_DB
                                              search removes the 18 water peaks too (36 peaks),
                                              FI_DB (which never scores the NL matrix) only the 18
Deterministic: random.seed(67).
"""

import json
import os
import random
import sys

PROTON = 1.00727646688
H2O = 18.0105647
MONO = {
    "G": 57.021464, "A": 71.037114, "S": 87.032028, "P": 97.052764, "V": 99.068414,
    "T": 101.047679, "C": 103.009185, "L": 113.084064, "I": 113.084064, "N": 114.042927,
    "D": 115.026943, "Q": 128.058578, "K": 128.094963, "E": 129.042593, "M": 131.040485,
    "H": 137.058912, "F": 147.068414, "R": 156.101111, "Y": 163.063329, "W": 186.079313,
}
# residues used to build random tryptic peptides (no C/M/W to keep things plain, no K/R inside)
POOL = "GASPVTLNDQEHFY"


def neutral_mass(pep):
    return sum(MONO[c] for c in pep) + H2O


def mz(pep, z):
    return (neutral_mass(pep) + z * PROTON) / z


def by_ions(pep, max_frag_charge):
    """(m/z, series) for every b and y ion at charges 1..max_frag_charge, as Comet enumerates them."""
    out = []
    n = len(pep)
    b = PROTON
    y = H2O + PROTON
    for i in range(n - 1):
        b += MONO[pep[i]]
        y += MONO[pep[n - 1 - i]]
        for z in range(1, max_frag_charge + 1):
            out.append(((b + (z - 1) * PROTON) / z, "b"))
            out.append(((y + (z - 1) * PROTON) / z, "y"))
    return out


def random_tryptic(rng, length):
    return "".join(rng.choice(POOL) for _ in range(length - 1)) + rng.choice("KR")


def find_p2(rng, target_mz, max_dmz, exclude):
    """Random tryptic peptide (10-14 residues) whose 3+ m/z is within max_dmz of target_mz but not
    within 0.02 m/z of it (so the 20 ppm configured tolerance at P1's m/z never admits it)."""
    for _ in range(2000000):
        pep = random_tryptic(rng, rng.randint(10, 14))
        if pep in exclude:
            continue
        d = abs(mz(pep, 3) - target_mz)
        if 0.3 < d <= max_dmz:
            return pep
    raise RuntimeError("no P2 found")


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    rng = random.Random(67)

    p1 = "LVNELTEFAK"
    p3 = "FVNELTELAK"
    assert sorted(p1) == sorted(p3) and p1 != p3
    p2 = find_p2(rng, mz(p1, 2), 1.0, {p1, p3})

    fillers = []
    while len(fillers) < 300:
        pep = random_tryptic(rng, rng.randint(8, 14))
        if pep not in (p1, p2, p3) and pep not in fillers:
            fillers.append(pep)

    # proteins: "MAR" + peptide + "AGAR" makes the peptide tryptic (R before, A after, ends in K/R)
    with open(os.path.join(outdir, "t67_chimeric.fasta"), "w", newline="\r\n") as f:
        for name, pep in (("P1", p1), ("P2", p2), ("P3", p3)):
            f.write(">%s %s\n%s\n" % (name, pep, "MAR" + pep + "AGAR"))
        for i in range(0, len(fillers), 6):
            f.write(">filler%03d\n%s\n" % (i // 6, "MAR" + "".join(fillers[i:i + 6]) + "AGAR"))

    def noise(n):
        return [(rng.uniform(150.0, 1500.0), rng.uniform(5.0, 15.0)) for _ in range(n)]

    def ions(pep, precursor_z, scale):
        maxz = 1 if precursor_z == 1 else min(precursor_z - 1, 3)
        return [(m, (100.0 if s == "y" else 60.0) * scale) for m, s in by_ions(pep, maxz)]

    # scan 6: P1 plus a water-loss (-18.0106) peak under every ion, for the use_NL_ions removal
    # check -- XcorrScoreI() credits those peaks (0.2 weight, PI_DB only, 1+ a/b/y ions), so the
    # chimeric removal must clear them on PI_DB with use_NL_ions on and leave them otherwise
    water = [(m - H2O, i * 0.3) for m, i in ions(p1, 2, 1.0)]
    scans = {
        1: ions(p1, 2, 1.0) + noise(40),
        2: ions(p1, 2, 3.0) + ions(p2, 3, 1.0) + noise(40),
        3: ions(p1, 2, 3.0) + ions(p3, 2, 1.0) + noise(40),
        4: ions(p1, 2, 1.0) + noise(40),
        5: noise(60),
        6: ions(p1, 2, 1.0) + water + noise(40),
    }
    for k in scans:
        scans[k].sort()

    # the precursor candidates each form searches
    range_blocks = {s: [(2, mz(p1, 2))] for s in (1, 2, 3, 4, 5, 6)}
    list_blocks = {
        1: [(2, mz(p1, 2))],
        2: [(2, mz(p1, 2)), (3, mz(p2, 3))],
        3: [(2, mz(p1, 2)), (2, mz(p3, 2))],   # identical candidates -> de-duplicated to one
        4: [(2, mz(p1, 2)), (3, mz(p1, 2))],
        5: [(2, mz(p1, 2)), (3, mz(p1, 2))],
        6: [(2, mz(p1, 2))],
    }

    def write_fixture(path, blocks):
        with open(path, "w", newline="\r\n") as f:
            for scan in sorted(scans):
                peaks = scans[scan]
                for z, m in blocks[scan]:
                    f.write("SPECTRUM %d %d %.6f %d\n" % (scan, z, m, len(peaks)))
                    for pm, pi in peaks:
                        f.write("%.5f %.3f\n" % (pm, pi))

    write_fixture(os.path.join(outdir, "t67_chimeric_range.txt"), range_blocks)
    write_fixture(os.path.join(outdir, "t67_chimeric_list.txt"), list_blocks)

    # The same scans as a multi-Z-line .ms2 for the C# harness (RealtimeSearch.exe chimeric):
    # the S line carries P1's 2+ m/z (the charge-range form's precursor), the Z lines the
    # explicit-list candidates as (charge, MH+).
    with open(os.path.join(outdir, "t67_chimeric.ms2"), "w", newline="\r\n") as f:
        f.write("H\tCreationDate\tt67_make_chimeric.py\nH\tExtractor\tt67_make_chimeric.py\n")
        for scan in sorted(scans):
            f.write("S\t%d\t%d\t%.6f\n" % (scan, scan, mz(p1, 2)))
            for z, m in list_blocks[scan]:
                f.write("Z\t%d\t%.6f\n" % (z, m * z - (z - 1) * PROTON))
            for pm, pi in scans[scan]:
                f.write("%.5f %.3f\n" % (pm, pi))

    info = {
        "P1": {"seq": p1, "mass": neutral_mass(p1), "mz2": mz(p1, 2)},
        "P2": {"seq": p2, "mass": neutral_mass(p2), "mz3": mz(p2, 3)},
        "P3": {"seq": p3, "mass": neutral_mass(p3), "mz2": mz(p3, 2)},
        "fillers": len(fillers),
    }
    with open(os.path.join(outdir, "t67_chimeric.json"), "w", newline="\r\n") as f:
        json.dump(info, f, indent=1)
    print(json.dumps(info, indent=1))


if __name__ == "__main__":
    main()
