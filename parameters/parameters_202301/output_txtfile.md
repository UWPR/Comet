### Comet parameter: output_txtfile

- Controls whether to output search results into a tab-delimited text file (.txt).
- Valid values are 0 (do not output) or 1 (output).
- The default value is "0" if this parameter is missing.
- "peff\_modified\_peptide" column is present only if a [PEFF search](peff_format.html) is performed.
PEFF modifications will contain bracketed OBO identifiers for each PEFF modification.  User specified
static and variable modifications will still be reported as bracketed mass differences.  For
example:  K.n[144.1000]RT[MOD:00047]EAR.S
- The "modifications" column reports a comma separated list of modifications in the format
"position\_code\_massdiff", e.g. "3\_V\_15.9949".  The "position" field is the position of the peptide residue
where position 1 is the first residue.  A position of "0" denotes the previous flanking amino acid and a position
of 1 greater than the peptide length denotes the following flanking amino acid; these are relevant for
PEFF substitutions.  The "code" field can be "S" for a static modification, "V" for a variable modification,
"P" for a PEFF modification, and "p" for a PEFF substitution.  In the case of a PEFF substitution, the
original amino acid is listed in the "massdiff" field, e.g. "2\_p\_L" indicates the 2nd residue was originally
a leucine before the PEFF substitution.
- The "modifications" string can be appended with:
  - "\_N" to denote a N-term protein modification, e.g. "1\_S\_-17.0265\_N"
  - "\_n" to denote a N-term peptide modification, e.g. "1\_A\_42.0146\_n"
  - "\_C" to denote a C-term protein modification, e.g. "9\_R\_356.1882_C"
  - "\_c" to denote a C-term peptide modification, e.g. "12\_K\_42.0106\_c"

Example:
```
output_txtfile = 0
output_txtfile = 1
```

Here's snippet of sample output below.  The first line of the output file is a
header line which contains the Comet version, search start time/date, and search
database.  The second line contains the column headers.

```
CometVersion 2022.01 rev. 0   20170103_HelaQC_01   01/13/2023, 03:07:37 PM human.small
scan  num   charge   exp_neutral_mass  calc_neutral_mass e-value  xcorr delta_cn sp_score ions_matched   ions_total  plain_peptide  modified_peptide  prev_aa  next_aa  protein  protein_count  modifications  retention_time_sec   sp_rank
5160  4  3  1593.645971 1591.615160 8.24E+00 0.9960   0.0658   22.7  5  48 DYDSNLNASRDDK  K.DYDS[79.9663]NLNASRDDK.K K  K  sp|Q14573|ITPR3_HUMAN   1  4_V_79.966331  1241.7   15
5160  5  3  1593.645971 1592.668956 1.47E+01 0.9305   0.0041   38.0  6  52 KDADVEDEDTEEAK R.KDADVEDEDTEEAK.T   R  T  sp|O60306|AQR_HUMAN  1  -  1241.7   3
5161  1  2  1198.478447 1195.478399 1.20E+01 0.6363   0.2096   46.8  4  16 MNCLDCLDR   R.MNCLDCLDR.T  R  T  sp|O15056|SYNJ2_HUMAN   1  3_S_57.021464,6_S_57.021464   1241.8   1
5161  2  2  1198.478447 1198.494758 4.07E+01 0.5030   0.0399   41.7  3  16 YEVSSPYFK   R.YEVS[79.9663]SPYFK.V  R  V  sp|P55290|CAD13_HUMAN   1  4_V_79.966331  1241.8   2
5161  3  2  1198.478447 1198.494758 4.07E+01 0.5028   0.0396   41.7  3  16 YEVSSPYFK   R.YEVSS[79.9663]PYFK.V  R  V  sp|P55290|CAD13_HUMAN   1  5_V_79.966331  1241.8   2
5161  4  2  1198.478447 1196.471916 4.89E+01 0.4829   0.0384   32.7  3  16 TYKMSMANR   K.TYKM[15.9949]S[79.9663]MANR.G  K  G  sp|P31327|CPSM_HUMAN 1  4_V_15.994900,5_V_79.966331   1241.8   3
5161  5  2  1198.478447 1196.471916 5.12E+01 0.4779   0.0284   32.7  3  16 TYKMSMANR   K.TYKMS[79.9663]M[15.9949]ANR.G  K  G  sp|P31327|CPSM_HUMAN 1  5_V_79.966331,6_V_15.994900   1241.8   3
```

Sample output for a PEFF search; note the two amino acid substitutions encoded in the modifications column.

