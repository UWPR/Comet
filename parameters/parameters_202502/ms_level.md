### Comet parameter: ms_level

- This parameter specifies which scans are searched.
- An input value of 2 will search MS/MS scans.
- An input value of 3 will search MS3 scans.
- This parameter is only valid for mzXML, mzML, and mz5 input files.
- Allowed values are 2 or 3.
- The default value is "2" if this parameter is missing or any value other than 2 or 3 is entered.

Example:
```
ms_level = 2
ms_level = 3
```
