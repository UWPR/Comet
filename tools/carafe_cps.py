#!/usr/bin/env python3
"""
Compact prediction store (.cps) for Carafe MS2 predictions -- docs/20260822_carafe_prerun.md
Section 5 (milestone M2). Stores, per tools/idx_to_carafe.py out_tsv row, exactly what
tools/carafe_ms2_to_fi_mask.py's mask computation consumes and nothing else:

  - nAA (peptide length)
  - base_peak over all 8 predicted channels (f32, EXACT -- FragmentTable already stores
    float32, and the max of a set of f32 values is itself an f32 value, so no precision is
    lost storing it; this is the "with modloss" threshold reference)
  - base_peak over the first 4 channels only (f32, exact; the --ignore-modloss /
    general-mode threshold reference -- it includes b_z2/y_z2, which are NOT stored
    per-position below, so it cannot be recomputed and must be stored)
  - per cleavage-site row (nAA-1 of them, AlphaBase order), the 4 z1 channel intensities the
    mask candidates are actually built from (b_z1, y_z1, b_modloss_z1, y_modloss_z1),
    quantized relative to the all-8 base_peak (u8 or u16 -- header-declared; every stored
    value is <= that base peak by construction, so quantization never saturates)

Keyed by out_tsv row_index (0-based data-row position), the same key idx_to_carafe.py's
variant-map sidecar uses -- so the store is .idx-flavor-neutral: one store built from a
phospho-mode (withNL) Carafe run serves BOTH the withNL mask build and the
--ignore-modloss noNL mask build; each .idx flavor's own variant map supplies its
(iWhichPeptide, modNumIdx, cNtermMod, cCtermMod) tuples and VarModConfig. Carafe's internal
sort_values('nAA') reordering is resolved ONCE, at translation time (the same content-tuple
join carafe_ms2_to_fi_mask.py does), not at every mask build.

File layout (little-endian throughout):

  magic line        b"Comet Carafe CPS v1\\n"
  header lines      "Key: Value\\n" ASCII, terminated by one blank line. Required keys:
                      SourceOutTsvRows      (data-row count of the source out_tsv)
                      SourceOutTsvHeadCRC32 (zlib.crc32 of the source out_tsv's first
                                             65536 bytes, hex -- cheap mispairing guard,
                                             NOT a full-content hash)
                      Quant                 (u8 | u16)
                      Mode                  (phospho | general -- whether modloss channels
                                             carried real data in the source prediction)
  u64               row_count (== SourceOutTsvRows; both present so a truncated header or a
                    header/payload mismatch is loudly detectable)
  directory         row_count x u64 -- absolute file offset of each row's payload
  payloads          per row: nAA(u8) base8(f32) base4(f32) then (nAA-1) rows x 4 channels
                    (b_z1, y_z1, b_modloss_z1, y_modloss_z1) x (u8|u16) quantized

Quantization: q = round(v / base8 * QMAX), clamped to [0, QMAX]; dequantized as
q / QMAX * base8. base8 == 0.0 stores all-zero q (and dequantizes to 0.0). The
quantization-granularity decision (u8 vs u16) is empirical -- see the M2 rebuild-diff
experiment in docs/20260822_carafe_prerun.md Section 5.4 -- so both are implemented and the
header declares which one a given store used.

compute_variant_mask_from_cps() below replicates tools/carafe_ms2_to_fi_mask.py's
compute_variant_mask() decision logic EXACTLY (same threshold comparison direction, same
candidate construction order, same stable sort, same floor top-up, via that module's own
_threshold_and_floor_pool/_pack_mask helpers) over dequantized values -- the only source of
divergence from a TSV-built mask is quantization itself, which is precisely what the M2
experiment measures.
"""

import array
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import carafe_ms2_to_fi_mask as fi_mask  # noqa: E402

CPS_MAGIC = b"Comet Carafe CPS v1\n"
QUANT_PARAMS = {"u8": ("B", 255), "u16": ("H", 65535)}
HEAD_CRC_BYTES = 65536


def out_tsv_head_crc32(path):
    """zlib.crc32 (hex string) of the first HEAD_CRC_BYTES of the out_tsv -- a cheap
    "did you pair this store with a different population's files" guard, deliberately not a
    full-content hash (a full pass over a 9.5GB file per open is not worth it; the row-count
    check catches truncation/extension, and the variant map -> mask -> .idx chain already
    carries the strong idx-binding guarantees)."""
    with open(path, "rb") as f:
        return format(zlib.crc32(f.read(HEAD_CRC_BYTES)) & 0xFFFFFFFF, "08x")


def quantize_row_values(values, base8, qmax):
    """values: iterable of floats (each <= base8 when base8 > 0). Returns list of ints."""
    if base8 <= 0.0:
        return [0] * len(values)
    scale = qmax / base8
    out = []
    for v in values:
        q = int(v * scale + 0.5)
        if q > qmax:
            q = qmax
        elif q < 0:
            q = 0
        out.append(q)
    return out


