#!/bin/bash
# carafe_prerun.sh
#
# The Carafe ahead-of-time pipeline driver -- docs/20260822_carafe_prerun.md milestone M4.
# Runs everything that must happen BEFORE a masked Comet FI search, end to end and
# resumable, so search time only ever consumes a finished .fi_mask file (never triggers
# Carafe, ai_pred.py, or any Python):
#
#   per flavor:  [s1] comet.exe -i  .idx build      (each flavor gets its own FASTA copy,
#                                                  since Comet writes <database_name>.idx
#                                                  -- two flavors of one FASTA would clobber)
#                [s2] comet.exe -x  variant export
#                [s3] idx_to_carafe.py              (out_tsv + variant-map sidecar)
#   once:        [s4] run_carafe_chunked.sh         (Carafe ai_pred.py -- the expensive step,
#                                                  hours; chunked + resumable on its own)
#                [s5] carafe_pred_to_cps.py         (compact prediction store; raw TSVs
#                                                  become deletable -- see --delete-raw)
#   per flavor:  [s6] carafe_cps_to_fi_mask.py      (~tens of minutes; --ignore-modloss is
#                                                  AUTO-DETECTED per flavor from its own
#                                                  VarModConfig: all neutral-loss deltas
#                                                  0.0 -> general mode)
#
# "Flavor" = one comet.params mod configuration sharing the same peptide population --
# canonically a withNL/noNL pair (same mods, neutral_loss zeroed in one). The FIRST flavor
# listed is the PRIMARY: its out_tsv feeds Carafe and the store. Every other flavor's
# conversion must produce the identical row count (checked, loud failure otherwise) --
# guaranteeing the store's row_index space is valid for all flavors' variant maps.
#
# Resume: each stage writes OUT/.prerun/<stage>.done on success and is skipped when that
# marker exists; delete a marker to re-run its stage (stage 4 additionally resumes at
# chunk granularity via run_carafe_chunked.sh's own markers). --stop-after lets a partial
# pipeline end deliberately (e.g. everything up to prediction on a GPU machine).
#
# Usage:
#   carafe_prerun.sh --fasta FILE --out DIR --comet PATH \
#       --flavor NAME=PARAMS_FILE [--flavor NAME=PARAMS_FILE ...] [options]
#
# Options:
#   --fasta FILE            protein database (target+decoy FASTA)
#   --out DIR               working/output directory (created if missing)
#   --comet PATH            comet.exe to build/export with (full path)
#   --flavor NAME=FILE      repeatable; first = primary. NAME becomes the file prefix
#                          (NAME.fasta.idx, NAME.carafe_peptides.tsv, NAME.fi_mask, ...)
#   --charges LIST          idx_to_carafe.py --charges (default: its own default "2,3";
#                          the validated production run used "2")
#   --include-decoys        pass through to idx_to_carafe.py (production runs did)
#   --carafe-mode MODE      ai_pred.py --mode (default: phosphorylation)
#   --parquet               transient parquet mode for stages 4-5 (prediction output
#                          ~5-9x smaller; stores verified byte-identical to the TSV
#                          path -- see run_carafe_chunked.sh's --parquet). Runs the
#                          s5 translator under the Carafe venv python (pandas/pyarrow).
#   --chunk-size N          run_carafe_chunked.sh chunk size (default 50000)
#   --quant u8|u16          store quantization (default u16 -- the validated choice;
#                          u8 diverges on ~2.8%% of variants, see plan doc Section 5.4)
#   --min-relative-intensity F / --min-kept-peaks N   mask thresholds (defaults 0.10 / 6)
#   --workers N             parallelism for translation + mask builds (default: cpus-2)
#   --venv-python PATH / --ai-pred-py PATH   forwarded to run_carafe_chunked.sh
#   --stop-after STAGE      stop after: idx, export, convert, predict, cps, mask
#   --delete-raw            after the store verifies (s5), delete the raw per-chunk Carafe
#                          prediction output (the ~hundreds-of-GB transient). Default OFF.
#
# Every stage's stdout/stderr lands in OUT/.prerun/<stage>.log.

