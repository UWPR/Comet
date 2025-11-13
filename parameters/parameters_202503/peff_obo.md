### Comet parameter: peff_obo

- A full or relative path to the OBO file used with a PEFF search.
- Supported OBO formats are PSI-Mod and Unimod OBO files.  Which OBO file you
use depends on your PEFF input file.
- This parameter is ignored if "[peff_format = 0](peff_format.html)".
- There is no default value if this parameter is missing.

Example:
```
peff_obo = PSI-MOD.obo
peff_obo = C:\local\obo\PSI-MOD.obo)
peff_obo = /usr/local/obo/PSI-MOD.obo
```
