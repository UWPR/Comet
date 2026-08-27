#!/usr/bin/env python3
"""
Build a Comet Carafe FI mask (v3, byte-compatible with tools/carafe_ms2_to_fi_mask.py's
output) from a compact prediction store (.cps -- tools/carafe_cps.py) instead of raw Carafe
TSV output -- docs/20260822_carafe_prerun.md milestone M3.

This replaces the chunked TSV mask build (tools/build_carafe_mask_chunked.py, ~5.5h +
merge) as the primary mask-(re)build path: any threshold/floor/ignore-modloss combination
can be re-swept from the ~31GB store in minutes, without the 386GB raw TSVs and without
re-running Carafe. (The plan doc originally sketched this as a --from-cps flag on
carafe_ms2_to_fi_mask.py; it lives in its own CLI instead because that script's 6
positional TSV arguments have no sensible meaning in cps mode -- same capability, cleaner
interface. All decision logic is shared: carafe_cps.compute_variant_mask_from_cps() calls
carafe_ms2_to_fi_mask.py's own threshold/floor/pack helpers.)

Usage:
  carafe_cps_to_fi_mask.py <idx_file> <variant_map_tsv> <cps_file> <out_mask_file>
      [--ignore-modloss] [--min-relative-intensity F] [--min-kept-peaks N]
      [--verify-out-tsv PATH] [--workers N]

  idx_file          the .idx this mask will be used with (fingerprint + raw-peptide count
                    baked into the mask header, exactly as the TSV builder does)
  variant_map_tsv   tools/idx_to_carafe.py's sidecar for THAT .idx (supplies the
                    (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod) tuples, the row_index
                    mapping into the store, and the VarModConfig header line)
  cps_file          the compact prediction store (may be built from a DIFFERENT .idx
                    flavor's export -- e.g. the withNL store serving a noNL mask build via
                    --ignore-modloss -- since the store is keyed by row_index over the
                    shared peptide population, not by .idx-specific tuples)
  --verify-out-tsv  optionally check the store's head-CRC provenance against the original
                    out_tsv (skippable since that file may be gone once the store is the
                    durable artifact)

Scale design (124.8M variants):

- The variant map is streamed in parallel BYTE RANGES, never loaded whole (the TSV
  builder's read_variant_map() list would be tens of GB here). Same-tuple lines are
  consecutive by construction (idx_to_carafe.py writes each variant's charge rows in one
  inner loop), so a worker owns exactly the tuple-groups whose FIRST line starts inside
  its byte range, reading past the range end to finish its last group and skipping the
  partial group at its start (owned by the previous worker) -- the standard parallel
  text-split pattern.
- Workers return mask entries PACKED to ENTRY_FMT bytes (the M2 lesson: returning Python
  object graphs to a slower parent lets Pool.imap's result buffer blow up -- 44GB RSS on
  the first M2 translation attempt), so the parent holds ~5.2GB of entry bytes at full
  scale, not ~30GB of tuples.
- The mask file's entries must be sorted by (iWhichPeptide, modNumIdx, cNtermMod,
  cCtermMod). The variant map's enumeration order is NOT globally key-ordered --
  empirically confirmed on the real 124.8M-row phospho map (mod-variant enumeration does
  not nest inside peptide order: e.g. key (1267306, 2458, ...) is followed by
  (762231, 132169, ...); the chunked TSV builder never noticed because it re-sorted at
  every chunk write and again at merge). So each worker SORTS its own range's entries and
  the parent k-way-merges the sorted runs (heapq.merge) while streaming the file out.
  After writing, the whole file is re-read and checked strictly increasing (proving both
  sort order and key uniqueness) -- a violation there means duplicate keys across ranges
  and is a loud abort.
"""

import heapq

import argparse
import multiprocessing
import os
import struct
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import carafe_cps  # noqa: E402
import carafe_ms2_to_fi_mask as fi_mask  # noqa: E402