set -euo pipefail

FASTA=""
OUT_DIR=""
COMET=""
FLAVOR_NAMES=()
FLAVOR_PARAMS=()
CHARGES=""
INCLUDE_DECOYS=0
CARAFE_MODE="phosphorylation"
PARQUET=0
CHUNK_SIZE=50000
QUANT="u16"
MIN_REL=0.10
MIN_KEPT=6
WORKERS=$(( $(nproc 2>/dev/null || echo 4) - 2 )); [ "$WORKERS" -lt 1 ] && WORKERS=1
VENV_PY=""
AI_PRED_PY=""
STOP_AFTER=""
DELETE_RAW=0

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

while [ $# -gt 0 ]; do
  case "$1" in
    --fasta) FASTA="$2"; shift 2 ;;
    --out) OUT_DIR="$2"; shift 2 ;;
    --comet) COMET="$2"; shift 2 ;;
    --flavor)
      name="${2%%=*}"; file="${2#*=}"
      if [ -z "$name" ] || [ -z "$file" ] || [ "$name" = "$2" ]; then
        echo "--flavor must be NAME=PARAMS_FILE, got '$2'" >&2; exit 1
      fi
      FLAVOR_NAMES+=("$name"); FLAVOR_PARAMS+=("$file"); shift 2 ;;
    --charges) CHARGES="$2"; shift 2 ;;
    --include-decoys) INCLUDE_DECOYS=1; shift ;;
    --carafe-mode) CARAFE_MODE="$2"; shift 2 ;;
    --parquet) PARQUET=1; shift ;;
    --chunk-size) CHUNK_SIZE="$2"; shift 2 ;;
    --quant) QUANT="$2"; shift 2 ;;
    --min-relative-intensity) MIN_REL="$2"; shift 2 ;;
    --min-kept-peaks) MIN_KEPT="$2"; shift 2 ;;
    --workers) WORKERS="$2"; shift 2 ;;
    --venv-python) VENV_PY="$2"; shift 2 ;;
    --ai-pred-py) AI_PRED_PY="$2"; shift 2 ;;
    --stop-after) STOP_AFTER="$2"; shift 2 ;;
    --delete-raw) DELETE_RAW=1; shift ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

[ -n "$FASTA" ] && [ -n "$OUT_DIR" ] && [ -n "$COMET" ] || {
  echo "Required: --fasta, --out, --comet. See script header." >&2; exit 1; }
[ "${#FLAVOR_NAMES[@]}" -ge 1 ] || { echo "At least one --flavor required." >&2; exit 1; }
[ -f "$FASTA" ] || { echo "--fasta not found: $FASTA" >&2; exit 1; }
[ -x "$COMET" ] || { echo "--comet not found/executable: $COMET" >&2; exit 1; }
for pf in "${FLAVOR_PARAMS[@]}"; do
  [ -f "$pf" ] || { echo "flavor params not found: $pf" >&2; exit 1; }
done

mkdir -p "$OUT_DIR/.prerun"
STATE="$OUT_DIR/.prerun"
PRIMARY="${FLAVOR_NAMES[0]}"

stage_done() { [ -f "$STATE/$1.done" ]; }
mark_done()  { touch "$STATE/$1.done"; }

maybe_stop() {  # call after finishing stage-kind $1 (idx/export/convert/predict/cps/mask)
  if [ -n "$STOP_AFTER" ] && [ "$1" = "$STOP_AFTER" ]; then
    echo "[driver] --stop-after $STOP_AFTER reached; stopping."
    exit 0
  fi
}

run_stage() {  # run_stage <stage-name> <desc> <cmd...>
  local stage="$1"; shift
  local desc="$1"; shift
  if stage_done "$stage"; then
    echo "[$stage] already done, skipping"
    return 0
  fi
  echo "[$stage] $desc ..."
  local t0; t0=$(date +%s)
  if "$@" > "$STATE/$stage.log" 2>&1; then
    mark_done "$stage"
    echo "[$stage] done in $(( $(date +%s) - t0 ))s"
  else
    echo "[$stage] FAILED after $(( $(date +%s) - t0 ))s -- see $STATE/$stage.log" >&2
    exit 1
  fi
}

