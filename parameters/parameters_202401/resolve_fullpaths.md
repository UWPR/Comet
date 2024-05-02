### Comet parameter: resolve_fullpaths

- The input value is an integer that controls whether or not to resolve
the full paths of the input files as reported in the "base_name" attributes
for both the "msms_run_summary" and "search_summary" elements of the pepXML
output. This also affects the "summary_xml" attribute in the "msms_pipeline_analysis"
element.
- This parameter takes in one input value:
  - 0 = do not resolve the full paths
  - 1 = will resolve the full paths (default)

Example:
```
resolve_fullpaths = 0
resolve_fullpaths = 1
```
