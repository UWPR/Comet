#!/bin/bash
# build_carafe_mask_chunked.sh
#
# Runs tools/carafe_ms2_to_fi_mask.py once per already-existing Carafe-inference chunk
# (tools/run_carafe_chunked.sh's chunk_preds/chunk_NNNNN/) instead of once against the whole
# concatenated ms2_pred.tsv. Built because the monolithic form is not viable at real
# full-proteome-phospho scale: the real phospho_charge2_withNL run's ms2_pred.tsv sums to
# ~3.63 BILLION fragment rows across its 2,498 chunks (measured directly, `wc -l`), and
# carafe_ms2_to_fi_mask.py's FragmentTable holds the whole thing in memory (float32, 32
# bytes/row) -- ~116GB just for that structure, before out_tsv_rows/ms2_df_by_content's own
# overhead, against a 54GB machine. Concatenating first and running once was never going to
# work; this runs the existing per-chunk ms2_df/ms2_pred files (each independently small and
# safe, ~300K-1.6M fragment rows) through the mask builder chunk-by-chunk instead, exactly
# mirroring run_carafe_chunked.sh's own reason for existing one level upstream.
#
# Designed for the --ignore-modloss "second mask from the same Carafe run" case specifically
# (docs/20260805_carafe.md Section 6.20): pairs an ALREADY-CHUNKED withNL Carafe prediction
# (ms2_df/ms2_pred, from run_carafe_chunked.sh) against a matching, freshly-chunked NoNL
# out_tsv/variant_map (same peptide population, different .idx -- neutral_loss zeroed). The
# withNL predictions are never re-chunked or re-run; only the noNL out_tsv/variant_map need
# splitting here, at the SAME 50,000-row boundaries the withNL predictions already used, so
# chunk_NNNNN on both sides refers to the same 50,000-peptide slice of the (row-order-identical)
# population.
#
# variant_map_tsv's row_index column is GLOBAL/positional in the source file, not chunk-local
# -- tools/split_variant_map_for_chunks.awk handles the rewrite (subtract chunk_index *
# chunk_size) in one streaming pass, relying on the file being row_index-ordered (true for
# idx_to_carafe.py's own output).
#
# Usage:
#   ./build_carafe_mask_chunked.sh \
#     --out-tsv FILE --variant-map FILE --idx FILE \
#     --withnl-chunk-preds DIR --out DIR [options]
#
# Options:
#   --out-tsv FILE           the noNL population's idx_to_carafe.py out_tsv
#                           (e.g. phospho_charge2_noNL_carafe_peptides.tsv). Required.
#   --variant-map FILE        its .variants.tsv sidecar. Required.
#   --idx FILE               the noNL .idx (neutral_loss zeroed) idx_to_carafe.py exported
#                           from -- must match --variant-map's embedded VarModConfig. Required.
#   --withnl-chunk-preds DIR   the ALREADY-COMPLETED withNL run's chunk_preds/ directory
#                           (tools/run_carafe_chunked.sh's --out DIR/chunk_preds), containing
#                           chunk_NNNNN/chunk_NNNNN_ms2_df.tsv + _ms2_pred.tsv per chunk.
#                           Required.
#   --out DIR                 output directory (created if missing). Required.
#   --chunk-size N             must match the withNL run's --chunk-size (default: 50000).
#   --min-relative-intensity F  passed through to carafe_ms2_to_fi_mask.py (default: 0.10)
#   --min-kept-peaks N          passed through (default: 6)
#   --limit-chunks N            stop after N *newly run* chunks this invocation (default: 1 --
#                             calibration-safe; pass 0 for "run all remaining chunks")
#   --python PATH              python to invoke carafe_ms2_to_fi_mask.py with (default:
#                             python3 -- this script is pure stdlib, no Carafe venv needed)
#
# Each chunk's mask lands at $OUT/mask_chunks/chunk_NNNNN.fi_mask, with a matching
# chunk_NNNNN.done marker (same resume semantics as run_carafe_chunked.sh: a chunk with an
# existing .done is skipped; an interrupted run resumes at the next incomplete chunk).
#
# Merging all per-chunk masks into one final .fi_mask is a separate step --
# tools/merge_carafe_fi_masks.py -- not done by this script.

set -euo pipefail