```
CometVersion 2017.01 rev. 0       20141219_Hela_01        06/05/2017, 04:08:04 PM nextprot_all.peff
scan   num  charge  exp_neutral_mass  calc_neutral_mass  e-value   xcorr   delta_cn  sp_score  ions_matched  ions_total  plain_peptide        modified_peptide                           peff_modified_peptide                          prev_aa next_aa protein                          protein_count   modifications
10007  1    2       2115.965447       2114.972957        1.00E-03  1.7280  0.4856    52.4      7             36          TTHFVEGGDAGNREDQINR  K.TTHFVEGGDAGNREDQINR.L                    K.TTHFVEGGDAGNREDQINR.L                        K       L       nxp:NX_P18124-1                  1
10007  2    2       2115.965447       2115.969012        1.90E+00  0.8888  0.5306    10.3      4             36          VVSPAAVHCALRSPPPEAR  R.VVS[79.9663]PAAVHCALRS[79.9663]PPPEAR.A  R.VVS[MOD:00046]PAAVHCALRS[MOD:00046]PPPEAR.A  R       A       nxp:NX_O43488-1                  1               3_P_79.966331,13_P_79.966331,4_p_R
10007  3    2       2115.965447       2115.978285        3.83E+00  0.8111  0.5925    6.6       3             34          ILDLTECSAVQFDYSQER   R.ILDLTECSAVQFDYSQER.V                     R.ILDLTECSAVQFDYSQER.V                         R       V       nxp:NX_Q9UN19-1,nxp:NX_Q9UN19-2  2
10019  1    2       1388.661763       1388.661917        1.03E-04  2.0068  0.6812    351.0     10            20          QAHLTNQYMQR          R.QAHLTNQYMQR.M                            R.QAHLTNQYMQR.M                                R       M       nxp:NX_P11940-1,nxp:NX_P11940-2  2
10019  2    2       1388.661763       1388.654524        1.91E+01  0.6397  0.6921    15.2      3             26          AGAGSGASGERWQR       R.AGAGSGASGERWQR.V                         R.AGAGSGASGERWQR.V                             R       V       nxp:NX_Q13424-1,nxp:NX_Q13424-2  2               8_p_G
10019  3    2       1388.661763       1388.670096        2.32E+01  0.6179  0.6931    17.0      3             22          TPTTPLPQTPTR         K.TPTTPLPQT[79.9663]PTR.R                  K.TPTTPLPQT[MOD:00047]PTR.R                    K       R       nxp:NX_Q8WU20-1                  1               9_P_79.966331
```

Note that there is a different text output if Comet is compiled with the
Crux flag (i.e. add -DCRUX to the CXXFLAGS in the Makefiles under Linux or #define CRUX
in Common.h).  Here's the Crux-specific text output where the files have a
".target.txt" or ".decoy.txt" extensions.

