#!/usr/bin/env python3
"""
Runs tools/carafe_ms2_to_fi_mask.py once per already-existing Carafe-inference chunk
(tools/run_carafe_chunked.py's chunk_preds/chunk_NNNNN/) instead of once against the
whole concatenated ms2_pred.tsv. Built because the monolithic form is not viable at real
full-proteome-phospho scale: the real phospho_charge2_withNL run's ms2_pred.tsv sums to
~3.63 BILLION fragment rows across its 2,498 chunks (measured directly, `wc -l`), and
carafe_ms2_to_fi_mask.py's FragmentTable holds the whole thing in memory (float32, 32
bytes/row) -- ~116GB just for that structure, before out_tsv_rows/ms2_df_by_content's
own overhead, against a 54GB machine. Concatenating first and running once was never
going to work; this runs the existing per-chunk ms2_df/ms2_pred files (each
independently small and safe, ~300K-1.6M fragment rows) through the mask builder
chunk-by-chunk instead, exactly mirroring run_carafe_chunked.py's own reason for
existing one level upstream.

(Python port of the original tools/build_carafe_mask_chunked.sh +
split_variant_map_for_chunks.awk pair, so it also runs in a native Windows terminal;
the awk's row_index-rewriting split now lives in
carafe_chunk_common.split_variant_map(). Marker/output layout is identical to the bash
era's, so a workdir started under the old driver resumes under this one. Normally
invoked through the umbrella CLI: `tools/carafe.py mask-chunks ...`.)

NOTE: this is the legacy chunked-TSV mask path, superseded for normal use by the
compact prediction store (carafe_pred_to_cps.py + carafe_cps_to_fi_mask.py) and kept
for TSV-only situations -- see docs/20260826_carafe.md.

Designed for the --ignore-modloss "second mask from the same Carafe run" case
specifically (docs/20260805_carafe.md Section 6.20): pairs an ALREADY-CHUNKED withNL
Carafe prediction (ms2_df/ms2_pred, from run_carafe_chunked.py) against a matching,
freshly-chunked NoNL out_tsv/variant_map (same peptide population, different .idx --
neutral_loss zeroed). The withNL predictions are never re-chunked or re-run; only the
noNL out_tsv/variant_map need splitting here, at the SAME 50,000-row boundaries the
withNL predictions already used, so chunk_NNNNN on both sides refers to the same
50,000-peptide slice of the (row-order-identical) population.

variant_map_tsv's row_index column is GLOBAL/positional in the source file, not
chunk-local -- carafe_chunk_common.split_variant_map() handles the rewrite (subtract
chunk_index * chunk_size) in one streaming pass, relying on the file being
row_index-ordered (true for idx_to_carafe.py's own output).

Usage:
  tools/carafe.py mask-chunks \\
    --out-tsv FILE --variant-map FILE --idx FILE \\
    --withnl-chunk-preds DIR --out DIR [options]

Options:
  --out-tsv FILE            the noNL population's idx_to_carafe.py out_tsv
                            (e.g. phospho_charge2_noNL_carafe_peptides.tsv). Required.
  --variant-map FILE        its .variants.tsv sidecar. Required.
  --idx FILE                the noNL .idx (neutral_loss zeroed) idx_to_carafe.py
                            exported from -- must match --variant-map's embedded
                            VarModConfig. Required.
  --withnl-chunk-preds DIR  the ALREADY-COMPLETED withNL run's chunk_preds/ directory
                            (run_carafe_chunked.py's --out DIR/chunk_preds), containing
                            chunk_NNNNN/chunk_NNNNN_ms2_df.tsv + _ms2_pred.tsv per
                            chunk. Required.
  --out DIR                 output directory (created if missing). Required.
  --chunk-size N            must match the withNL run's --chunk-size (default: 50000).
  --min-relative-intensity F  passed through to carafe_ms2_to_fi_mask.py (default: 0.10)
  --min-kept-peaks N          passed through (default: 6)
  --limit-chunks N            stop after N *newly run* chunks this invocation
                            (default: 1 -- calibration-safe; pass 0 for "run all
                            remaining chunks")
  --python PATH               python to invoke carafe_ms2_to_fi_mask.py with (default:
                            this same interpreter -- the mask builder is pure stdlib,
                            no Carafe venv needed)

Each chunk's mask lands at OUT/mask_chunks/chunk_NNNNN.fi_mask, with a matching
chunk_NNNNN.done marker (same resume semantics as run_carafe_chunked.py: a chunk with
an existing .done is skipped; an interrupted run resumes at the next incomplete chunk).

Merging all per-chunk masks into one final .fi_mask is a separate step --
tools/merge_carafe_fi_masks.py -- not done by this script.
"""

