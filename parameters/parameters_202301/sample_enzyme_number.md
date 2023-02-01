### Comet parameter: sample_enzyme_number

- Note that this parameter has no effect on the search and search
results at all. It is used solely to annotate additional information
in the output.
- This parameter is relevant only for pepXML output i.e. when
"[output_pepxmlfile](output_pepxmlfile.html)" is set to 1.
- The pepXML format encodes the enzyme that is applied to the sample
i.e. trypsin.  This enzyme is written to the "sample\_enzyme" element.
- The sample enzyme could be different from the search enzyme i.e.
the sample enzyme is "trypsin" yet the search enzyme is "Cut\_everwhere"
for a non-specific search.  Hence the need for this separate parameter.
- Valid values are any integer represented in the enzyme list.
- The default value is "0" if this parameter is missing.

Example:
```
sample_enzyme_number = 1
sample_enzyme_number = 3
```