OUT_TSV=""
VARIANT_MAP=""
IDX_FILE=""
WITHNL_CHUNK_PREDS=""
OUT_DIR=""
CHUNK_SIZE=50000
MIN_REL_INTENSITY=0.10
MIN_KEPT_PEAKS=6
LIMIT_CHUNKS=1
PYTHON_BIN="python3"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MASK_BUILDER="$SCRIPT_DIR/carafe_ms2_to_fi_mask.py"
VMAP_SPLITTER="$SCRIPT_DIR/split_variant_map_for_chunks.awk"

while [ $# -gt 0 ]; do
  case "$1" in
    --out-tsv) OUT_TSV="$2"; shift 2 ;;
    --variant-map) VARIANT_MAP="$2"; shift 2 ;;
    --idx) IDX_FILE="$2"; shift 2 ;;
    --withnl-chunk-preds) WITHNL_CHUNK_PREDS="$2"; shift 2 ;;
    --out) OUT_DIR="$2"; shift 2 ;;
    --chunk-size) CHUNK_SIZE="$2"; shift 2 ;;
    --min-relative-intensity) MIN_REL_INTENSITY="$2"; shift 2 ;;
    --min-kept-peaks) MIN_KEPT_PEAKS="$2"; shift 2 ;;
    --limit-chunks) LIMIT_CHUNKS="$2"; shift 2 ;;
    --python) PYTHON_BIN="$2"; shift 2 ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

for v in OUT_TSV VARIANT_MAP IDX_FILE WITHNL_CHUNK_PREDS OUT_DIR; do
  if [ -z "${!v}" ]; then
    echo "Missing required option for $v. See script header for usage." >&2
    exit 1
  fi
done
if [ ! -f "$OUT_TSV" ]; then echo "--out-tsv not found: $OUT_TSV" >&2; exit 1; fi
if [ ! -f "$VARIANT_MAP" ]; then echo "--variant-map not found: $VARIANT_MAP" >&2; exit 1; fi
if [ ! -f "$IDX_FILE" ]; then echo "--idx not found: $IDX_FILE" >&2; exit 1; fi
if [ ! -d "$WITHNL_CHUNK_PREDS" ]; then echo "--withnl-chunk-preds not a directory: $WITHNL_CHUNK_PREDS" >&2; exit 1; fi
if [ ! -f "$MASK_BUILDER" ]; then echo "carafe_ms2_to_fi_mask.py not found next to this script" >&2; exit 1; fi

NONL_OUT_CHUNK_DIR="$OUT_DIR/nonl_out_chunks"
NONL_VMAP_CHUNK_DIR="$OUT_DIR/nonl_vmap_chunks"
MASK_CHUNK_DIR="$OUT_DIR/mask_chunks"
mkdir -p "$NONL_OUT_CHUNK_DIR" "$NONL_VMAP_CHUNK_DIR" "$MASK_CHUNK_DIR"

# ---- 1. Split noNL out_tsv (same method as run_carafe_chunked.sh's own split) ----
if [ ! -f "$NONL_OUT_CHUNK_DIR/.split_done" ]; then
  echo "[split-out] splitting $OUT_TSV into ${CHUNK_SIZE}-row chunks under $NONL_OUT_CHUNK_DIR ..."
  HEADER=$(head -n 1 "$OUT_TSV")
  tail -n +2 "$OUT_TSV" | split -l "$CHUNK_SIZE" -d -a 5 --additional-suffix=.body - "$NONL_OUT_CHUNK_DIR/chunk_"
  for body in "$NONL_OUT_CHUNK_DIR"/chunk_*.body; do
    [ -e "$body" ] || continue
    chunk="${body%.body}.tsv"
    { printf '%s\n' "$HEADER"; cat "$body"; } > "$chunk"
    rm "$body"
  done
  touch "$NONL_OUT_CHUNK_DIR/.split_done"
  echo "[split-out] done: $(ls "$NONL_OUT_CHUNK_DIR"/chunk_*.tsv 2>/dev/null | wc -l) chunks"
else
  echo "[split-out] already split, reusing"
fi

# ---- 2. Split noNL variant_map (row_index rewritten per chunk) ----
if [ ! -f "$NONL_VMAP_CHUNK_DIR/.split_done" ]; then
  echo "[split-vmap] splitting $VARIANT_MAP into ${CHUNK_SIZE}-row chunks under $NONL_VMAP_CHUNK_DIR ..."
  awk -v OUTDIR="$NONL_VMAP_CHUNK_DIR" -v CHUNK_SIZE="$CHUNK_SIZE" -f "$VMAP_SPLITTER" "$VARIANT_MAP"
  touch "$NONL_VMAP_CHUNK_DIR/.split_done"
  echo "[split-vmap] done: $(ls "$NONL_VMAP_CHUNK_DIR"/chunk_*.tsv 2>/dev/null | wc -l) chunks"
