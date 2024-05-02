### Comet parameter: export_additional_pepxml_scores

- Controls whether to output additional search scores in the [pep.xml output](https://uwpr.github.io/Comet/parameters/parameters_202301/output_pepxmlfile.html).
- Valid values are 0 (do not output) or 1 (output).
- The default value is "0" if this parameter is missing.
- This is a optional/hidden parameter in that it doesn't appear by default in comet.params files.
- The additional search scores reported are:
  - lnrSp:  natural log of the Sp rank
  - deltLCn:  deltaCn value of the last reported peptide in the output list
  - lnExpect:  natural log of the expectation value
  - IonFrac:  decimal value representing (# matched fragment ions / # total fragment ions)

Example:
```
export_additional_pepxml_scores = 0
export_additional_pepxml_scores = 1
```