def pack_row(nAA, base8, base4, quantized_rows, qchar):
    """Serialize one row's payload -- the single definition both CpsWriter.append_row() and
    any out-of-process packer (carafe_pred_to_cps.py's workers) share, so worker-packed
    bytes are definitionally identical to parent-packed ones."""
    n_pos = nAA - 1
    if len(quantized_rows) != n_pos:
        raise ValueError(f"nAA={nAA} implies {n_pos} rows, got {len(quantized_rows)}")
    flat = [q for row in quantized_rows for q in row]
    return struct.pack("<Bff", nAA, base8, base4) + struct.pack(f"<{len(flat)}{qchar}", *flat)


class CpsWriter:
    """Two-phase writer: payloads stream to <path>.payload.tmp while offsets accumulate in
    memory (8 bytes/row -- ~1GB at 124.8M rows, accepted); finalize() writes the real file
    (magic+header+count+directory) and appends the payload tmp, then deletes it. Rows MUST be
    appended in row_index order; append_row() returns the row's payload size for callers that
    track progress."""

    def __init__(self, path, source_rows, source_head_crc, quant, mode):
        if quant not in QUANT_PARAMS:
            raise ValueError(f"quant must be one of {sorted(QUANT_PARAMS)}, got {quant!r}")
        self.path = str(path)
        self.tmp_path = self.path + ".payload.tmp"
        self.source_rows = int(source_rows)
        self.source_head_crc = source_head_crc
        self.quant = quant
        self.mode = mode
        self._qchar, self.qmax = QUANT_PARAMS[quant]
        # array('Q'), not a Python list: at 124.8M rows a list of int objects costs ~4GB of
        # parent RSS for what is 1GB of actual u64 data.
        self._offsets = array.array("Q")
        self._payload_pos = 0
        self._tmp = open(self.tmp_path, "wb")

    def append_row(self, nAA, base8, base4, quantized_rows):
        """quantized_rows: (nAA-1) sequences of 4 ints (b_z1, y_z1, b_ml_z1, y_ml_z1)."""
        blob = pack_row(nAA, base8, base4, quantized_rows, self._qchar)
        self._offsets.append(self._payload_pos)
        self._tmp.write(blob)
        self._payload_pos += len(blob)
        return len(blob)

    def append_packed(self, payload_blob, row_sizes):
        """Bulk-append rows already serialized by pack_row() elsewhere (e.g. a worker
        process -- carafe_pred_to_cps.py packs per-chunk in its pool workers precisely so
        the parent never holds a chunk's rows as Python object graphs; the first full-scale
        translation attempt buffered exactly those and hit ~44GB parent RSS in minutes).
        row_sizes: iterable of per-row byte sizes, in row order, summing to
        len(payload_blob)."""
        total = 0
        for size in row_sizes:
            self._offsets.append(self._payload_pos + total)
            total += size
        if total != len(payload_blob):
            raise ValueError(f"row_sizes sum {total} != payload blob length {len(payload_blob)}")
        self._tmp.write(payload_blob)
        self._payload_pos += total
        return total

    def finalize(self):
        self._tmp.close()
        n = len(self._offsets)
        if n != self.source_rows:
            raise ValueError(
                f"wrote {n} rows but header says SourceOutTsvRows={self.source_rows} -- "
                f"refusing to finalize a store that silently disagrees with its own header")
        header = (
            CPS_MAGIC
            + f"SourceOutTsvRows: {self.source_rows}\n".encode("ascii")
            + f"SourceOutTsvHeadCRC32: {self.source_head_crc}\n".encode("ascii")
            + f"Quant: {self.quant}\n".encode("ascii")
            + f"Mode: {self.mode}\n".encode("ascii")
            + b"\n"
        )
        payload_base = len(header) + 8 + 8 * n
        with open(self.path, "wb") as out:
            out.write(header)
            out.write(struct.pack("<Q", n))
            out.write(struct.pack(f"<{n}Q", *(payload_base + o for o in self._offsets)))
            with open(self.tmp_path, "rb") as tmp:
                while True:
                    buf = tmp.read(1 << 24)
                    if not buf:
                        break
                    out.write(buf)
        Path(self.tmp_path).unlink()