else
  echo "[split-vmap] already split, reusing"
fi

# ---- 3. Sanity check: chunk counts must agree across all three inputs ----
n_out_chunks=$(ls "$NONL_OUT_CHUNK_DIR"/chunk_*.tsv 2>/dev/null | wc -l)
n_vmap_chunks=$(ls "$NONL_VMAP_CHUNK_DIR"/chunk_*.tsv 2>/dev/null | wc -l)
n_withnl_chunks=$(ls -d "$WITHNL_CHUNK_PREDS"/chunk_*/ 2>/dev/null | wc -l)
echo "[driver] chunk counts -- noNL out_tsv: $n_out_chunks, noNL variant_map: $n_vmap_chunks, withNL preds: $n_withnl_chunks"
if [ "$n_out_chunks" != "$n_withnl_chunks" ]; then
  echo "[driver] WARNING: noNL out_tsv chunk count ($n_out_chunks) != withNL prediction chunk count ($n_withnl_chunks) -- populations may not actually match row-for-row, or --chunk-size differs from the withNL run's. Verify before trusting results." >&2
fi

# ---- 4. Run mask builder per chunk ----
run_chunk_mask() {
  base="$1"   # e.g. chunk_00000
  nonl_out="$NONL_OUT_CHUNK_DIR/${base}.tsv"
  nonl_vmap="$NONL_VMAP_CHUNK_DIR/${base}.tsv"
  withnl_ms2_df="$WITHNL_CHUNK_PREDS/${base}/${base}_ms2_df.tsv"
  withnl_ms2_pred="$WITHNL_CHUNK_PREDS/${base}/${base}_ms2_pred.tsv"
  out_mask="$MASK_CHUNK_DIR/${base}.fi_mask"
  done_marker="$MASK_CHUNK_DIR/${base}.done"
  log="$MASK_CHUNK_DIR/${base}.log"

  if [ -f "$done_marker" ]; then
    echo "[$base] already done, skipping"
    return 0
  fi
  if [ ! -f "$nonl_out" ] || [ ! -f "$nonl_vmap" ]; then
    echo "[$base] SKIPPED: missing noNL split chunk ($nonl_out / $nonl_vmap)"
    return 1
  fi
  if [ ! -f "$withnl_ms2_df" ] || [ ! -f "$withnl_ms2_pred" ]; then
    echo "[$base] SKIPPED: missing withNL prediction chunk ($withnl_ms2_df / $withnl_ms2_pred) -- was it built with a different --chunk-size?"
    return 1
  fi

  start_ts=$(date +%s)
  if "$PYTHON_BIN" "$MASK_BUILDER" \
      "$IDX_FILE" "$nonl_out" "$nonl_vmap" "$withnl_ms2_df" "$withnl_ms2_pred" "$out_mask" \
      --ignore-modloss \
      --min-relative-intensity "$MIN_REL_INTENSITY" \
      --min-kept-peaks "$MIN_KEPT_PEAKS" \
      > "$log" 2>&1; then
    elapsed=$(( $(date +%s) - start_ts ))
    touch "$done_marker"
    echo "[$base] done in ${elapsed}s"
    return 0
  else
    elapsed=$(( $(date +%s) - start_ts ))
    echo "[$base] FAILED after ${elapsed}s -- see $log"
    return 1
  fi
}

mapfile -t ALL_CHUNKS < <(ls "$NONL_OUT_CHUNK_DIR"/chunk_*.tsv 2>/dev/null | xargs -n1 basename | sed 's/\.tsv$//' | sort)
TODO=()
for base in "${ALL_CHUNKS[@]}"; do
  [ -f "$MASK_CHUNK_DIR/${base}.done" ] && continue
  TODO+=("$base")
done
echo "[driver] ${#ALL_CHUNKS[@]} total chunks, ${#TODO[@]} not yet done"

if [ "$LIMIT_CHUNKS" != "0" ] && [ "${#TODO[@]}" -gt "$LIMIT_CHUNKS" ]; then
  TODO=("${TODO[@]:0:$LIMIT_CHUNKS}")
fi
echo "[driver] running ${#TODO[@]} chunk(s) this invocation"

for base in "${TODO[@]}"; do
  run_chunk_mask "$base" || echo "[driver] $base FAILED, continuing to next chunk"
done

echo "[driver] invocation complete."
