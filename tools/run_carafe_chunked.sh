#!/bin/bash
# run_carafe_chunked.sh
#
# Splits a Carafe idx_to_carafe.py peptide TSV (sequence/mods/mod_sites/charge)
# into fixed-size row chunks and runs Carafe's real ai_pred.py over each chunk
# as a separate process/invocation, instead of one monolithic all-rows-in-
# memory call. Written after phospho_charge2_withNL_carafe died silently after
# ~51h wall time with zero bytes written anywhere (see chat 2026-08-16):
# ai_pred.py reads its whole --in_file into one pandas DataFrame, does one
# single-threaded (--device cpu) predict_ms2() call over the whole thing, and
# writes nothing to disk until that entire call returns -- so a 124.8M-row
# input is an all-or-nothing multi-day black box with no progress signal and
# no resume point if interrupted.
#
# This wrapper gives each chunk its own process, its own output files, and a
# ".done" marker -- so progress is visible, memory is bounded per-chunk
# instead of for the whole 124.8M-row set at once, and a killed/interrupted
# run resumes at the next incomplete chunk instead of restarting from zero.
#
# ai_pred.py's --device cpu path wraps predict_ms2() in
# threadpool_limits(limits=1, ...) for BLAS/OpenMP regardless of machine core
# count (confirmed by reading its source), so multiple chunks can safely run
# as concurrent OS processes (--jobs N) without fighting each other for the
# same thread pool -- each process is its own single-threaded worker. In
# practice, though, measured concurrent throughput on this project's dev
# machine was *worse* than serial (4-way parallel: ~140 rows/sec/process,
# ~560 rows/sec aggregate, vs. ~1850 rows/sec running alone) -- something
# outside threadpool_limits's BLAS/OpenMP scope contends across processes
# (suspected: model-loading I/O over a slow filesystem bridge). Default
# --jobs 1; only raise it after calibrating on the target machine, and don't
# assume it will help -- measure first.
#
# Usage:
#   ./run_carafe_chunked.sh --in FILE --out DIR [options]
#
# Options (all have defaults suitable for a first calibration run):
#   --in FILE           input peptide TSV (idx_to_carafe.py output). Required.
#   --out DIR            output directory (created if missing). Required.
#   --chunk-size N        data rows per chunk (default: 50000 -- calibration-sized;
#                        raise for the real run once a rate is known)
#   --mode MODE          ai_pred.py --mode (default: phosphorylation, matching
#                        the dead run)
#   --device DEV          ai_pred.py --device (default: cpu -- no GPU in this env)
#   --tf-type TYPE        ai_pred.py --tf_type (default: ms2). NOTE: despite the
#                        name, every tf_type branch in ai_pred.py's main() calls
#                        both predict_rt() AND predict_ms2() regardless -- the
#                        branches only differ in which model_dir each step uses.
#                        There is no CLI-level way to skip RT prediction; its
#                        output (*_rt_pred.tsv) is simply unused by
#                        carafe_ms2_to_fi_mask.py, not absent. (Measured cost
#                        of including it is small relative to MS2 prediction,
#                        so this is not worth working around.)
#   --parquet            transient parquet mode (docs/20260822_carafe_prerun.md
#                        Section 6.1): converts each input chunk to parquet inline
#                        (cached as chunk_NNNNN.input.parquet) and runs ai_pred.py
#                        --fast, so prediction output lands as parquet (~5-9x
#                        smaller -- the ~390GB full-proteome-phospho transient
#                        high-water mark becomes ~45GB). carafe_pred_to_cps.py
#                        auto-detects parquet chunk outputs. Requires pandas+pyarrow
#                        in the --venv-python environment (the Carafe venv has them).
#   --limit-chunks N       stop after N *newly run* chunks this invocation
#                        (default: 1 -- calibration-safe; pass 0 for "run all
#                        remaining chunks")
#   --jobs N             concurrent ai_pred.py processes (default: 1 -- see
#                        the parallelism note above before raising this)
#   --venv-python PATH     python to invoke ai_pred.py with
#                        (default: ~/.carafe/.venv/bin/python)
#   --ai-pred-py PATH      path to ai_pred.py
#                        (default: /mnt/c/Work/Carafe/src/main/resources/py/v2/ai_pred.py)
#
# Each chunk's output lands in $OUT/chunk_preds/chunk_NNNNN/, with:
#   ai_pred.log            ai_pred.py's stdout+stderr
#   mem_samples.tsv         epoch_seconds, ai_pred.py RSS (KB), system swap used (KB),
#                        sampled every 5s while the process runs
#   .start_time / .end_time  UTC timestamps
#   .elapsed_seconds / .rate_rows_per_sec
#   .done                 written only after ai_pred.py exits 0 -- presence of
#                        this file is what makes a chunk "already done" and
#                        skippable on the next invocation
#
# Concatenating completed chunks' *_ms2_df.tsv / *_ms2_pred.tsv into single
# files for carafe_ms2_to_fi_mask.py is a separate follow-up step, not done by
# this script -- run this script to completion (or however many chunks you
# want) first.

