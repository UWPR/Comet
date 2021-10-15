### Comet parameter: old_mods_encoding

- This parameter enables using the old character based modification encodings
(e.g. DLYM*NCK) instead mass based encodings (e.g. DLYM[15.9949]NCK) in the SQT
output files.
- A value of "1" will cause Comet to use the old character based modification
encodings (e.g. DLYM*NCK).
- A value of "0" will cause Comet to use the mass based modification encodings
(e.g. DLYM[15.9949]NCK).
- This parameter affects SQT output files only.
- The default value is "0" if this parameter is missing.

Example:
```
old_mods_encoding = 1
```
