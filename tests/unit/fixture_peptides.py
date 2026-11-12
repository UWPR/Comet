"""
Peptide masses and synthetic-spectrum helpers shared by the RTS unit tests (run_tests.py) and
the fixture generators in tests/unit/data/ (t67_make_chimeric.py, t68_make_histfill.py,
t70_make_highcharge.py).

One copy of the constants on purpose: before 2026-11-09 run_tests.py used a proton mass of
1.007276 and an H2O of 18.010565 while the generators used 1.00727646688 and 18.0105647, and
nine tests each carried their own copy of the residue table and b/y ladder.

The generators run as "python -I <script>", which leaves the script's directory off sys.path,
so they add tests/unit/ to sys.path themselves before importing this module.
"""

PROTON = 1.00727646688
H2O = 18.0105647

# monoisotopic residue masses, the values Comet uses
MONO = {
    "G": 57.021464, "A": 71.037114, "S": 87.032028, "P": 97.052764, "V": 99.068414,
    "T": 101.047679, "C": 103.009185, "L": 113.084064, "I": 113.084064, "N": 114.042927,
    "D": 115.026943, "Q": 128.058578, "K": 128.094963, "E": 129.042593, "M": 131.040485,
    "H": 137.058912, "F": 147.068414, "R": 156.101111, "Y": 163.063329, "W": 186.079313,
}


def residue_masses(pep, mods=None):
    """Residue masses of pep; mods maps a 0-based position to a mass delta added there."""
    mods = mods or {}
    return [MONO[c] + mods.get(i, 0.0) for i, c in enumerate(pep)]


def neutral_mass(pep, mods=None):
    return sum(residue_masses(pep, mods)) + H2O


def mz(pep, z, mods=None):
    """Precursor m/z at charge z."""
    return (neutral_mass(pep, mods) + z * PROTON) / z


def by_ladder(pep, mods=None):
    """(b, y): the 1+ b1..b(n-1) and y1..y(n-1) m/z lists."""
    res = residue_masses(pep, mods)
    b, y = PROTON, H2O + PROTON
    bs, ys = [], []
    for i in range(len(res) - 1):
        b += res[i]
        y += res[len(res) - 1 - i]
        bs.append(b)
        ys.append(y)
    return bs, ys


def by_ions_1plus(pep, mods=None):
    """The 1+ b and y m/z values interleaved b1, y1, b2, y2, ..."""
    bs, ys = by_ladder(pep, mods)
    return [m for pair in zip(bs, ys) for m in pair]


def by_ions(pep, max_frag_charge, mods=None):
    """(m/z, "b" | "y") for every b and y ion at fragment charges 1..max_frag_charge, in the
    order Comet enumerates them."""
    bs, ys = by_ladder(pep, mods)
    out = []
    for b, y in zip(bs, ys):
        for z in range(1, max_frag_charge + 1):
            out.append(((b + (z - 1) * PROTON) / z, "b"))
            out.append(((y + (z - 1) * PROTON) / z, "y"))
    return out


def by_peaks(pep, b_inten=60.0, y_inten=100.0, mods=None):
    """1+ b/y peaks as (m/z, intensity), interleaved b1, y1, ... (unsorted)."""
    bs, ys = by_ladder(pep, mods)
    out = []
    for b, y in zip(bs, ys):
        out += [(b, b_inten), (y, y_inten)]
    return out


def weak_noise(n=15, inten=5.0):
    """A fixed, evenly spaced weak noise floor (250.55 + k * 61.37): enough peaks that a
    spectrum keeps minimum_peaks after a chimeric iteration removes the target's ions."""
    return [(250.55 + k * 61.37, inten) for k in range(n)]


def spectrum_block(scan, z, precursor_mz, peaks, tol=None):
    """An rts_repro SPECTRUM block as a list of lines (tests/rts_repro/README.md): the header,
    with the optional per-candidate tolerance in m/z, then one "m/z intensity" line per peak."""
    head = f"SPECTRUM {scan} {z} {precursor_mz:.6f} {len(peaks)}" + ("" if tol is None else f" {tol}")
    return [head] + [f"{m:.5f} {i:.1f}" for m, i in peaks]