set -euo pipefail

IN_TSV=""
OUT_DIR=""
CHUNK_SIZE=50000
MODE="phosphorylation"
DEVICE="cpu"
TF_TYPE="ms2"
PARQUET=0
LIMIT_CHUNKS=1
JOBS=1
VENV_PY="$HOME/.carafe/.venv/bin/python"
AI_PRED_PY="/mnt/c/Work/Carafe/src/main/resources/py/v2/ai_pred.py"

while [ $# -gt 0 ]; do
  case "$1" in
    --in) IN_TSV="$2"; shift 2 ;;
    --out) OUT_DIR="$2"; shift 2 ;;
    --chunk-size) CHUNK_SIZE="$2"; shift 2 ;;
    --mode) MODE="$2"; shift 2 ;;
    --device) DEVICE="$2"; shift 2 ;;
    --tf-type) TF_TYPE="$2"; shift 2 ;;
    --parquet) PARQUET=1; shift ;;
    --limit-chunks) LIMIT_CHUNKS="$2"; shift 2 ;;
    --jobs) JOBS="$2"; shift 2 ;;
    --venv-python) VENV_PY="$2"; shift 2 ;;
    --ai-pred-py) AI_PRED_PY="$2"; shift 2 ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

if [ -z "$IN_TSV" ] || [ -z "$OUT_DIR" ]; then
  echo "Usage: $0 --in FILE --out DIR [options]" >&2
  exit 1
fi
if [ ! -f "$IN_TSV" ]; then
  echo "Input file not found: $IN_TSV" >&2
  exit 1
fi
if [ ! -x "$VENV_PY" ]; then
  echo "venv python not found/executable: $VENV_PY" >&2
  exit 1
fi
if [ ! -f "$AI_PRED_PY" ]; then
  echo "ai_pred.py not found: $AI_PRED_PY" >&2
  exit 1
fi

CHUNK_DIR="$OUT_DIR/chunks"
PRED_DIR="$OUT_DIR/chunk_preds"
mkdir -p "$CHUNK_DIR" "$PRED_DIR"

# ---- 1. Split (idempotent) ----
if [ ! -f "$CHUNK_DIR/.split_done" ]; then
  echo "[split] splitting $IN_TSV into ${CHUNK_SIZE}-row chunks under $CHUNK_DIR ..."
  HEADER=$(head -n 1 "$IN_TSV")
  # split the body (no header) into fixed-line pieces, then prepend the header
  # to each piece so every chunk is independently a valid ai_pred.py input.
  tail -n +2 "$IN_TSV" | split -l "$CHUNK_SIZE" -d -a 5 --additional-suffix=.body - "$CHUNK_DIR/chunk_"
  for body in "$CHUNK_DIR"/chunk_*.body; do
    [ -e "$body" ] || continue
    chunk="${body%.body}.tsv"
    { printf '%s\n' "$HEADER"; cat "$body"; } > "$chunk"
    rm "$body"
  done
  touch "$CHUNK_DIR/.split_done"
  n_chunks=$(ls "$CHUNK_DIR"/chunk_*.tsv 2>/dev/null | wc -l)
  echo "[split] done: $n_chunks chunks"
else
  echo "[split] already split (found $CHUNK_DIR/.split_done), reusing existing chunks"
fi