class CpsReader:
    """Sequential or random-access reader. Header parsed eagerly; the directory is loaded
    as one array-backed struct (8B/row); payloads read on demand."""

    def __init__(self, path):
        self.path = str(path)
        self._f = open(self.path, "rb")
        magic = self._f.readline()
        if magic != CPS_MAGIC:
            raise ValueError(f"{path!r}: not a {CPS_MAGIC!r} store (got {magic!r})")
        self.header = {}
        while True:
            line = self._f.readline()
            if not line or line == b"\n":
                break
            k, _, v = line.decode("ascii").rstrip("\n").partition(": ")
            self.header[k] = v
        (self.row_count,) = struct.unpack("<Q", self._f.read(8))
        if int(self.header.get("SourceOutTsvRows", "-1")) != self.row_count:
            raise ValueError(
                f"{path!r}: header SourceOutTsvRows={self.header.get('SourceOutTsvRows')} "
                f"!= binary row_count={self.row_count} -- truncated or corrupt store")
        self.quant = self.header["Quant"]
        if self.quant not in QUANT_PARAMS:
            raise ValueError(f"{path!r}: unknown Quant {self.quant!r}")
        self._qchar, self.qmax = QUANT_PARAMS[self.quant]
        self._qsize = struct.calcsize(self._qchar)
        self._offsets = array.array("Q")
        self._offsets.frombytes(self._f.read(8 * self.row_count))
        if sys.byteorder != "little":
            self._offsets.byteswap()

    def verify_source(self, out_tsv_path, n_data_rows=None):
        """Raise if this store wasn't built from (a file identical in its first 64KB to)
        out_tsv_path; optionally also check the caller's known data-row count."""
        crc = out_tsv_head_crc32(out_tsv_path)
        if crc != self.header["SourceOutTsvHeadCRC32"]:
            raise ValueError(
                f"{self.path!r} was not built from {out_tsv_path!r}: head CRC {crc} != "
                f"stored {self.header['SourceOutTsvHeadCRC32']}")
        if n_data_rows is not None and n_data_rows != self.row_count:
            raise ValueError(
                f"{self.path!r} row_count {self.row_count} != caller's out_tsv data-row "
                f"count {n_data_rows}")

    def read_row(self, row_index):
        """Returns (nAA, base8, base4, rows4) where rows4 is a list of (nAA-1) 4-tuples of
        DEQUANTIZED floats (b_z1, y_z1, b_modloss_z1, y_modloss_z1)."""
        if not (0 <= row_index < self.row_count):
            raise IndexError(row_index)
        self._f.seek(self._offsets[row_index])
        nAA, base8, base4 = struct.unpack("<Bff", self._f.read(9))
        n_pos = nAA - 1
        flat = struct.unpack(f"<{n_pos * 4}{self._qchar}",
                              self._f.read(n_pos * 4 * self._qsize))
        if base8 > 0.0:
            inv = base8 / self.qmax
            vals = [q * inv for q in flat]
        else:
            vals = [0.0] * len(flat)
        rows4 = [tuple(vals[i * 4:(i + 1) * 4]) for i in range(n_pos)]
        return nAA, base8, base4, rows4

    def close(self):
        self._f.close()


def compute_variant_mask_from_cps(rows4_per_charge, nAA, base8_per_charge, base4_per_charge,
                                    min_relative_intensity, min_kept_peaks,
                                    has_modloss, is_modified):
    """The .cps counterpart of carafe_ms2_to_fi_mask.compute_variant_mask(), with identical
    decision logic (same helpers, same candidate construction/iteration order, same stable
    sort and floor) over dequantized values. rows4_per_charge: one rows4 list per charge
    state sharing this variant (cross-charge max taken here, mirroring
    max_across_charges()); base peaks likewise maxed across charges. has_modloss=False is
    the --ignore-modloss / general-mode path: threshold reference is the FIRST-4-channel
    base peak and the modloss pool is skipped."""
    n_pos = nAA - 1
    for rows4 in rows4_per_charge:
        if len(rows4) != n_pos:
            raise ValueError(f"nAA={nAA} implies {n_pos} rows, got {len(rows4)}")

    merged = rows4_per_charge[0]
    if len(rows4_per_charge) > 1:
        merged = [tuple(max(vals) for vals in zip(*rows_at_r))
                  for rows_at_r in zip(*rows4_per_charge)]

    base_peak = max(base8_per_charge) if has_modloss else max(base4_per_charge)
    threshold = min_relative_intensity * base_peak

    b_by_length = {}
    y_by_length = {}
    b_modloss_by_length = {}
    y_modloss_by_length = {}
    for r, (b_z1, y_z1, b_ml_z1, y_ml_z1) in enumerate(merged):
        b_by_length[r + 1] = b_z1
        y_by_length[nAA - r - 1] = y_z1
        b_modloss_by_length[r + 1] = b_ml_z1
        y_modloss_by_length[nAA - r - 1] = y_ml_z1

    def build_candidates(by_length_b, by_length_y):
        candidates = []
        for length, intensity in by_length_b.items():
            if length >= fi_mask.MIN_ION_LENGTH:
                candidates.append((intensity, "b", length))
        for length, intensity in by_length_y.items():
            if length >= fi_mask.MIN_ION_LENGTH:
                candidates.append((intensity, "y", length))
        return candidates

    unshifted = build_candidates(b_by_length, y_by_length)
    kept, n_candidates = fi_mask._threshold_and_floor_pool(unshifted, threshold, min_kept_peaks)
    bMask, yMask = fi_mask._pack_mask(kept)

    if has_modloss and is_modified:
        ml = build_candidates(b_modloss_by_length, y_modloss_by_length)
        ml_kept, n_ml_candidates = fi_mask._threshold_and_floor_pool(ml, threshold, min_kept_peaks)
        bModlossMask, yModlossMask = fi_mask._pack_mask(ml_kept)
        n_ml_kept = len(ml_kept)
    else:
        bModlossMask = yModlossMask = 0
        n_ml_candidates = n_ml_kept = 0

    return (bMask, yMask, bModlossMask, yModlossMask,
            n_candidates, len(kept), n_ml_candidates, n_ml_kept, base_peak)
