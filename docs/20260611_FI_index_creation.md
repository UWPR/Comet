# Fragment Ion Index Creation: v2026.01.1 vs v2026.02.0

**Date:** 2026-06-11  
**Database:** `human.canonical.fasta` (canonical human proteome)  
**Params:** `comet.params` — no enzyme, peptide length 8–25, no variable mods  
**Command:** `comet.exe -i -Dhuman.canonical.fasta`  
**Working directory:** `20260422-MHC/`

---

## Results

| Metric | v2026.01.1 | v2026.02.0 |
|--------|------------|------------|
| Run time | 5m 11s | 2m 22s |
| Memory (peak) | *(not reported)* | 44.1 GB |
| Plain peptides | 189,892,915 | ~1.897×10⁸ |
| `.idx` file size | 11.574 GB | 11.558 GB |

---

## Observations

**Speed:** v2026.02.0 is ~2.2× faster (5m11s → 2m22s). The v2026.01.1 output includes a separate
`- remove duplicate peptides` step that is absent in v2026.02.0, indicating deduplication was folded
into the parse step or otherwise restructured.

**Plain peptide count:** v2026.01.1 produces ~192,000 more plain peptides than v2026.02.0. This is
consistent with the known I/L long-path dedup bug in v2026.01.1, which fails to remove some
I/L-equivalent duplicate peptides; those extras survive into the index.

**Memory:** v2026.01.1 does not report peak memory usage. v2026.02.0 reports 44.1 GB and added
memory reporting to the `done` line as a new output feature.

**`.idx` file size:** v2026.02.0 produces a 16.6 MB smaller index (11.558 GB vs 11.574 GB),
directly reflecting the ~192K fewer unique peptides stored.

---

## Output format differences

v2026.01.1:
```
 Comet version "2026.01 rev. 1 (e4f767c)"

 Creating plain peptide/protein index file for fragment ion indexing:
 - parse peptides from database ...   5% 11% ... 100%
 - remove duplicate peptides
 - write peptides/proteins to file
 - mods:
 - initializing combinations (peptide length 25, max mods 0, total combinations 0)
 - 189892915 plain peptides, 0 modifiable peptides
 - done. human.canonical.fasta.idx ... 5 min 11 sec
```

v2026.02.0:
```
 Comet version "2026.02 rev. 0"

 Creating plain peptide/protein index file for fragment ion indexing:
 - parse peptides from database ... 100%
 - write peptides/proteins to file
 - mods:
 - initializing combinations (peptide length 25, max mods 0, total combinations 0)
 - 1.897e+08 plain peptides, 0.000e+00 modifiable peptides
 - created: human.canonical.fasta.idx
 - done. (2m:22s, 44.1GB)
```

Notable changes in v2026.02.0:
- `remove duplicate peptides` step removed from output (logic merged elsewhere)
- Peptide counts switched from integer to scientific notation
- `- created: <filename>` confirmation line added
- `done` line now includes wall-clock time and peak memory
