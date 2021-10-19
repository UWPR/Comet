### Comet parameter: variable_mod01 through variable_mod09

There are 8 entries/settings that are associated with these parameters:
  - The first entry is a decimal value specifying the modification mass difference.
  - The second entry is the residue(s) that the modifications are possibly applied to.
    If more than a single residue is modified by the same mass difference, list them
    all as a string.  Use 'n' for N-terminal modfication and 'c' for C-terminal modification.
  - The third entry is a integer to specify whether the modification is a
    variable modification (0) or a binary modification (non-zero value).
    - 0 = variable modification analyzes all permutations of modified and unmodified residues.
    - non-zero value = A binary modification analyzes peptides where all residues are either modified or all residues are not modified.
    Note that if you set the same binary modification value in multiple "variable_mod?" parameter
    entries, Comet will treat those variable modifications as a "binary set".  This means
    that all modifiable residues in the "binary set" must be unmodified or modified.  Multiple
    "binary sets" can be specified by setting a different binary modification value e.g.
    use "1" for all modifications in set 1, and "2" or all modifications in set 2.  Binary groups were added with version 2015.02 rev. 1.
  - The fourth entry is an integer specifying the maximum number of modified residues
    possible in a peptide for this modification entry. With release 2020.01 rev. 3, this
    field has been extended to allow specifying both a mininum and maximum number of
    modified residues for this modification entry. A single integer, e.g. "3", would
    specify that up to 3 variable mods are allowed.  Comma separated values, e.g. "2,4"
    would specify that peptides must have between 2 and 4 of this variable modification.
  - The fifth entry specifies the distance the modification is applied to from the respective terminus:
    - -2 = apply anywhere except c-terminal residue of peptide
    - -1 = no distance contraint
    - 0 = only applies to terminal residue
    - 1 = only applies to terminal residue and next residue
    - 2 = only applies to terminal residue through next 2 residues
    - *N* = only applies to terminal residue through next <i>N</i> residues where <i>N</i> is a positive integer
  - The sixth entry specifies which terminus the distance constraint is applied to:
    - 0 = protein N-terminus
    - 1 = protein C-terminus
    - 2 = peptide N-terminus
    - 3 = peptide C-terminus
  - The seventh entry specifies whether peptides must contain this modification.  If set to 1,
    only peptides that contain this modification will be analyzed.
    - 0 = not forced to be present
    - 1 = modification is required 
  - The eigth entry is an optional fragment neutral loss field. For any fragment ion that
    contain the variable modification, a neutral loss will also be analyzed if the specified
    neutral loss value is not zero (0.0).
  - The default value is "0.0 X 0 3 -1 0 0 0.0" if this parameter is missing *except* if Comet is
    compiled with the [Crux](http://crux.ms) flag on.
    For Crux compilation, the default value for variable_mod01 is "15.9949 M 0 3 -1 0 0 0.0" if this
    parameter is missing.

Example:
```
variable_mod01 = 15.9949 M 0 3 -1 0 0 0.0
variable_mod02 = 79.966331 STY 0 3 -1 0 0 97.976896 ... possible phosphorylation on any S, T, Y residue with a neutral loss of 98
variable_mod02 = 79.966331 STY 0 3 -1 0 1 0.0</tt> &nbsp; &nbsp; ... force peptide IDs to contain at least one phosphorylation mod
variable_mod01 = 42.010565 nK 0 3 -1 0 0 0.0</tt> &nbsp; &nbsp; ... acetylation mod to lysine and N-terminus of all peptides
variable_mod01 = 15.994915 n 0 3 0 0 0 0.0</tt> &nbsp; &nbsp; ... oxidation of protein's N-terminus
variable_mod01 = 28.0 c 0 3 8 1 0 0.0</tt> &nbsp; &nbsp; ... modification applied to C-terminus as long as the C-term residue is one of last 9 residues in protein
variable_mod03 = -17.026549 Q 0 1 0 2 0 0.0</tt> &nbsp; &nbsp; ... cyclization of N-terminal glutamine to form pyroglutamic acid (elimination of NH3)
variable_mod04 = -18.010565 E 0 1 0 2 0 0.0</tt> &nbsp; &nbsp; ... cyclization of N-terminal glutamic acid to form pyroglutamic acid (elimination of H2O)
```

Here is a binary modification search example of triple SILAC plus acetylation of lysine.
The SILAC modifications are "R +6 and K +4" (medium) and "R +10 and K +8" (heavy).
In conjunction with K +42 acetylation, the binary modification sets would be
"R +6, K +4, K +4+42" for SILAC medium (binary group 1)> and
"R +10, K +8, K +8+42" for SILAC heavy (binary group 2).
Mass values are listed with no precision for clarity; definitely use precise
modification masses in practice.
```
variable_mod01 = 42.0 K 0 3 -1 0 0 0.0
variable_mod02 =  6.0 R 1 3 -1 0 0 0.0
variable_mod03 =  4.0 K 1 3 -1 0 0 0.0
variable_mod04 = 46.0 K 1 3 -1 0 0 0.0
variable_mod05 = 10.0 R 2 3 -1 0 0 0.0
variable_mod06 =  8.0 K 2 3 -1 0 0 0.0
variable_mod07 = 50.0 K 2 3 -1 0 0 0.0
variable_mod08 =  0.0 X 0 3 -1 0 0 0.0
variable_mod09 =  0.0 X 0 3 -1 0 0 0.0

```
