### Comet parameter: scan_range

- Defines the scan range to search.  Only spectra within (and inclusive) of the specified
scan range are searched.
- This parameter works with MS2, mzXML and mzML inputs files.
- Two digits are specified for this parameter.  The first digit is the start scan and the
second digit is the end scan.
- You can set either just the start scan (leaving end scan 0) or just the end scan
(leaving start scan 0).  
- When the end scan is less than the start scan, no scan can satisfy that scan range
so no spectra will be searched.
- The default value is "0 0" if this parameter is missing. The entire file will be searched
with a "0 0" scan setting.
- Any time a non-zero value is specified for either the start scan or the end scan, the
output files will have the scan range encoded in the output file name.

Example:
```
scan_range = 0 0
scan_range = 0 1000
scan_range = 2000 0
scan_range = 1000 1500
```
