#!/usr/bin/env python3
"""
Translate a chunked Carafe prediction run (tools/run_carafe_chunked.sh's chunk_preds/ TSV
output) into one compact prediction store (.cps -- tools/carafe_cps.py, docs/
20260822_carafe_prerun.md Section 5, milestone M2). After the resulting store verifies, the
raw Carafe output (386-395GB at real full-proteome-phospho scale) is discardable: the store
holds everything both the withNL and --ignore-modloss noNL mask builds consume, at ~35-45x
smaller (u8 quantization) -- see the format module's docstring.

Inputs mirror the chunked pipeline's own layout:
  --chunks-dir     run_carafe_chunked.sh's <out>/chunks/ (chunk_NNNNN.tsv out_tsv slices,
                   header + 50K data rows each; row order IS row_index order)
  --preds-dir      its <out>/chunk_preds/ (chunk_NNNNN/chunk_NNNNN_ms2_df.tsv + _ms2_pred.tsv)
  --source-out-tsv the ORIGINAL unchunked out_tsv (for the store's provenance header:
                   data-row count + head CRC; not re-read row-by-row)

Per chunk, the same content-tuple join carafe_ms2_to_fi_mask.py uses (via its own
read_out_tsv/read_ms2_df/read_ms2_pred, so parsing -- including FragmentTable's float32
storage -- is bit-identical to what the TSV mask builds saw) resolves Carafe's internal
nAA-sort reordering; rows are then quantized and appended to the store in row_index order.

Chunks are processed by a multiprocessing pool (--workers) but appended strictly in chunk
order (imap preserves it), so the output is deterministic regardless of worker count.

Writes <out>.building alongside the store while running; the final .cps only appears on
success (CpsWriter.finalize() renames nothing -- it writes the real file only at the end --
so a crashed run leaves a .payload.tmp and no misleading half-store).
"""

import argparse
import multiprocessing
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import carafe_cps  # noqa: E402
import carafe_ms2_to_fi_mask as fi_mask  # noqa: E402


