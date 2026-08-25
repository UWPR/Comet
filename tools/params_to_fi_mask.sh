#!/bin/bash
# params_to_fi_mask.sh
#
# One-command wrapper around carafe_prerun.sh for the single-flavor case: given ONE
# comet.params (fixed enzyme/mods/neutral-loss/mass range, PLUS two Carafe mask-
# threshold keys this script reads directly out of it -- see below) and a FASTA,
# drives the whole ahead-of-time pipeline end to end through to a finished .fi_mask:
#
#   .idx build -> variant export -> idx_to_carafe.py -> Carafe prediction -> .cps -> .fi_mask
#
# See docs/20260822_carafe_prerun.md for the full pipeline design and per-stage timing.
# This script itself has no stages of its own -- it's a thin argument translator in
# front of carafe_prerun.sh, always invoked with exactly one --flavor.
#
# The two threshold keys below are Carafe/mask-build-only. comet.exe accepts and
# stores them (Comet.cpp's paramHandlers) but never reads them for anything -- that
# registration exists purely so the SAME comet.params used here can also be handed
# to comet.exe for the actual masked search afterward without printing a stray
# "Warning - invalid parameter found" for either key:
#
#   carafe_mask_min_relative_intensity = 0.10   # -> carafe_cps_to_fi_mask.py --min-relative-intensity
#   carafe_mask_min_peaks              = 6      # -> carafe_cps_to_fi_mask.py --min-kept-peaks
#
# If either key is absent from --params, carafe_prerun.sh's own default for that
# threshold is used (0.10 / 6). An explicit --min-relative-intensity/--min-kept-peaks
# flag on this script's own command line always wins over both.
#
# Usage:
#   params_to_fi_mask.sh --params comet.params --fasta db.fasta --out DIR --comet /path/to/comet.exe
#       [any other carafe_prerun.sh flag: --charges, --include-decoys, --carafe-mode,
#        --parquet, --chunk-size, --quant, --workers, --venv-python, --ai-pred-py,
#        --stop-after STAGE, --delete-raw ...]
#
# Every other flag is passed through to carafe_prerun.sh untouched -- see that
# script's own header for what each one does and for the resume-via-markers behavior.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

PARAMS=""
FASTA=""
OUT_DIR=""
COMET=""
HAVE_MIN_REL=0
HAVE_MIN_KEPT=0
EXTRA_ARGS=()

while [ $# -gt 0 ]; do
  case "$1" in
    --params) PARAMS="$2"; shift 2 ;;
    --fasta) FASTA="$2"; EXTRA_ARGS+=(--fasta "$2"); shift 2 ;;
    --out) OUT_DIR="$2"; EXTRA_ARGS+=(--out "$2"); shift 2 ;;
    --comet) COMET="$2"; EXTRA_ARGS+=(--comet "$2"); shift 2 ;;
    --min-relative-intensity) HAVE_MIN_REL=1; EXTRA_ARGS+=("$1" "$2"); shift 2 ;;
    --min-kept-peaks) HAVE_MIN_KEPT=1; EXTRA_ARGS+=("$1" "$2"); shift 2 ;;
    *) EXTRA_ARGS+=("$1"); shift ;;
  esac
done

[ -n "$PARAMS" ] && [ -n "$FASTA" ] && [ -n "$OUT_DIR" ] && [ -n "$COMET" ] || {
  echo "Required: --params, --fasta, --out, --comet. See script header." >&2; exit 1; }
[ -f "$PARAMS" ] || { echo "--params not found: $PARAMS" >&2; exit 1; }

# get_param <key> -- prints the value of the first 'key = value' line in $PARAMS
# (up to the next whitespace or '#' comment), or nothing if the key is absent.
get_param() {
  grep -E "^$1[[:space:]]*=" "$PARAMS" | head -1 \
    | sed -E "s/^$1[[:space:]]*=[[:space:]]*([^[:space:]#]+).*/\1/"
}

if [ "$HAVE_MIN_REL" = 0 ]; then
  v=$(get_param carafe_mask_min_relative_intensity || true)
  [ -n "$v" ] && EXTRA_ARGS+=(--min-relative-intensity "$v")
fi
if [ "$HAVE_MIN_KEPT" = 0 ]; then
  v=$(get_param carafe_mask_min_peaks || true)
  [ -n "$v" ] && EXTRA_ARGS+=(--min-kept-peaks "$v")
fi

exec "$SCRIPT_DIR/carafe_prerun.sh" --flavor primary="$PARAMS" "${EXTRA_ARGS[@]}"
