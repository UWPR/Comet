#!/usr/bin/env python3
"""
Compare rts_repro's search parameters with the RealtimeSearch example harness.

rts_repro.cpp's defaults follow RealtimeSearch/SearchMS1MS2.cs's
SearchSettings.ConfigureInputSettings() (the part every search runs, before the database
branch -- an existing index only has its header read there). That harness is an example
program whose settings change as RTS is tried in different configurations, so drift is
expected and is NOT an error: this script reports it, so a run of rts_repro can be made to
match a harness run (set the differing values with RTS_REPRO_PARAMS), or rts_repro's
defaults re-aligned. The unit tests never rely on either set of defaults (run_tests.py
passes RTS_TEST_PARAMS).

Usage:  python3 harness_param_drift.py [repo_root]
Prints one line per parameter set with a literal value by both programs that differs, and
the literal parameters only the harness sets. Exit status 0; 2 if either file cannot be
parsed (no parameters found -- the parser needs updating).
"""

import re
import sys
from pathlib import Path


def _num(s):
    try:
        return float(s)
    except ValueError:
        return None


def harness_params(cs_text):
    """{name: value string} for SetParam(name, sTmp, iTmp/dTmp) calls with a literal iTmp/dTmp,
    from the start of ConfigureInputSettings() up to its 'if (bDatabaseSearch)'."""
    start = cs_text.find("public bool ConfigureInputSettings(")
    end = cs_text.find("if (bDatabaseSearch)", start)
    if start < 0 or end < 0:
        return {}
    out, last = {}, {}
    for line in cs_text[start:end].splitlines():
        m = re.search(r"\b([id]Tmp)\s*=\s*([^;]+);", line)
        if m:
            last[m.group(1)] = m.group(2).split("//")[0].strip()
        m = re.search(r'SetParam\("([^"]+)",\s*sTmp,\s*([id]Tmp)\)', line)
        if m and _num(last.get(m.group(2), "x")) is not None:
            out[m.group(1)] = last[m.group(2)]
    return out


def rts_repro_params(cpp_text):
    """{name: value string} for the first unconditional-looking Set*Param(mgr, name, value) of each
    name; a variable value is resolved through its 'int x = (argc > N) ? atoi(argv[N]) : default;'."""
    defaults = {m.group(1): m.group(2) for m in
                re.finditer(r"\b(?:int|double)\s+(\w+)\s*=\s*\(argc > \d+\)\s*\?\s*ato[if]\(argv\[\d+\]\)\s*:\s*([-0-9.eE]+)\s*;", cpp_text)}
    out = {}
    for m in re.finditer(r'Set(?:Int|Double)Param\(mgr,\s*"([^"]+)",\s*([^)]+)\)', cpp_text):
        name, val = m.group(1), m.group(2).strip()
        if name in out:
            continue
        if _num(val) is None:
            val = defaults.get(val)
        if val is not None:
            out[name] = val
    return out


def compare(repo_root):
    root = Path(repo_root)
    cs = harness_params((root / "RealtimeSearch" / "SearchMS1MS2.cs").read_text(encoding="utf-8", errors="replace"))
    rr = rts_repro_params((root / "tests" / "rts_repro" / "rts_repro.cpp").read_text(encoding="utf-8", errors="replace"))
    differ = [(k, cs[k], rr[k]) for k in sorted(cs) if k in rr and _num(cs[k]) != _num(rr[k])]
    harness_only = [(k, cs[k]) for k in sorted(cs) if k not in rr]
    return cs, rr, differ, harness_only


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
    cs, rr, differ, harness_only = compare(root)
    if not cs or not rr:
        print(f"could not parse parameters (harness {len(cs)}, rts_repro {len(rr)}): update the parser")
        return 2
    print(f"{len(cs)} literal harness parameters, {len(rr)} rts_repro parameters compared")
    for k, a, b in differ:
        print(f"  differs: {k}: SearchMS1MS2.cs {a}, rts_repro {b}")
    for k, a in harness_only:
        print(f"  harness only: {k} = {a} (rts_repro leaves Comet's default)")
    if not differ:
        print("  no differing values")
    return 0


if __name__ == "__main__":
    sys.exit(main())