# Rewrite one param in a params file copy (whole-line replacement, key must exist).
params_with() {  # params_with <src> <dst> <key> <value>
  local src="$1" dst="$2" key="$3" value="$4"
  if ! grep -qE "^${key}[[:space:]]*=" "$src"; then
    echo "params file $src has no '$key' line -- add one (e.g. '$key =') first" >&2
    exit 1
  fi
  sed -E "s|^${key}[[:space:]]*=.*|${key} = ${value}|" "$src" > "$dst"
}

# ---- stages 1-3, per flavor ----
for i in "${!FLAVOR_NAMES[@]}"; do
  F="${FLAVOR_NAMES[$i]}"
  PF="${FLAVOR_PARAMS[$i]}"
  FFASTA="$OUT_DIR/$F.fasta"
  FIDX="$FFASTA.idx"

  if ! stage_done "s1_idx_$F"; then
    cp -f "$FASTA" "$FFASTA"
    params_with "$PF" "$OUT_DIR/$F.build.params" database_name "$FFASTA"
  fi
  run_stage "s1_idx_$F" "build $FIDX (comet -i)" \
    "$COMET" "-P$OUT_DIR/$F.build.params" -i

  if ! stage_done "s2_export_$F"; then
    params_with "$PF" "$OUT_DIR/$F.export.params" database_name "$FIDX"
  fi
  run_stage "s2_export_$F" "export variant enumeration (comet -x)" \
    "$COMET" "-P$OUT_DIR/$F.export.params" "-x$OUT_DIR/$F.variants_export.tsv"

  convert_args=("$FIDX" "$OUT_DIR/$F.variants_export.tsv" "$OUT_DIR/$F.carafe_peptides.tsv")
  [ -n "$CHARGES" ] && convert_args+=(--charges "$CHARGES")
  [ "$INCLUDE_DECOYS" = 1 ] && convert_args+=(--include-decoys)
  run_stage "s3_convert_$F" "idx_to_carafe.py conversion" \
    python3 "$SCRIPT_DIR/idx_to_carafe.py" "${convert_args[@]}"
done
maybe_stop idx; maybe_stop export

# Cross-flavor population identity: every flavor's conversion must report the same row
# count as the primary's (the store's row_index space must be valid for all variant maps).
primary_rows=$(grep -oE "Wrote [0-9]+ rows" "$STATE/s3_convert_$PRIMARY.log" | grep -oE "[0-9]+" | head -1)
[ -n "$primary_rows" ] || { echo "cannot read primary row count from s3_convert_$PRIMARY.log" >&2; exit 1; }
for F in "${FLAVOR_NAMES[@]}"; do
  rows=$(grep -oE "Wrote [0-9]+ rows" "$STATE/s3_convert_$F.log" | grep -oE "[0-9]+" | head -1)
  if [ "$rows" != "$primary_rows" ]; then
    echo "FLAVOR POPULATION MISMATCH: $F converted $rows rows vs primary $PRIMARY's" \
         "$primary_rows -- flavors must share one peptide population (same enzyme/mass/" \
         "length/mod residues+masses; only neutral-loss deltas may differ)." >&2
    exit 1
  fi
done
echo "[driver] population identity OK: ${#FLAVOR_NAMES[@]} flavor(s) x $primary_rows rows"
maybe_stop convert

# ---- stage 4: Carafe prediction (once, primary flavor's out_tsv) ----
predict_args=(--in "$OUT_DIR/$PRIMARY.carafe_peptides.tsv" --out "$OUT_DIR/prediction"
              --chunk-size "$CHUNK_SIZE" --mode "$CARAFE_MODE" --device cpu
              --tf-type ms2 --limit-chunks 0 --jobs 1)