import argparse
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import carafe_chunk_common as common  # noqa: E402

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MASK_BUILDER = os.path.join(SCRIPT_DIR, "carafe_ms2_to_fi_mask.py")


def touch(path):
    with open(path, "w", newline="\n"):
        pass


def run_chunk_mask(base, args, dirs):
    nonl_out = os.path.join(dirs["out_chunks"], base + ".tsv")
    nonl_vmap = os.path.join(dirs["vmap_chunks"], base + ".tsv")
    withnl_ms2_df = os.path.join(args.withnl_chunk_preds, base, base + "_ms2_df.tsv")
    withnl_ms2_pred = os.path.join(args.withnl_chunk_preds, base, base + "_ms2_pred.tsv")
    out_mask = os.path.join(dirs["mask_chunks"], base + ".fi_mask")
    done_marker = os.path.join(dirs["mask_chunks"], base + ".done")
    log_path = os.path.join(dirs["mask_chunks"], base + ".log")

    if os.path.isfile(done_marker):
        print(f"[{base}] already done, skipping")
        return True
    if not (os.path.isfile(nonl_out) and os.path.isfile(nonl_vmap)):
        print(f"[{base}] SKIPPED: missing noNL split chunk ({nonl_out} / {nonl_vmap})")
        return False
    if not (os.path.isfile(withnl_ms2_df) and os.path.isfile(withnl_ms2_pred)):
        print(f"[{base}] SKIPPED: missing withNL prediction chunk ({withnl_ms2_df} / "
              f"{withnl_ms2_pred}) -- was it built with a different --chunk-size?")
        return False

    start = time.monotonic()
    with open(log_path, "wb") as log:
        rc = subprocess.run(
            [args.python, MASK_BUILDER,
             args.idx, nonl_out, nonl_vmap, withnl_ms2_df, withnl_ms2_pred, out_mask,
             "--ignore-modloss",
             "--min-relative-intensity", str(args.min_relative_intensity),
             "--min-kept-peaks", str(args.min_kept_peaks)],
            stdout=log, stderr=subprocess.STDOUT, check=False).returncode
    elapsed = int(round(time.monotonic() - start))
    if rc == 0:
        touch(done_marker)
        print(f"[{base}] done in {elapsed}s")
        return True
    print(f"[{base}] FAILED after {elapsed}s -- see {log_path}")
    return False