def find_data_start(vmap_path):
    """Byte offset of the first data line (after the '# VarModConfig:' comment and the
    column-header line), plus the VarModConfig string itself."""
    with open(vmap_path, "rb") as f:
        first = f.readline()
        if not first.startswith(b"# VarModConfig:"):
            raise ValueError(f"{vmap_path!r}: missing '# VarModConfig:' first line -- "
                             f"rebuild the export with a current comet.exe -x")
        var_mod_config = first.decode("ascii").rstrip("\r\n").partition(": ")[2]
        f.readline()   # column header
        return f.tell(), var_mod_config


_MAX_VMAP_LINE = 4096   # variant-map lines are ~30-50 bytes; 4KB is a generous ceiling


def _parse_vmap_line(line, vmap_path, line_pos):
    parts = line.rstrip(b"\r\n").split(b"\t")
    if len(parts) != 5:
        raise ValueError(f"{vmap_path!r}: malformed line at byte {line_pos}: {line!r}")
    return int(parts[0]), (int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4]))


def iter_vmap_groups(vmap_path, data_start, byte_start, byte_end):
    """Yield (key_tuple, [row_index, ...]) for every tuple-group OWNED by
    [byte_start, byte_end): a group is owned iff its FIRST line starts in the range (line
    ownership: a line belongs to the range containing its first byte). Reading continues
    past byte_end to finish the last owned group; leading lines that continue a group
    started in the previous range are skipped -- decided by comparing the first in-range
    line's key against the PREVIOUS line's key (read via a small backward window), not by
    blanket-skipping the first group (a range whose first line STARTS a new group owns it)."""
    with open(vmap_path, "rb") as f:
        skip_key = None   # skip leading lines while their key == skip_key
        if byte_start <= data_start:
            f.seek(data_start)
        else:
            # Position at the first line starting at >= byte_start, and learn the PREVIOUS
            # line's key so a leading continuation-group can be distinguished from a fresh
            # group starting exactly at our boundary. Two cases:
            window_start = max(data_start, byte_start - _MAX_VMAP_LINE)
            f.seek(window_start)
            window = f.read(byte_start - window_start)

            def _line_start_after(nl_index):
                if nl_index < 0:
                    if window_start != data_start:
                        raise ValueError(
                            f"{vmap_path!r}: no newline within {_MAX_VMAP_LINE} bytes "
                            f"before byte {byte_start} -- line longer than expected?")
                    return data_start
                return window_start + nl_index + 1

            if window.endswith(b"\n"):
                # Case 1: byte_start sits exactly on a line boundary. The previous line is
                # the one ENDING at byte_start; its start is after the second-to-last
                # newline. We are already positioned (post-read) exactly at our first line.
                prev_line_start = _line_start_after(window.rfind(b"\n", 0, len(window) - 1))
                prev_line = window[prev_line_start - window_start:]
                _, skip_key = _parse_vmap_line(prev_line, vmap_path, prev_line_start)
                f.seek(byte_start)
            else:
                # Case 2: byte_start falls mid-line. That straddling line belongs to the
                # previous range; consume it (positioning at our true first line) and use
                # its key as the continuation marker.
                prev_line_start = _line_start_after(window.rfind(b"\n"))
                f.seek(prev_line_start)
                prev_line = f.readline()
                _, skip_key = _parse_vmap_line(prev_line, vmap_path, prev_line_start)

        cur_key = None
        cur_rows = []
        while True:
            line_pos = f.tell()
            line = f.readline()
            if not line:
                break
            row_index, key = _parse_vmap_line(line, vmap_path, line_pos)
            if skip_key is not None:
                if key == skip_key:
                    continue   # continuation of the previous range's last group
                skip_key = None
            if key != cur_key:
                if cur_key is not None:
                    yield cur_key, cur_rows
                if line_pos >= byte_end:
                    cur_key = None
                    break
                cur_key = key
                cur_rows = [row_index]
            else:
                cur_rows.append(row_index)
        if cur_key is not None:
            yield cur_key, cur_rows