```
scan  charge   spectrum precursor m/z  spectrum neutral mass   peptide mass   delta_cn sp score sp rank  xcorr score xcorr rank  b/y ions matched  b/y ions total total matches/spectrum  sequence modified sequence modifications  protein id  flanking aa e-value
5160  3  532.222600  1593.645971 1592.643804 0.5933   382.0618 1  2.6817   1  14 52 849   RAAEDDEDDDVDTK K.RAAEDDEDDDVDTK.K   -  sp|P06454|PTMA_HUMAN KK 2.73E-06
5160  3  532.222600  1593.645971 1591.647706 0.6174   44.5599  2  1.0906   2  7  52 849   YDMGILDLGSGDEK R.YDMGILDLGS[79.9663]GDEK.V   10_V_79.966331 sp|A3KN83|SBNO1_HUMAN   RV 3.57E+00
5160  3  532.222600  1593.645971 1591.637795 0.6286   13.5461  40 1.0261   3  4  48 849   TYYLMDPSGNAHK  R.TYYLM[15.9949]DPS[79.9663]GNAHK.W 5_V_15.994900,8_V_79.966331   sp|O15530|PDPK1_HUMAN   RW 6.31E+00
5160  3  532.222600  1593.645971 1591.615160 0.6530   22.6996  15 0.9960   4  5  48 849   DYDSNLNASRDDK  K.DYDS[79.9663]NLNASRDDK.K 4_V_79.966331  sp|Q14573|ITPR3_HUMAN   KK 8.24E+00
5160  3  532.222600  1593.645971 1592.668956 0.6544   38.0146  3  0.9305   5  6  52 849   KDADVEDEDTEEAK R.KDADVEDEDTEEAK.T   -  sp|O60306|AQR_HUMAN  RT 1.47E+01
5161  2  600.246500  1198.478447 1195.478399 0.2096   46.8252  1  0.6363   1  4  16 427   MNCLDCLDR   R.MNCLDCLDR.T  3_S_57.021464,6_S_57.021464   sp|O15056|SYNJ2_HUMAN   RT 1.20E+01
5161  2  600.246500  1198.478447 1198.494758 0.2098   41.7443  2  0.5030   2  3  16 427   YEVSSPYFK   R.YEVS[79.9663]SPYFK.V  4_V_79.966331  sp|P55290|CAD13_HUMAN   RV 4.07E+01
5161  2  600.246500  1198.478447 1198.494758 0.2411   41.7443  2  0.5028   3  3  16 427   YEVSSPYFK   R.YEVSS[79.9663]PYFK.V  5_V_79.966331  sp|P55290|CAD13_HUMAN   RV 4.07E+01
5161  2  600.246500  1198.478447 1196.471916 0.2490   32.7316  3  0.4829   4  3  16 427   TYKMSMANR   K.TYKM[15.9949]S[79.9663]MANR.G  4_V_15.994900,5_V_79.966331   sp|P31327|CPSM_HUMAN KG 4.89E+01
5161  2  600.246500  1198.478447 1196.471916 0.2703   32.7316  3  0.4779   5  3  16 427   TYKMSMANR   K.TYKMS[79.9663]M[15.9949]ANR.G  5_V_79.966331,6_V_15.994900   sp|P31327|CPSM_HUMAN KG 5.12E+01
5162  3  416.869700  1247.587271 1245.604642 0.0096   31.5543  8  1.1721   1  5  36 784   YLAVESPFLK  K.YLAVES[79.9663]PFLK.E 6_V_79.966331  sp|P50416|CPT1A_HUMAN   KE 2.53E+00
5162  3  416.869700  1247.587271 1244.552944 0.1794   61.5555  1  1.1609   2  6  40 784   MVEGHDEVMAK K.MVEGHDEVMAK.S   -  sp|A6NNT2|CP096_HUMAN   KS 2.78E+00
5162  3  416.869700  1247.587271 1247.600697 0.1922   51.6947  2  0.9618   3  6  48 784   SHHANSPTAGAAK  K.SHHANSPTAGAAK.S -  sp|Q15555|MARE2_HUMAN   KS 1.48E+01
5162  3  416.869700  1247.587271 1246.574739 0.2308   24.9601  19 0.9468   4  4  36 784   AFVWSSTLTR  K.AFVWS[79.9663]STLTR.H 5_V_79.966331  sp|A6NNF4|ZN726_HUMAN   KH 1.68E+01
5162  3  416.869700  1247.587271 1244.560792 0.2511   41.4564  5  0.9015   5  5  32 784   HMQEWLETR   R.HM[15.9949]QEWLETR.G  2_V_15.994900  sp|P07311|ACYP1_HUMAN   RG 2.46E+01
5165  3  432.885100  1295.633471 1295.658212 0.4386   429.6465 1  2.2225   1  19 40 917   LGIHEDSQNRK K.LGIHEDSQNRK.K   -  sp|P07900|HS90A_HUMAN   KK 1.29E-02
5165  3  432.885100  1295.633471 1295.623480 0.5113   200.3429 2  1.2476   2  13 36 917   RQITVDSEIR  K.RQITVDS[79.9663]EIR.K 7_V_79.966331  sp|P11532|DMD_HUMAN  KK 4.97E+00
5165  3  432.885100  1295.633471 1295.656026 0.5308   77.2904  30 1.0861   3  9  40 917   VALDSLVLQMK R.VALDS[79.9663]LVLQMK.S   5_V_79.966331  sp|Q14147|DHX34_HUMAN   RS 1.33E+01
5165  3  432.885100  1295.633471 1292.616604 0.5450   84.5115  24 1.0427   4  11 40 917   ALKSSQAFFSK K.ALKSSQAFFS[79.9663]K.L   10_V_79.966331 sp|O00566|MPP10_HUMAN   KL 1.74E+01
5165  3  432.885100  1295.633471 1294.628231 0.5551   70.6711  42 1.0112   5  11 40 917   QALEEKASALR R.QALEEKAS[79.9663]ALR.T   8_V_79.966331  sp|A0A096LP49|CC187_HUMAN  RT 2.11E+01

```
