#!/usr/bin/env python3
"""
Merges per-chunk .fi_mask files (tools/build_carafe_mask_chunked.py's output, one per
50,000-row population slice) into a single final mask file, using
tools/carafe_ms2_to_fi_mask.py's own read_mask_file()/write_mask_file() so the binary format
(Comet Carafe FI mask v3) is produced identically to a real non-chunked run -- Phase 3's
CometPredictedMask::Load() cannot tell the difference between a mask built in one pass and one
merged from chunks.

Correctness: chunks partition the population by row_index range (tools/
carafe_chunk_common.py's split_variant_map()), which are non-overlapping by construction, so entries across
chunks never collide (a differing outcome would mean the chunk split itself was broken, not a
tie-breaking question this script needs to resolve) -- this script does not need to consider
merge conflicts. All chunks must report the SAME header (fingerprint, num_raw_peptides, idx_path,
threshold, min_kept_peaks, general_mode, var_mod_config), since they were all built from the
same --idx/--min-relative-intensity/--min-kept-peaks/--ignore-modloss inputs; a header mismatch
between chunks is treated as an error, not silently resolved, since it would mean two chunks
were built against different .idx files or settings.

Memory: holds all merged entries in memory at once (a plain list of 8-int tuples, ~124.8M
entries at full real-proteome-phospho scale -- tens of GB, verified to fit this project's
target machines; unlike the per-chunk mask BUILD step this script's own memory use scales with
the FINAL entry count, not the much larger fragment-row count that made the monolithic mask
build itself infeasible). A genuinely larger population would need a streaming merge (append
entries incrementally rather than holding the full list) -- not attempted here since it wasn't
needed at this run's scale.

Usage:
  merge_carafe_fi_masks.py --chunk-dir DIR --out FILE

  --chunk-dir DIR   directory of chunk_NNNNN.fi_mask files (tools/build_carafe_mask_chunked.py's
                    mask_chunks/ output). Only files with a matching chunk_NNNNN.done marker are
                    included, so a partially-completed chunked build can still be safely merged
                    up to whatever's actually finished (with --allow-partial; otherwise this
                    script refuses to merge until every expected chunk is done).
  --out FILE        output path for the merged mask file.
  --expect-chunks N  if given, verifies exactly N chunks were merged (fails loudly on a silent
                    gap, e.g. a chunk directory that's missing files nobody noticed).
  --allow-partial   merge whatever chunks are currently .done instead of requiring --expect-chunks
                    of them (or, without --expect-chunks, whatever's present) -- for inspecting
                    an in-progress chunked build. Off by default: a merge is expected to be a
                    deliberate "the chunked build is finished" step, not an accidental partial one.
"""

import argparse
import glob
import os
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))
import carafe_ms2_to_fi_mask as fi_mask  # noqa: E402

HEADER_KEYS_MUST_MATCH = (
    "SourceIdxFingerprint", "SourceIdxNumRawPeptides", "SourceIdxPath",
    "MinRelativeIntensity", "MinKeptPeaks", "GeneralMode", "VarModConfig",
)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--chunk-dir", required=True, help="Directory of chunk_NNNNN.fi_mask files")
    ap.add_argument("--out", required=True, help="Output path for the merged mask file")
    ap.add_argument("--expect-chunks", type=int, default=None,
                     help="Fail unless exactly this many chunks are merged")
    ap.add_argument("--allow-partial", action="store_true",
                     help="Merge whatever .done chunks are present instead of requiring "
                          "--expect-chunks of them")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args(argv)

    mask_files = sorted(glob.glob(os.path.join(args.chunk_dir, "chunk_*.fi_mask")))
    done_files = []
    for mf in mask_files:
        done_marker = mf[:-len(".fi_mask")] + ".done"
        if os.path.exists(done_marker):
            done_files.append(mf)
        elif args.verbose:
            print(f"skipping {mf} -- no matching .done marker", file=sys.stderr)

    if not done_files:
        print(f"No completed (.done-marked) chunk mask files found under {args.chunk_dir!r}",
              file=sys.stderr)
        sys.exit(1)

    if args.expect_chunks is not None and len(done_files) != args.expect_chunks:
        print(f"Expected {args.expect_chunks} completed chunks, found {len(done_files)} -- "
              f"refusing to merge an incomplete build. Pass --allow-partial to override.",
              file=sys.stderr)
        if not args.allow_partial:
            sys.exit(1)
    elif args.expect_chunks is None and not args.allow_partial:
        print(f"--expect-chunks not given -- refusing to guess whether {len(done_files)} "
              f"completed chunks is the full set. Pass --expect-chunks N to confirm, or "
              f"--allow-partial to merge whatever's here deliberately.", file=sys.stderr)
        sys.exit(1)

    print(f"Merging {len(done_files)} chunk mask file(s) from {args.chunk_dir!r} ...",
          file=sys.stderr)

    reference_header = None
    all_entries = []
    total_entries = 0
    for i, mf in enumerate(done_files):
        header, entries = fi_mask.read_mask_file(mf)
        if reference_header is None:
            reference_header = header
        else:
            for key in HEADER_KEYS_MUST_MATCH:
                if header.get(key) != reference_header.get(key):
                    print(f"ERROR: {mf!r} header {key}={header.get(key)!r} does not match "
                          f"{done_files[0]!r}'s {key}={reference_header.get(key)!r} -- chunks "
                          f"were not built from the same .idx/settings, refusing to merge",
                          file=sys.stderr)
                    sys.exit(1)
        all_entries.extend(entries)
        total_entries += len(entries)
        if args.verbose and (i % 200 == 0 or i == len(done_files) - 1):
            print(f"  [{i + 1}/{len(done_files)}] {mf}: {len(entries)} entries "
                  f"({total_entries} total so far)", file=sys.stderr)

    print(f"Merged {total_entries} total entries from {len(done_files)} chunks. Writing "
          f"{args.out!r} ...", file=sys.stderr)

    fi_mask.write_mask_file(
        args.out,
        fingerprint=reference_header["SourceIdxFingerprint"],  # hex string (CRC32, e.g. "99a2a034"), not decimal -- write_mask_file() just interpolates it
        num_raw_peptides=int(reference_header["SourceIdxNumRawPeptides"]),
        idx_path=reference_header["SourceIdxPath"],
        threshold=float(reference_header["MinRelativeIntensity"]),
        min_kept_peaks=int(reference_header["MinKeptPeaks"]),
        general_mode=(reference_header["GeneralMode"] == "1"),
        var_mod_config=reference_header["VarModConfig"],
        entries=all_entries,
    )
    print(f"Done: {args.out!r}, {total_entries} entries.", file=sys.stderr)


if __name__ == "__main__":
    main()
