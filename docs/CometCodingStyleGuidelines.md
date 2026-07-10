# Comet Coding Style Guidelines

**Scope:** These conventions govern the native C++ core in `CometSearch/`
(`CometSearch.cpp`, `CometPreprocess.cpp`, `core/*.h`, etc.) -- new code there,
including edits to existing files, should follow all of A-F below.

Two documented exceptions, not violations to "fix" if you encounter them:
- `MSToolkit/` is adapted third-party source (Mike Hoopmann's library, with a
  `/clr`-based Thermo RawFileReader integration layered on top per
  `docs/20260618_RawFileReaderMigration.md`); files there such as
  `MSToolkit/src/MSToolkit/RAWReader.cpp` follow K&R brace style and tab/2-space
  indentation rather than A/B below, matching the surrounding third-party code
  rather than this guide.
- The newer OOP architecture layer (`CometSearch/search/*.h`, `CometSearch/output/*.h`
  -- `Pipeline`, `ISearchStrategy`, `SearchSession`, `IResultWriter`, etc.,
  introduced by the Phase 1-5 migration in `docs/20260612_architecture_migration.md`)
  uses a `_member` leading-underscore convention for private fields instead of
  Hungarian prefixes, and generally drops Hungarian prefixes from parameters of
  "modern" types (`SearchSession& session`, `ThreadPool* tp` with no `p`,
  `std::vector<...> writers` with no `v`). Follow the convention already used
  in whichever file you're editing rather than importing Hungarian notation
  into that layer, or vice versa.

## A. Brace style

Use Allman brace style (credited to Eric Allman). Opening braces are placed on their own line, at the same indentation level as the control structure.

```cpp
if (condition_variable == condition1)
{
   // Some code.
   condition_variable = condition2;
}
else if (condition_variable == condition2)
{
   // Some code.
   condition_variable = condition1;
}
else
{
   // Some code.
   condition_variable = false;
}
```

## B. Indentation

Use spaces for indentation, not tab characters. The standard indent is **3 spaces** per logical level.

## C. Trailing whitespace

Avoid trailing whitespace on any line.

## D. Line endings

Use Windows-style carriage returns (`\r\n`) rather than Unix-style (`\n`).

## E. Comments

Use `//` for inline comments. This reserves `/* */` for commenting out blocks of code (including lines that already contain `//` comments).
Use ASCII characters only in comments.

## F. Variable naming -- Systems Hungarian Notation

Prefix variable names with a lowercase type tag:

| Prefix | Type |
|--------|------|
| `i` | `int` |
| `l` | `long` |
| `f` | `float` |
| `d` | `double` |
| `b` | `bool` |
| `c` | `char` (single character) |
| `sz` | null-terminated `char[]` string |
| `p` | pointer (combined with type prefix, e.g. `pf`, `pd`) |
| `v` | `vector` |
| `g_` | global variable |

Examples: `iCount`, `dMass`, `szName`, `bFlag`, `pdAAMass`, `g_staticParams`.
