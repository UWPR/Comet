### Notes 2017.10.05

Here's some notes on setting Comet search parameters for various
isotopic/isobaric labeling strategies.

#### ITRAQ 4-plex

The 4-plex reagent has different  monoisotopic mass values for 114
(144.105918), 115 (144.09599), and 116/117 (144.102063).  The mass value used
below is derived from averaging the three monoisotopic masses.  The
"clear_mz_range" parameter is used to remove any reporter ion signal from the
spectra so that they aren't matched as fragment ions.
```
add_Nterm_peptide = 144.10253
add_K_lysine = 144.10253
clear_mz_range = 113.5 117.5
```

#### ITRAQ 8-plex:

Similarly, the 8-plex reagent has two different set of masses for
115/118/119/121 (304.199040) and 113/114/116/117 (304.205360).  The mass
modification below is the average of the two.
```
add_Nterm_peptide = 304.2022
add_K_lysine = 304.2022
clear_mz_range = 112.5 121.5
```

#### TMT 2-plex:

```
add_Nterm_peptide = 225.155833
add_K_lysine = 225.155833
clear_mz_range = 125.5 127.5
```

#### TMT 6-plex and 10-plex:

```
add_Nterm_peptide = 229.162932
add_K_lysine = 229.162932
clear_mz_range = 125.5 131.5
```

#### SILAC4:

There are a number of different SILAC reagents with a ~4 Da  modification
(based on combinations of C13 and N15), each with different sites of
specificity.  My example below is for the 15N(4) reagent applied to R residues.
Adjust the modification mass and residue(s) applied to as necessary.

To perform a mixed light/heavy search using a variable modification search
in binary mode (binary mode = no mixing light and heavy modifications within a
peptide so all arginine residues are considered light or all arginine
residues are considered light):

```
variable_mod01 = 3.988140 R 1 3 -1 0 0
```

To search just the heavy labeled sample, you can apply a static modification:

```
add_R_lysine = 3.988140
```

#### SILAC6:

I'm using the 13C(6) SILAC mass in the example below assuming it's applied to
both K and R; adjust as necessary.  There's at least one more SILAC reagent
with ~6 Da modification mass and different residue specificity: 13C(5) 15N(1)

To perform a mixed light/heavy search using a variable modification search in
binary mode (binary mode = no mixing light and heavy modifications within a
peptide so all arginine+lysine residues are considered light or all
arginine+lysine residues are considered light):

```
variable_mod01 = 6.020129 KR 1 3 -1 0 0
```

To search just the heavy labeled sample, you can apply a static modification:

```
add_K_lysine = 6.020129
add_R_arginine = 6.020129
```

#### SILAC8:

The example below is for 13C(6) 15N(2) on K residues.

Variable (binary) modification search:

```
variable_mod01 = 8.014199 K 1 3 -1 0 0
```

Static modification for just the heavy labeled search:

```
add_K_lysine = 8.014199
```