# ---- 2. Run one chunk ----
run_chunk() {
  chunk="$1"
  base=$(basename "$chunk" .tsv)
  chunk_out="$PRED_DIR/$base"
  mkdir -p "$chunk_out"

  if [ -f "$chunk_out/.done" ]; then
    echo "[$base] already done, skipping"
    return 0
  fi

  nrows=$(($(wc -l < "$chunk") - 1))
  echo "[$base] starting: $nrows rows, mode=$MODE device=$DEVICE tf_type=$TF_TYPE parquet=$PARQUET, $(date -u +%FT%TZ)"
  date -u +%FT%TZ > "$chunk_out/.start_time"
  start_ts=$(date +%s)

  # --parquet (docs/20260822_carafe_prerun.md Section 6.1, the M1a-adopted transient
  # mode): ai_pred.py --fast is all-or-nothing -- it reads --in_file with read_parquet()
  # too -- so the input chunk must be converted first (trivial: ~0.1s and ~14x smaller
  # than the TSV). Conversion is cached next to the chunk and reused on resume. Outputs
  # then land as ${base}_*.parquet (~5-9x smaller than TSV); carafe_pred_to_cps.py
  # auto-detects them.
  in_file="$chunk"
  extra_args=()
  if [ "$PARQUET" = 1 ]; then
    pq_in="${chunk%.tsv}.input.parquet"
    if [ ! -f "$pq_in" ]; then
      "$VENV_PY" - "$chunk" "$pq_in" > "$chunk_out/parquet_convert.log" 2>&1 << 'PYEOF'
import sys
import pandas as pd
src, dst = sys.argv[1], sys.argv[2]
df = pd.read_csv(src, sep="\t", low_memory=False, dtype={"mod_sites": str, "mods": str})
df["mods"] = df["mods"].fillna("")
df["mod_sites"] = df["mod_sites"].fillna("")
df.to_parquet(dst, compression="zstd")
PYEOF
      if [ ! -f "$pq_in" ]; then
        echo "[$base] FAILED converting input to parquet -- see $chunk_out/parquet_convert.log"
        return 1
      fi
    fi
    in_file="$pq_in"
    extra_args=(--fast)
  fi

  "$VENV_PY" "$AI_PRED_PY" \
      --in_file "$in_file" \
      --out_dir "$chunk_out" \
      --out_prefix "$base" \
      --mode "$MODE" \
      --device "$DEVICE" \
      --tf_type "$TF_TYPE" \
      "${extra_args[@]}" \
      > "$chunk_out/ai_pred.log" 2>&1 &
  ai_pid=$!

  # background memory sampler: RSS of the ai_pred.py process + system swap
  # used, every 5s -- direct evidence for/against a swap-thrashing failure
  # mode, instead of guessing after the fact.
  (
    echo -e "epoch\trss_kb\tswap_used_kb"
    while kill -0 "$ai_pid" 2>/dev/null; do
      rss=$(ps -o rss= -p "$ai_pid" 2>/dev/null | tr -d ' ')
      swap=$(free -k | awk '/^Swap:/{print $3}')
      echo -e "$(date +%s)\t${rss:-0}\t${swap:-0}"
      sleep 5
    done
  ) > "$chunk_out/mem_samples.tsv" &
  mon_pid=$!

  set +e
  wait "$ai_pid"
  rc=$?
  set -e
  wait "$mon_pid" 2>/dev/null || true

  end_ts=$(date +%s)
  elapsed=$((end_ts - start_ts))
  date -u +%FT%TZ > "$chunk_out/.end_time"
  echo "$elapsed" > "$chunk_out/.elapsed_seconds"

  if [ "$rc" -ne 0 ]; then
    echo "[$base] FAILED (exit $rc) after ${elapsed}s -- see $chunk_out/ai_pred.log"
    return 1
  fi

  peak_rss=$(awk -F'\t' 'NR>1 && $2+0>m{m=$2} END{print m+0}' "$chunk_out/mem_samples.tsv")
  peak_swap=$(awk -F'\t' 'NR>1 && $3+0>m{m=$3} END{print m+0}' "$chunk_out/mem_samples.tsv")
  rate=$(awk -v n="$nrows" -v s="$elapsed" 'BEGIN{ if (s>0) printf "%.1f", n/s; else print "n/a" }')
  echo "$rate" > "$chunk_out/.rate_rows_per_sec"
  touch "$chunk_out/.done"
  echo "[$base] done in ${elapsed}s (${rate} rows/sec), peak RSS ${peak_rss} KB, peak swap-used ${peak_swap} KB"
}
export -f run_chunk
export VENV_PY AI_PRED_PY MODE DEVICE TF_TYPE PRED_DIR PARQUET

# ---- 3. Drive chunks, honoring --limit-chunks and --jobs ----
mapfile -t ALL_CHUNKS < <(ls "$CHUNK_DIR"/chunk_*.tsv 2>/dev/null | sort)
TODO=()
for c in "${ALL_CHUNKS[@]}"; do
  base=$(basename "$c" .tsv)
  [ -f "$PRED_DIR/$base/.done" ] && continue
  TODO+=("$c")
done
echo "[driver] ${#ALL_CHUNKS[@]} total chunks, ${#TODO[@]} not yet done"

if [ "$LIMIT_CHUNKS" != "0" ] && [ "${#TODO[@]}" -gt "$LIMIT_CHUNKS" ]; then
  TODO=("${TODO[@]:0:$LIMIT_CHUNKS}")
fi
echo "[driver] running ${#TODO[@]} chunk(s) this invocation, jobs=$JOBS"

if [ "$JOBS" -le 1 ]; then
  for c in "${TODO[@]}"; do
    # Do not let `set -e` abort the whole multi-hour run over one bad chunk --
    # log it and keep going; it stays undone (no .done marker) and will be
    # retried on the next invocation.
    run_chunk "$c" || echo "[driver] $(basename "$c" .tsv) FAILED, continuing to next chunk"
  done
else
  printf '%s\n' "${TODO[@]}" | xargs -P "$JOBS" -I{} bash -c 'run_chunk "$@"' _ {}
fi

echo "[driver] invocation complete."