def process_range(job):
    """Worker: one variant-map byte range -> (range_index, packed_entries_bytes SORTED by
    key, n_entries). The variant map's own order is not key order (see module docstring),
    so each range is sorted here; the parent merges the sorted runs."""
    (range_index, vmap_path, data_start, byte_start, byte_end, cps_path,
     min_rel, min_kept, has_modloss) = job

    reader = carafe_cps.CpsReader(cps_path)
    keyed = []
    for key, row_indices in iter_vmap_groups(vmap_path, data_start, byte_start, byte_end):
        rows4_pc, b8s, b4s, nAA_seen = [], [], [], None
        for ri in row_indices:
            nAA, b8, b4, rows4 = reader.read_row(ri)
            nAA_seen = nAA
            rows4_pc.append(rows4)
            b8s.append(b8)
            b4s.append(b4)
        got = carafe_cps.compute_variant_mask_from_cps(
            rows4_pc, nAA_seen, b8s, b4s, min_rel, min_kept,
            has_modloss=has_modloss, is_modified=(key[1] != -1))
        keyed.append((key, struct.pack(fi_mask.ENTRY_FMT, key[0], key[1], key[2], key[3],
                                        got[0], got[1], got[2], got[3])))
    reader.close()
    keyed.sort(key=lambda kp: kp[0])
    return range_index, b"".join(p for _, p in keyed), len(keyed)


def iter_packed_entries(blob):
    """Yield (key, entry_bytes) from one packed sorted run."""
    for off in range(0, len(blob), fi_mask.ENTRY_SIZE):
        e = blob[off:off + fi_mask.ENTRY_SIZE]
        yield struct.unpack_from("<Iibb", e), e


def merge_sorted_runs(blobs):
    """k-way merge of per-range sorted packed-entry runs -> yields entry_bytes in global
    key order. Pure streaming; memory is the blobs themselves plus the heap."""
    return (e for _key, e in heapq.merge(
        *(iter_packed_entries(b) for b in blobs if b), key=lambda kp: kp[0]))


