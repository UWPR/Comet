### Comet parameter: search_enzyme_number

- The search enzyme is specified by this parameter.  Note a [second enzyme](search_enzyme2_number.html)
can also be specified.
- The list of search enzymes is specified at the end of the comet.params file
beginning with the line [COMET_ENZYME_INFO].  The actual enzyme list and
digestion parameters are read from here in each search.  So one can edit/add/delete
enzyme definitions simply be changing the enzyme information.
- This parameter works in conjection with the [num_enzyme_termini](num_enzyme_termini.html)
parameter to define the cleavage rule for fully-digested vs. semi-digested search options.
- This parameter works in conjection with the [allowed_missed_cleavage](allowed_missed_cleavage.html)
parameter to define the miss cleavage rule.
- The default value is "1" if this parameter is missing.

Example:
```
search_enzyme_number = 0
search_enzyme_number = 1
```


The format of the parameter definition looks like the following:
```
COMET_ENZYME_INFO]
0.  Cut_everywhere         0      -           -
1.  Trypsin                1      KR          P
2.  Trypsin/P              1      KR          -
3.  Lys_C                  1      K           P
4.  Lys_N                  0      K           -
5.  Arg_C                  1      R           P
6.  Asp_N                  0      D           -
7.  CNBr                   1      M           -
8.  Glu_C                  1      DE          P
9.  PepsinA                1      FL          P
10. Chymotrypsin           1      FWYL        P
```

The first column of the parameter definition is the enzyme number. This number list
must start from 0 and sequentially increase by 1.  The second column is the enzyme name;
no spaces are allowed in this name field.  The third column is the digestion "sense"
i.e. a value of "0" specifies cleavage N-teriminal to (before) the specified residues
in column 4 and a value of "1" specifies cleavage C-terminal to (after) the specified
residues in column 4.  Column 4 contains the residue(s) that the enzyme cleaves at.
Column 5 contains the flanking residue(s) that negate cleavage.
