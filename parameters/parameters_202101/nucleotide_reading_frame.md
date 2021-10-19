### Comet parameter: nucleotide_reading_frame

- This parameter is used to search nucleotide sequence databases.
- It controls how the nucleotides are translated specifically
which sets of reading frames are translated.
- Valid values are 0 through 9.
- Set this parameter to 0 for a protein sequence database.
- Set this parameter to 1 to search the 1st forward reading frame.
- Set this parameter to 2 to search the 2nd forward reading frame.
- Set this parameter to 3 to search the 3rd forward reading frame.
- Set this parameter to 4 to search the 1st reverse reading frame.
- Set this parameter to 5 to search the 2nd reverse reading frame.
- Set this parameter to 6 to search the 3rd reverse reading frame.
- Set this parameter to 7 to search all 3 forward reading frames.
- Set this parameter to 8 to search all 3 reverse reading frames.
- Set this parameter to 9 to search all 6 reading frames.
- The default value is "0" if this parameter is missing.

Example:
```
nucleotide_reading_frame = 0
nucleotide_reading_frame = 9
```