[ -n "$VENV_PY" ] && predict_args+=(--venv-python "$VENV_PY")
[ -n "$AI_PRED_PY" ] && predict_args+=(--ai-pred-py "$AI_PRED_PY")
[ "$PARQUET" = 1 ] && predict_args+=(--parquet)
run_stage "s4_predict" "Carafe prediction (run_carafe_chunked.sh -- the expensive step)" \
  bash "$SCRIPT_DIR/run_carafe_chunked.sh" "${predict_args[@]}"
# run_carafe_chunked.sh logs failures and continues; a chunk without .done means failure.
n_chunks=$(ls "$OUT_DIR/prediction/chunks/"chunk_*.tsv 2>/dev/null | wc -l)
n_done=$(ls "$OUT_DIR/prediction/chunk_preds/"*/.done 2>/dev/null | wc -l)
if [ "$n_chunks" -eq 0 ] || [ "$n_done" -ne "$n_chunks" ]; then
  rm -f "$STATE/s4_predict.done"
  echo "prediction incomplete: $n_done/$n_chunks chunks done -- re-run to resume" >&2
  exit 1
fi
maybe_stop predict

# ---- stage 5: compact prediction store ----
CPS_PY=python3
[ "$PARQUET" = 1 ] && CPS_PY="${VENV_PY:-$HOME/.carafe/.venv/bin/python}"
run_stage "s5_cps" "translate predictions -> $PRIMARY.cps" \
  "$CPS_PY" "$SCRIPT_DIR/carafe_pred_to_cps.py" \
    --chunks-dir "$OUT_DIR/prediction/chunks" \
    --preds-dir "$OUT_DIR/prediction/chunk_preds" \
    --source-out-tsv "$OUT_DIR/$PRIMARY.carafe_peptides.tsv" \
    --out "$OUT_DIR/$PRIMARY.cps" \
    --quant "$QUANT" --workers "$WORKERS"

if [ "$DELETE_RAW" = 1 ] && ! stage_done "s5b_delete_raw"; then
  echo "[s5b_delete_raw] store verified; deleting raw per-chunk Carafe output ..."
  rm -rf "$OUT_DIR/prediction/chunk_preds"
  mark_done "s5b_delete_raw"
fi
maybe_stop cps

# ---- stage 6: masks, per flavor (--ignore-modloss auto-detected) ----
for F in "${FLAVOR_NAMES[@]}"; do
  vmap="$OUT_DIR/$F.carafe_peptides.variants.tsv"
  # VarModConfig fields look like "79.966331STY--97.976896|..."; the number after each
  # "--" is that slot's neutral-loss delta. All zero -> general mode -> --ignore-modloss.
  vmc=$(head -1 "$vmap")
  mask_args=("$OUT_DIR/$F.fasta.idx" "$vmap" "$OUT_DIR/$PRIMARY.cps" "$OUT_DIR/$F.fi_mask"
             --min-relative-intensity "$MIN_REL" --min-kept-peaks "$MIN_KEPT"
             --workers "$WORKERS"
             --verify-out-tsv "$OUT_DIR/$PRIMARY.carafe_peptides.tsv")
  if ! echo "$vmc" | grep -qE -- "--(0*[1-9][0-9]*\.|0*\.0*[1-9])"; then
    echo "[driver] flavor $F: all neutral-loss deltas zero -> --ignore-modloss"
    mask_args+=(--ignore-modloss)
  else
    echo "[driver] flavor $F: neutral-loss delta present -> modloss channels active"
  fi
  run_stage "s6_mask_$F" "build $F.fi_mask from store" \
    python3 "$SCRIPT_DIR/carafe_cps_to_fi_mask.py" "${mask_args[@]}"
done
maybe_stop mask

echo "[driver] complete. Masks:"
for F in "${FLAVOR_NAMES[@]}"; do
  ls -la "$OUT_DIR/$F.fi_mask"
done
echo "[driver] point comet.params fragment_index_predicted_mask_file (or RTS --mask) at the"
echo "[driver] flavor-matching .fi_mask, with database_name = that flavor's .fasta.idx."
