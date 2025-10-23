### Comet parameter: protein_modslist_file

- A full or relative path to a protein modifications file.
- If this entry is blank, aka no file is specified, then this functionality is ignored.
- If this entry points to a modifications file, Comet will parse the modification numbers and protein
  strings from the file and limit the application of the specified variable modifications to the
  sequence entries that match the protein string.
- The default value is a blank string if this parameter entry is missing.
- The protein modifications file is a text file composed of one or more lines.
  - Each line contains a modification number and a string.
  - The modification number corresponds to the variable_modXX entry, e.g. "2" will apply "variable_mod02" to the matched protein(s).
  - The input string (no spaces) will be matched to the first "word" or accession in the protein description line.
  - If the input string is a substring of the full accession word, it will be considered a match.  For example, if
    the input string is "HUMAN" then any accession containing "HUMAN" anywhere will be a match.
  - No wildcards or regular expressions are currently supported.
  - The string matches are case sensitive.

Example:
```
protein_modslist_file = myproteinmodslist
protein_modslist_file = C:\local\myproteinmods.txt
protein_modslist_file = /usr/local/proteinmods.file
```

Example contents of the protein modifications file.  In this example, varible_mod02 will only be applied to HLAA_HUMAN
through HLAH_HUMAN and variable_mod03 will only be applied to HLAA_HUMAN and HLAG_HUMAN.  If variable_mod01 were specified
in the search parameters, it is not restricted to any subset of proteins and will apply to all proteins including the
proteins listed in this modifications file.
```
2  HLAA_HUMAN
2  HLAB_HUMAN
2  HLAC_HUMAN
2  HLAE_HUMAN
2  HLAF_HUMAN
2  HLAG_HUMAN
2  HLAH_HUMAN
3  HLAA_HUMAN
3  HLAG_HUMAN
```
