### Comet parameter: skip_researching

**Note that this parameter has been deprecated in version 2018.01 rev. 2.**

- This parameter is valid only when
         [output_outfiles](output_outfiles.html),
is set to 1 and each of
         [output_pepxmlfile](output_pepxmlfile.html),
         [output_sqtfile](output_sqtfile.html), and
         [output_sqtstream](output_sqtstream.html), are set to 0.
- When .out files only are set to be exported, this parameter will look to see if
an .out file already exists for each query spectrum.  If so, it will not re-search
that particular spectrum.
- When set to 0, all spectra are re-searched.  When set to 1, the search is skipped
for those spectra where an .out file already exists.
- Valid values are 0 and 1.
- The default value is "1" if this parameter is missing.

Example:
```
skip_researching = 0
skip_researching = 1
```
