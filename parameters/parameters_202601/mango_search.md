### Comet parameter: mango_search

- This parameter controls custom internal behavior for the [Mango cross-link searches](https://pubmed.ncbi.nlm.nih.gov/29676898/). Utility is specific to the [Bruce lab](https://brucelab.gs.washington.edu) at the University of Washington.
- When set to "1", Comet will export additional custom output in the pep.xml and txt formats that are used to track and report Mango precursor pairs.
- The default value is "0" if this parameter is missing.

Example:
```
mango_search = 1
```
