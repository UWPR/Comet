### Comet parameter: clip_nterm_methionine

- This parameter controls whether Comet will automatically remove
the N-terminal methionine from a sequence entry.
- If set to 0, the sequence is analyzed as-is.
- If set to 1, any sequence with an N-terminal methionine will be
analyzed as-is as well as with the methionine removed.  This means
that any N-terminal modifications will also apply (if appropriate)
to the peptide that is generated after the removal of the methionine.
- Valid values are 0 and 1.
- The default value is "0" if this parameter is missing.

Example:
```
clip_nterm_methionine = 0
clip_nterm_methionine = 1
```
