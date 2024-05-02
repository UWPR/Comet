### Comet parameter: peff_verbose_output

- Specifies whether the verbose output is reported during a PEFF search.  Examples of
the reporting includes not finding an entry in the OBO file, amino acid variant
same as original residue, invalid mod/variant amino acid position, etc.
- Valid values are 0 and 1.
- To suppress verbose output, set the value to 0.
- To show verbose output, set the value to 1.
- The default value is "0" if this parameter is missing.
- This is a hidden parameter that is not included in the parameters file generated
by "comet -p".  You must manually add this parameter if you want to set it.

Example:
```
peff_verbose_output = 1
```
