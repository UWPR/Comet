---
name: comet-search-run
description: Run Comet batch or real-time searches, interpret output files, and understand search parameters. Use when executing searches, reading results, or configuring search settings.
---

# Comet Search Run

## Batch search

```bash
Comet.exe -D<database.idx> <input.raw>
# or with a FASTA database:
Comet.exe -D<database.fasta> <input.raw>
```

`-D` specifies the database. The `.idx` suffix triggers fragment ion index mode (FI_DB); `.fasta` triggers standard FASTA search mode.

Additional flags:
- `-P<comet.params>` — use a specific params file (default: `comet.params` in working dir)
- `-N<output_basename>` — override output file base name

## Create .idx file for Real-time search (RTS)

```bash
Comet.exe -D<database.fasta> -i
Comet.exe -D<database.fasta> -k
```

- the "-i" command line option creates a fragment ion index plain peptide database with .idx extension
- the "-j" command line option creates a peptide index database with .idx extension

## Real-time search (RTS)

```bash
RealtimeSearch.exe <query.raw> <ms1_reference.raw> <database.idx> <num_threads>
```

- `query.raw` — file containing MS2 spectra to search
- `ms1_reference.raw` — file containing MS1 scans for RT alignment (can be the same file)
- `database.idx` — fragment ion index database
- `num_threads` — number of parallel search threads (typically 20)

Output is written to `rts.out` in the working directory.

## Output file format (rts.out)

Each identified MS2 scan:
```
MS2 <scan>  <prevAA>.<peptide>.<nextAA>  <xcorr>  <evalue>  z <charge>  exp <exp_mass>  calc <calc_mass>  AScore <ascore>  Sites '<sites>'  <ms> ms  prot '<protein>'
```

Example:
```
MS2 1325  K.HAVSEGTK.A  1.8840  1.68E-08  z 2  exp 827.4132  calc 827.4137  AScore 0.00  Sites ''  8 ms  prot 'sp|O60814|H2B1K_HUMAN Histone H2B...'
```

Modification notation: `[mass_shift]` inline, e.g. `S[79.9663]` = phosphoserine, `M[15.9949]` = oxidized Met.
N-terminal mod: `n[mass]`, C-terminal mod: `c[mass]`.

Output also includes:
- Search histogram (ms per spectrum distribution)
- `<= 5ms / > 5ms / > 10ms` breakdown
- 5 slowest MS2 runs
- `initialize search elapsed time` — index load time
- `search elapsed time` — parallel search time with avg ms/spec and Hz

## Database types

| Suffix | Type constant | Description |
|--------|--------------|-------------|
| `.idx` | `FI_DB` | Fragment ion index (pre-built, fast RTS) |
| `.fasta` | `FASTA_DB` | Standard FASTA, searched directly |
| `.idx` (peptide) | `PI_DB` | Peptide index variant |

## Key search parameters (comet.params)

- `peptide_mass_tolerance` / `peptide_mass_units` — precursor mass window
- `fragment_bin_tol` / `fragment_bin_offset` — fragment ion binning
- `enzyme_number` — 0 = no enzyme, 1 = trypsin, etc.
- `allowed_missed_cleavage`
- `peptide_length_range` — e.g. `8 25` for MHC peptides
- `num_threads` — overridden by RTS command-line argument
- `num_results` — hits to report per spectrum
- `minimum_xcorr` — score cutoff

## g_pvProteinNameCache (indexed DB)

At index load time, all protein names from the `.idx` file are cached in `g_pvProteinNameCache` (an `unordered_map<comet_fileoffset_t, string>`). For `human.target-decoy.fasta.idx` this is ~40,858 entries, ~7 MB RAM. RTS protein lookups are then in-memory hashtable hits with no per-spectrum file I/O.
