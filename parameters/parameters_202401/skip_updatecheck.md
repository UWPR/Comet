### Comet parameter: skip_updatecheck

- Comet will check if there is an updated version available and report if so. This
also triggers a Comet Google analytics hit.
- When set to 1, the update check will not be performed.
- Valid values are 0 and 1.
- The default value is "0" if this parameter is missing.

Example:
```
skip_updatecheck = 1
```

If an update is available, you will see "**UPDATE AVAILABLE**" after the version string when a search a run:

```
Comet version "2018.01 rev. 0"  **UPDATE AVAILABLE**

Search start:  05/08/2018, 06:45:31 AM
- Input file: JE102306_102306_18Mix4_Tube1_01.mzXML
  - Load spectra: 5164
    - Search progress: 100%
    - Post analysis:  done
Search end:    05/08/2018, 06:47:24 AM, 1m:53s</pre>
```
