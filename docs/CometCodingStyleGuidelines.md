# Comet Coding Style Guidelines

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

## F. Variable naming — Systems Hungarian Notation

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
