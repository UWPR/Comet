#pragma once

// Plain native (no /clr) translation unit, compiled in the same project/binary as the
// /clr-enabled Main.cpp, to validate that MSVC's per-file CompileAsManaged override lets
// managed and unmanaged .cpp files coexist in one project -- the same combination the real
// RAWReader.cpp rewrite will need inside MSToolkit (see Phase 0 item 3,
// docs/20260618_RawFileReaderMigration.md).

double NativeSumBuffer(const double* data, size_t n);
