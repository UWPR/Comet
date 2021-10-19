### Comet parameter: output_suffix

- This parameter specifies the suffix string that is appended to
the base output name for the pep.xml, pin.xml, txt and sqt output files.
- Use this parameter to give output files a unique suffix base name.
- For example, if the output_suffix parameter is set to
"output_suffix = _000", then a search of the file base.mzXML
will generate output files named base_000.pep.xml, base_000.pin.xml,
base_000.txt, and/or base_000.sqt.
- Note that using this parameter could break downstream tools that
expect the output base name to be the same as the input file base name.
- The default value is blank if this parameter is missing i.e.
base.mzXML will generate base.pep.xml.

Example:
```
output_suffix =
output_suffix = _some_suffix
output_suffix = any_string_you_want_without_spaces
```