def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Per-chunk Carafe FI-mask builder (legacy chunked-TSV path).",
        formatter_class=argparse.RawDescriptionHelpFormatter, epilog=__doc__)
    ap.add_argument("--out-tsv", required=True)
    ap.add_argument("--variant-map", required=True)
    ap.add_argument("--idx", required=True)
    ap.add_argument("--withnl-chunk-preds", required=True)
    ap.add_argument("--out", dest="out_dir", required=True)
    ap.add_argument("--chunk-size", type=int, default=50000)
    ap.add_argument("--min-relative-intensity", type=float, default=0.10)
    ap.add_argument("--min-kept-peaks", type=int, default=6)
    ap.add_argument("--limit-chunks", type=int, default=1)
    ap.add_argument("--python", default=sys.executable)
    args = ap.parse_args(argv)

    for path, label in ((args.out_tsv, "--out-tsv"), (args.variant_map, "--variant-map"),
                        (args.idx, "--idx")):
        if not os.path.isfile(path):
            sys.exit(f"{label} not found: {path}")
    if not os.path.isdir(args.withnl_chunk_preds):
        sys.exit(f"--withnl-chunk-preds not a directory: {args.withnl_chunk_preds}")
    if not os.path.isfile(MASK_BUILDER):
        sys.exit("carafe_ms2_to_fi_mask.py not found next to this script")

    dirs = {
        "out_chunks": os.path.join(args.out_dir, "nonl_out_chunks"),
        "vmap_chunks": os.path.join(args.out_dir, "nonl_vmap_chunks"),
        "mask_chunks": os.path.join(args.out_dir, "mask_chunks"),
    }
    for d in dirs.values():
        os.makedirs(d, exist_ok=True)

    # ---- 1. Split noNL out_tsv (same boundaries as run_carafe_chunked.py's split) ----
    marker = os.path.join(dirs["out_chunks"], ".split_done")
    if not os.path.isfile(marker):
        print(f"[split-out] splitting {args.out_tsv} into {args.chunk_size}-row chunks "
              f"under {dirs['out_chunks']} ...")
        n = common.split_tsv_with_header(args.out_tsv, dirs["out_chunks"],
                                         args.chunk_size)
        touch(marker)
        print(f"[split-out] done: {n} chunks")
    else:
        print("[split-out] already split, reusing")

    # ---- 2. Split noNL variant_map (row_index rewritten per chunk) ----
    marker = os.path.join(dirs["vmap_chunks"], ".split_done")
    if not os.path.isfile(marker):
        print(f"[split-vmap] splitting {args.variant_map} into {args.chunk_size}-row "
              f"chunks under {dirs['vmap_chunks']} ...")
        common.split_variant_map(args.variant_map, dirs["vmap_chunks"], args.chunk_size)
        touch(marker)
        n = len(common.list_chunk_tsvs(dirs["vmap_chunks"]))
        print(f"[split-vmap] done: {n} chunks")
    else:
        print("[split-vmap] already split, reusing")

    # ---- 3. Sanity check: chunk counts must agree across all three inputs ----
    n_out = len(common.list_chunk_tsvs(dirs["out_chunks"]))
    n_vmap = len(common.list_chunk_tsvs(dirs["vmap_chunks"]))
    n_withnl = len([d for d in os.listdir(args.withnl_chunk_preds)
                    if d.startswith("chunk_")
                    and os.path.isdir(os.path.join(args.withnl_chunk_preds, d))])
    print(f"[driver] chunk counts -- noNL out_tsv: {n_out}, noNL variant_map: {n_vmap}, "
          f"withNL preds: {n_withnl}")
    if n_out != n_withnl:
        print(f"[driver] WARNING: noNL out_tsv chunk count ({n_out}) != withNL "
              f"prediction chunk count ({n_withnl}) -- populations may not actually "
              f"match row-for-row, or --chunk-size differs from the withNL run's. "
              f"Verify before trusting results.", file=sys.stderr)

    # ---- 4. Run mask builder per chunk ----
    all_bases = [os.path.basename(c)[:-len(".tsv")]
                 for c in common.list_chunk_tsvs(dirs["out_chunks"])]
    todo = [b for b in all_bases
            if not os.path.isfile(os.path.join(dirs["mask_chunks"], b + ".done"))]
    print(f"[driver] {len(all_bases)} total chunks, {len(todo)} not yet done")

    if args.limit_chunks != 0 and len(todo) > args.limit_chunks:
        todo = todo[:args.limit_chunks]
    print(f"[driver] running {len(todo)} chunk(s) this invocation")

    for base in todo:
        if not run_chunk_mask(base, args, dirs):
            print(f"[driver] {base} FAILED, continuing to next chunk")

    print("[driver] invocation complete.")


if __name__ == "__main__":
    main()
