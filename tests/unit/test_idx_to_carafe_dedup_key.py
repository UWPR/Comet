#!/usr/bin/env python3
"""
Correctness unit test for tools/idx_to_carafe.py's dedup_key() -- the 2026-08-14 memory fix
that replaced main()'s `seen` dict's raw (sequence, mods_str, mod_sites_str, charge) 4-tuple
key with a compact 128-bit BLAKE2b hash (see that function's own docstring for the full
rationale: at real phospho+oxMet scale, ~125M unique rows, the raw-tuple dict was on track for
~33GB, over this box's 31GB WSL memory ceiling).

Pure Python, no comet.exe/.idx dependency -- exercises dedup_key() directly against
hand-constructed inputs, per this project's established pattern (see
test_carafe_ms2_to_fi_mask.py) of a dedicated correctness unit test before any memory/perf fix
touches real data.

Does NOT substitute for the real-data verification: a full rerun of the already-completed
oxMet charge2+3+decoys conversion, diffed byte-for-byte against the known-good pre-fix output
(docs/20260805_carafe.md's memory-fix section documents that result). This test only proves
dedup_key()'s hashing behavior is sound in isolation -- correct field separation, determinism,
and content-based (not identity-based) equality.
"""

import os
import sys
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent.parent.parent / "tools"
sys.path.insert(0, str(TOOLS_DIR))
import idx_to_carafe as itc  # noqa: E402


def check(cond, msg, failures):
    if not cond:
        failures.append(msg)
    return cond


def test_deterministic_across_calls(failures):
    """Same inputs must always produce the same key -- dedup correctness depends entirely on
    this (unlike Python's built-in hash(), BLAKE2b is not process-randomized)."""
    k1 = itc.dedup_key("PEPTIDEK", "Phospho@S", "3", 2)
    k2 = itc.dedup_key("PEPTIDEK", "Phospho@S", "3", 2)
    check(k1 == k2, f"dedup_key not deterministic: {k1} != {k2}", failures)


def test_returns_plain_int_in_128_bit_range(failures):
    """Must be a plain int (cheapest hashable CPython object for a dict key -- module
    docstring's rationale), within [0, 2**128)."""
    k = itc.dedup_key("AAAK", "", "", 2)
    check(isinstance(k, int), f"dedup_key should return int, got {type(k)}", failures)
    check(0 <= k < (1 << 128), f"dedup_key out of 128-bit range: {k}", failures)


def test_content_equality_not_identity(failures):
    """Two calls with equal but NOT object-identical strings (e.g. one built via concatenation,
    the other a literal) must produce the same key -- dedup_key hashes CONTENT, matching the
    semantics of the original raw-tuple dict key it replaced (tuple/string equality is also
    content-based, not identity-based) -- this is the property that makes the fix a safe
    drop-in replacement."""
    seq_a = "PEPTIDEK"
    seq_b = "PEPTIDE" + "K"   # same content, different str object
    check(seq_a is not seq_b or True, "sanity: test setup", failures)  # don't assert identity;
    # CPython may or may not intern these short literals -- irrelevant, just confirms the test
    # isn't accidentally relying on object identity either way.
    k1 = itc.dedup_key(seq_a, "Oxidation@M", "4", 3)
    k2 = itc.dedup_key(seq_b, "Oxidation@M", "4", 3)
    check(k1 == k2, "dedup_key must depend on string CONTENT, not object identity", failures)


def test_different_inputs_produce_different_keys(failures):
    """Spot-check that varying each of the four fields independently changes the key -- catches
    a construction bug where one field is accidentally ignored."""
    base = itc.dedup_key("PEPTIDEK", "Phospho@S", "3", 2)
    variants = [
        itc.dedup_key("PEPTIDER", "Phospho@S", "3", 2),   # different sequence
        itc.dedup_key("PEPTIDEK", "Phospho@T", "3", 2),   # different mods_str
        itc.dedup_key("PEPTIDEK", "Phospho@S", "5", 2),   # different mod_sites_str
        itc.dedup_key("PEPTIDEK", "Phospho@S", "3", 3),   # different charge
    ]
    for i, v in enumerate(variants):
        check(v != base, f"variant {i} collided with base key (should differ)", failures)
    # And pairwise among the variants themselves, cheaply (not exhaustive, just a sanity net).
    check(len(set(variants + [base])) == 5, "expected 5 distinct keys among base + 4 variants",
          failures)


def test_field_boundary_not_ambiguous(failures):
    """The classic hashing-key-construction bug: without an unambiguous separator, two
    different (field-split) inputs could concatenate to the same byte string, e.g.
    seq='AB',mods='C' vs seq='A',mods='BC'. dedup_key() must use a real separator (module
    docstring: b'\\x00' between fields) so this can't happen -- verified here directly rather
    than just trusting the implementation's own comment."""
    k1 = itc.dedup_key("AB", "C", "", 2)
    k2 = itc.dedup_key("A", "BC", "", 2)
    check(k1 != k2, "field-boundary ambiguity: 'AB'+'C' collided with 'A'+'BC'", failures)

    k3 = itc.dedup_key("A", "B", "C", 2)
    k4 = itc.dedup_key("A", "BC", "", 2)
    check(k3 != k4, "field-boundary ambiguity across mods_str/mod_sites_str split", failures)


def test_empty_strings_handled(failures):
    """Unmodified peptides route through here with mods_str == mod_sites_str == '' -- must not
    raise or collide with a superficially similar non-empty case."""
    k_empty = itc.dedup_key("PEPTIDEK", "", "", 2)
    k_nonempty = itc.dedup_key("PEPTIDEK", "Oxidation@M", "4", 2)
    check(isinstance(k_empty, int), "empty mods/sites should still produce a valid int key",
          failures)
    check(k_empty != k_nonempty, "empty-mod and modified variants must not collide", failures)


TESTS = [
    test_deterministic_across_calls,
    test_returns_plain_int_in_128_bit_range,
    test_content_equality_not_identity,
    test_different_inputs_produce_different_keys,
    test_field_boundary_not_ambiguous,
    test_empty_strings_handled,
]


def run_test():
    all_failures = []
    for test_fn in TESTS:
        failures = []
        test_fn(failures)
        status = "PASS" if not failures else "FAIL"
        print(f"  [{status}] {test_fn.__name__}")
        for f in failures:
            print(f"         - {f}")
        all_failures.extend(failures)

    if all_failures:
        print(f"\n{len(all_failures)} failure(s)")
        return False
    print("\nPASS")
    return True


if __name__ == "__main__":
    ok = run_test()
    sys.exit(0 if ok else 1)
