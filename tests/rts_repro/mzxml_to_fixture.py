#!/usr/bin/env python3
"""
Convert the MS2 scans of an mzXML file into rts_repro's SPECTRUM fixture format.

Companion to ms2_to_fixture.py for the real high-resolution data this repo has only as
mzXML on Linux (e.g. 20170103_HelaQC_01.mzXML, the benchmark file): comet.exe cannot write
.ms2 and msconvert is not available here. Each MS2 scan becomes one block

    SPECTRUM <scan> <precursorCharge> <precursorMz> <numPeaks>
    <m/z> <intensity>
    ...

Scans without a precursorCharge attribute get charge 0 (rts_repro's mode 0 then reports
NO_MATCH for them; the chimeric modes ignore the charge). Peaks are decoded from the
<peaks> element (base64, network byte order, 32- or 64-bit, optional zlib).

Usage:
    python3 mzxml_to_fixture.py input.mzXML [--max N] [--first-scan S] [--last-scan E] > fixture.txt
"""

import argparse
import base64
import struct
import sys
import xml.etree.ElementTree as ET
import zlib


def _local(tag):
    return tag.rsplit("}", 1)[-1]


def decode_peaks(elem):
    text = (elem.text or "").strip()
    if not text:
        return []
    raw = base64.b64decode(text)
    if elem.get("compressionType", "none").lower() == "zlib":
        raw = zlib.decompress(raw)
    precision = int(elem.get("precision", "32"))
    fmt = ">%d%s" % (len(raw) // (precision // 8), "f" if precision == 32 else "d")
    vals = struct.unpack(fmt, raw)
    if elem.get("pairOrder", "m/z-int") != "m/z-int":
        raise SystemExit("unsupported pairOrder %r" % elem.get("pairOrder"))
    return list(zip(vals[0::2], vals[1::2]))


def convert(path, out, max_scans=0, first_scan=0, last_scan=0):
    n = 0
    for event, elem in ET.iterparse(path, events=("end",)):
        if _local(elem.tag) != "scan":
            continue
        try:
            if elem.get("msLevel") != "2":
                continue
            scan = int(elem.get("num"))
            if (first_scan and scan < first_scan) or (last_scan and scan > last_scan):
                continue
            mz = 0.0
            charge = 0
            peaks = []
            for child in elem:
                t = _local(child.tag)
                if t == "precursorMz":
                    mz = float((child.text or "0").strip())
                    charge = int(child.get("precursorCharge", "0"))
                elif t == "peaks":
                    peaks = decode_peaks(child)
            peaks = [(m, i) for m, i in peaks if i > 0.0]
            if not peaks:
                continue
            out.write("SPECTRUM %d %d %.6f %d\n" % (scan, charge, mz, len(peaks)))
            for m, i in peaks:
                out.write("%.5f %.2f\n" % (m, i))
            n += 1
            if max_scans and n >= max_scans:
                break
        finally:
            elem.clear()   # an MS2 scan is nested inside its MS1 scan in mzXML; keep memory flat
    return n


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("mzxml")
    ap.add_argument("--max", type=int, default=0, help="stop after N MS2 scans (0 = all)")
    ap.add_argument("--first-scan", type=int, default=0)
    ap.add_argument("--last-scan", type=int, default=0)
    args = ap.parse_args()
    n = convert(args.mzxml, sys.stdout, args.max, args.first_scan, args.last_scan)
    print("wrote %d MS2 scans" % n, file=sys.stderr)


if __name__ == "__main__":
    main()