def process_chunk(job):
    """Worker: one chunk -> (chunk_base, payload_blob, row_sizes, n_rows, n_missing,
    has_modloss). Rows are PACKED TO BYTES here in the worker (carafe_cps.pack_row(), the
    same serializer CpsWriter.append_row() uses) rather than returned as Python object
    graphs: a chunk's rows as tuples-of-tuples is ~150MB of Python objects, and with 12
    workers outpacing a parent that was doing the packing itself, Pool.imap's in-order
    result buffer accumulated dozens of those -- ~44GB parent RSS within minutes on the
    first full-scale attempt. Packed, a chunk is ~7MB of bytes and the parent's only work
    is a write, so no backlog can form."""
    chunk_base, chunk_tsv, ms2_df_tsv, ms2_pred_tsv, quant = job
    qchar, qmax = carafe_cps.QUANT_PARAMS[quant]

    out_rows = fi_mask.read_out_tsv(chunk_tsv)           # row_index -> content tuple
    ms2_by_content = fi_mask.read_ms2_df(ms2_df_tsv)     # content tuple -> (nAA, start, stop)
    pred, has_modloss = fi_mask.read_ms2_pred(ms2_pred_tsv)

    import array
    blob_parts = []
    row_sizes = array.array("I")
    n_rows = 0
    n_missing = 0
    for row_index in range(len(out_rows)):
        content = out_rows[row_index]
        entry = ms2_by_content.get(content)
        if entry is None:
            # Mirrors carafe_ms2_to_fi_mask.py's "not found -> skip" warning path; a missing
            # row here would leave the store misaligned with row_index, which is unacceptable
            # -- so record a zero row (nAA from the sequence itself, all intensities 0) and
            # count it; the caller fails loudly if any chunk has misses.
            n_missing += 1
            nAA = len(content[0])
            packed = carafe_cps.pack_row(nAA, 0.0, 0.0, [(0, 0, 0, 0)] * (nAA - 1), qchar)
            blob_parts.append(packed)
            row_sizes.append(len(packed))
            n_rows += 1
            continue
        nAA, start, stop = entry
        rows8 = pred[start:stop]   # list of 8-tuples, CHANNELS order, float32-derived
        base8 = max((v for row in rows8 for v in row), default=0.0)
        base4 = max((v for row in rows8 for v in row[:4]), default=0.0)
        quantized = []
        if base8 > 0.0:
            scale = qmax / base8
            for b_z1, _b2, y_z1, _y2, b_ml, _bml2, y_ml, _yml2 in rows8:
                quantized.append((
                    min(qmax, int(b_z1 * scale + 0.5)),
                    min(qmax, int(y_z1 * scale + 0.5)),
                    min(qmax, int(b_ml * scale + 0.5)),
                    min(qmax, int(y_ml * scale + 0.5))))
        else:
            quantized = [(0, 0, 0, 0)] * (nAA - 1)
        packed = carafe_cps.pack_row(nAA, base8, base4, quantized, qchar)
        blob_parts.append(packed)
        row_sizes.append(len(packed))
        n_rows += 1

    return chunk_base, b"".join(blob_parts), row_sizes, n_rows, n_missing, has_modloss


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--chunks-dir", required=True)
    ap.add_argument("--preds-dir", required=True)
    ap.add_argument("--source-out-tsv", required=True)
    ap.add_argument("--out", required=True, help="Output .cps path")
    ap.add_argument("--quant", choices=sorted(carafe_cps.QUANT_PARAMS), default="u8")
    ap.add_argument("--workers", type=int, default=max(1, (os.cpu_count() or 2) - 2))
    ap.add_argument("--limit-chunks", type=int, default=0,
                     help="Process only the first N chunks (0 = all). A limited run FAILS "
                          "finalization on purpose (row count won't match the source header) "
                          "unless --source-out-tsv is a matching slice -- intended for "
                          "experiments that read the .payload.tmp path via the test harness, "
                          "or with a sliced source file.")
    args = ap.parse_args()

    chunk_tsvs = sorted(Path(args.chunks_dir).glob("chunk_*.tsv"))
    if args.limit_chunks:
        chunk_tsvs = chunk_tsvs[:args.limit_chunks]
    if not chunk_tsvs:
        print(f"no chunk_*.tsv under {args.chunks_dir!r}", file=sys.stderr)
        sys.exit(1)

    jobs = []
    for ct in chunk_tsvs:
        base = ct.stem
        pd = Path(args.preds_dir) / base
        ms2_df = pd / f"{base}_ms2_df.tsv"
        ms2_pred = pd / f"{base}_ms2_pred.tsv"
        if not ms2_df.is_file() or not ms2_pred.is_file():
            print(f"missing prediction files for {base} under {pd}", file=sys.stderr)
            sys.exit(1)
        jobs.append((base, str(ct), str(ms2_df), str(ms2_pred), args.quant))

    # Source provenance: head CRC ties the store to the specific out_tsv; the row COUNT in
    # the header is the count actually written (== the source's data-row count when run over
    # the full chunk set, since the chunks were split from it). Downstream consumers
    # cross-check the count against the variant map they read anyway (CpsReader.verify_source).
    if not Path(args.source_out_tsv).is_file():
        print(f"--source-out-tsv not found: {args.source_out_tsv}", file=sys.stderr)
        sys.exit(1)
    src_crc = carafe_cps.out_tsv_head_crc32(args.source_out_tsv)

    t0 = time.time()
    written = 0
    n_chunks_done = 0
    n_missing_total = 0
    mode = None
    writer = None
    pool = multiprocessing.Pool(processes=args.workers)
    try:
        for chunk_base, blob, row_sizes, n_rows, n_missing, has_modloss in pool.imap(
                process_chunk, jobs):
            if writer is None:
                mode = "phospho" if has_modloss else "general"
                writer = carafe_cps.CpsWriter(
                    args.out, source_rows=0, source_head_crc=src_crc,
                    quant=args.quant, mode=mode)
            writer.append_packed(blob, row_sizes)
            written += n_rows
            n_chunks_done += 1
            n_missing_total += n_missing
            if n_missing:
                print(f"[{chunk_base}] WARNING: {n_missing} out_tsv rows had no ms2_df match",
                      file=sys.stderr)
            if n_chunks_done % 100 == 0 or n_chunks_done == len(jobs):
                elapsed = time.time() - t0
                rate = written / elapsed if elapsed > 0 else 0
                print(f"[{n_chunks_done}/{len(jobs)} chunks] {written} rows written, "
                      f"{elapsed:.0f}s, {rate:.0f} rows/s", file=sys.stderr)
    finally:
        pool.close()
        pool.join()

    if n_missing_total:
        print(f"FAILING: {n_missing_total} rows had no ms2_df content match -- the store "
              f"would carry zero rows misrepresenting real predictions. Investigate before "
              f"translating.", file=sys.stderr)
        sys.exit(1)

    writer.source_rows = written   # finalize() checks written == header row count
    writer.finalize()
    print(f"Done: {args.out!r}, {written} rows, quant={args.quant}, mode={mode}, "
          f"{time.time() - t0:.0f}s total", file=sys.stderr)


if __name__ == "__main__":
    main()
