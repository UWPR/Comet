#!/usr/bin/env python3
"""Count Mango A/B label shifts caused by Z lines dropped by Comet's precursor filters.

For each <base>.ms2 / <base>.pep.xml.gz pair:
  - parse S/Z lines; each Z line (z, MH+) is one precursor ordinal k in file order
  - a Z line is "dropped" if MH+ outside [MASS_LO, MASS_HI] or z outside [Z_MIN, Z_MAX]
  - survivors are relabeled by Comet using survivor index s: label = (s//2, 'AB'[s%2])
  - correct label would be (k//2, 'AB'[k%2]); mismatch => mislabeled query
  - also validate the model against the pep.xml: multiset of (label, neutral mass, z)
    predicted vs observed must agree if pepXML prints every query.
"""
import gzip, os, re, sys, glob, collections

MASS_LO, MASS_HI = 600.0, 5000.0   # digest_mass_range
Z_MIN, Z_MAX = 1, 6                 # min_precursor_charge / max_precursor_charge
PROTON = 1.00727646688

d = sys.argv[1]
rx = re.compile(r'<spectrum_query spectrum="[^"]*_(\d{3})_([AB])\.(\d+)\.\d+\.(\d+)"[^>]*precursor_neutral_mass="([\d.]+)"[^>]*assumed_charge="(\d+)"')

tot = collections.Counter()
rows = []
for ms2 in sorted(glob.glob(os.path.join(d, '*.ms2'))):
    base = ms2[:-4]
    pep = base + '.pep.xml.gz'
    if not os.path.exists(pep):
        continue
    # ---- parse ms2
    scans = collections.OrderedDict()
    cur = None
    with open(ms2, 'r', errors='replace') as f:
        for line in f:
            c = line[0]
            if c == 'S':
                cur = int(line.split()[1]); scans[cur] = []
            elif c == 'Z':
                p = line.split(); scans[cur].append((int(p[1]), float(p[2])))
    # ---- model
    c = collections.Counter()
    predicted = collections.Counter()
    shifted_scans = []
    for scan, zs in scans.items():
        c['scans'] += 1; c['zlines'] += len(zs); c['pairs'] += len(zs)//2
        if len(zs) % 2: c['odd_zline_scans'] += 1
        s = 0; drop = []; mis_q = 0; mis_pairs = set()
        for k, (z, mh) in enumerate(zs):
            mz = (mh / z) if k == 0 else (mh + PROTON*(z-1)) / z   # Comet: first Z line uses mh/z
            dmass = mz*z - PROTON*(z-1)
            if k == 0 and z >= 2:
                c['first_zline_z2plus'] += 1; c['first_z%d' % z] += 1
            if not (MASS_LO <= dmass <= MASS_HI) or not (Z_MIN <= z <= Z_MAX):
                drop.append(k)
                if dmass < MASS_LO: c['drop_mass_lo'] += 1
                elif dmass > MASS_HI: c['drop_mass_hi'] += 1
                else: c['drop_charge'] += 1
                continue
            lab = (s//2, 'AB'[s%2]); s += 1
            neutral = dmass - PROTON
            predicted[(lab, round(neutral, 4), z)] += 1
            if lab != (k//2, 'AB'[k%2]):
                mis_q += 1; mis_pairs.add(k//2)
        if drop:
            c['scans_with_drop'] += 1; c['dropped_zlines'] += len(drop)
            # a drop orphans its own pair even if nothing shifts
            c['pairs_with_dropped_member'] += len(set(k//2 for k in drop))
        if mis_q:
            c['scans_shifted'] += 1; c['mislabeled_queries'] += mis_q
            c['mislabeled_pairs'] += len(mis_pairs)
            shifted_scans.append((scan, len(zs), drop, mis_q))
    # ---- observed
    observed = collections.Counter()
    with gzip.open(pep, 'rt', errors='replace') as f:
        for line in f:
            m = rx.search(line)
            if m:
                lab = (int(m.group(1)), m.group(2))
                observed[(lab, round(float(m.group(5)), 4), int(m.group(4)))] += 1
    c['pep_queries'] = sum(observed.values())
    c['pred_queries'] = sum(predicted.values())
    c['model_mismatch'] = sum((predicted - observed).values()) + sum((observed - predicted).values())
    tot.update(c)
    rows.append((os.path.basename(base), c, shifted_scans))

hdr = ['file','scans','zlines','pairs','scans_with_drop','dropped_zlines','drop_mass_lo','drop_mass_hi','drop_charge','pairs_with_dropped_member','first_zline_z2plus','first_z2','first_z3','first_z4','scans_shifted','mislabeled_queries','mislabeled_pairs','pred_queries','pep_queries','model_mismatch','odd_zline_scans']
print('\t'.join(hdr))
for base, c, _ in rows:
    print('\t'.join([base] + [str(c[h]) for h in hdr[1:]]))
print('\t'.join(['TOTAL'] + [str(tot[h]) for h in hdr[1:]]))
print()
print('Shifted scans (file, scan, nZ, dropped ordinals, mislabeled queries):')
for base, c, sh in rows:
    for scan, n, drop, mq in sh:
        print(base, scan, n, drop, mq)
print()
print('Dropped Z-line mass summary:')