def verify_written_mask_sorted(path):
    """Stream the finished mask file and confirm entries are strictly increasing by key
    (proves both sort order and key uniqueness) -- same check the M2-era merge verification
    used. Returns the entry count."""
    with open(path, "rb") as f:
        magic = f.readline()
        assert magic == fi_mask.MASK_FILE_MAGIC, magic
        while True:
            line = f.readline()
            if not line or line == b"\n":
                break
        (count,) = struct.unpack("<Q", f.read(8))
        prev = None
        n = 0
        while True:
            buf = f.read(200000 * fi_mask.ENTRY_SIZE)
            if not buf:
                break
            for e in struct.iter_unpack(fi_mask.ENTRY_FMT, buf):
                key = e[:4]
                if prev is not None and key <= prev:
                    raise ValueError(f"written mask not strictly increasing at entry {n}: "
                                     f"{prev} -> {key}")
                prev = key
                n += 1
        if n != count:
            raise ValueError(f"written mask header count {count} != streamed entries {n}")
        return n


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("idx_file")
    ap.add_argument("variant_map_tsv")
    ap.add_argument("cps_file")
    ap.add_argument("out_mask_file")
    ap.add_argument("--min-relative-intensity", type=float,
                     default=fi_mask.DEFAULT_MIN_RELATIVE_INTENSITY)
    ap.add_argument("--min-kept-peaks", type=int, default=fi_mask.DEFAULT_MIN_KEPT_PEAKS)
    ap.add_argument("--ignore-modloss", action="store_true",
                     help="Build a general-mode-equivalent mask (same semantics as "
                          "carafe_ms2_to_fi_mask.py's flag: first-4-channel base peak, "
                          "modloss masks 0, GeneralMode: 1)")
    ap.add_argument("--verify-out-tsv", default=None,
                     help="Optional: the original out_tsv, to check the store's provenance "
                          "head-CRC against")
    ap.add_argument("--workers", type=int, default=max(1, (os.cpu_count() or 2) - 2))
    args = ap.parse_args(argv)

    t0 = time.time()
    reader = carafe_cps.CpsReader(args.cps_file)
    store_rows = reader.row_count
    store_mode = reader.header.get("Mode", "phospho")
    if args.verify_out_tsv:
        reader.verify_source(args.verify_out_tsv)
        print(f"store provenance vs {args.verify_out_tsv!r}: OK", file=sys.stderr)
    reader.close()
    has_modloss = (store_mode == "phospho") and not args.ignore_modloss

    print(f"Fingerprinting {args.idx_file} ...", file=sys.stderr)
    fingerprint, num_raw_peptides = fi_mask.idx_fingerprint(args.idx_file)

    data_start, var_mod_config = find_data_start(args.variant_map_tsv)
    vmap_size = os.path.getsize(args.variant_map_tsv)
    n_ranges = max(1, args.workers * 4)   # oversplit so a slow range can't straggle badly
    span = max(1, (vmap_size - data_start) // n_ranges)
    jobs = []
    for i in range(n_ranges):
        byte_start = data_start + i * span
        byte_end = data_start + (i + 1) * span if i < n_ranges - 1 else vmap_size
        if byte_start >= vmap_size:
            break
        jobs.append((i, args.variant_map_tsv, data_start, byte_start, byte_end,
                     args.cps_file, args.min_relative_intensity, args.min_kept_peaks,
                     has_modloss))

    print(f"{len(jobs)} ranges, {args.workers} workers, has_modloss={has_modloss}, "
          f"threshold={args.min_relative_intensity}, floor={args.min_kept_peaks}",
          file=sys.stderr)

    blobs = [None] * len(jobs)
    counts = [0] * len(jobs)
    n_done = 0
    pool = multiprocessing.Pool(processes=args.workers)
    try:
        for range_index, blob, n in pool.imap_unordered(process_range, jobs):
            blobs[range_index] = blob
            counts[range_index] = n
            n_done += 1
            if n_done % 8 == 0 or n_done == len(jobs):
                elapsed = time.time() - t0
                total = sum(counts)
                print(f"[{n_done}/{len(jobs)} ranges] {total} entries, {elapsed:.0f}s",
                      file=sys.stderr)
    finally:
        pool.close()
        pool.join()

    total = sum(counts)
    print(f"Merging {len(jobs)} sorted runs and writing {total} entries to "
          f"{args.out_mask_file!r} ...", file=sys.stderr)
    header_is_general = not has_modloss
    with open(args.out_mask_file, "wb") as f:
        f.write(fi_mask.MASK_FILE_MAGIC)
        f.write(f"SourceIdxFingerprint: {fingerprint}\n".encode("ascii"))
        f.write(f"SourceIdxNumRawPeptides: {num_raw_peptides}\n".encode("ascii"))
        f.write(f"SourceIdxPath: {args.idx_file}\n".encode("ascii"))
        f.write(f"MinRelativeIntensity: {args.min_relative_intensity}\n".encode("ascii"))
        f.write(f"MinKeptPeaks: {args.min_kept_peaks}\n".encode("ascii"))
        f.write(f"GeneralMode: {1 if header_is_general else 0}\n".encode("ascii"))
        f.write(f"VarModConfig: {var_mod_config}\n".encode("ascii"))
        f.write(b"\n")
        f.write(struct.pack("<Q", total))
        buf = []
        buf_bytes = 0
        for e in merge_sorted_runs(blobs):
            buf.append(e)
            buf_bytes += len(e)
            if buf_bytes >= (1 << 22):
                f.write(b"".join(buf))
                buf = []
                buf_bytes = 0
        if buf:
            f.write(b"".join(buf))

    n_verified = verify_written_mask_sorted(args.out_mask_file)
    print(f"Done: {args.out_mask_file!r}, {n_verified} entries (sorted+unique verified), "
          f"{time.time() - t0:.0f}s total, store had {store_rows} rows", file=sys.stderr)


if __name__ == "__main__":
    main()
