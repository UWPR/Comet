### Comet parameter: output_txtfile

- Controls whether to output search results into a tab-delimited text file (.txt).
- Valid values are 0 (do not output) or 1 (output).
- The default value is "0" if this parameter is missing.
- "peff_modified_peptide" column is present only if a [PEFF search](peff_format.html) is performed.
PEFF modifications will contain bracketed OBO identifiers for each PEFF modification.  User specified
static and variable modifications will still be reported as bracketed mass differences.  For
example:  K.n[144.1000]RT[MOD:00047]EAR.S
- The "modifications" column reports a comma separated list of modifications in the format
"position_code_massdiff", e.g. "3_V_15.9949".  The "position" field is the position of the peptide residue
where position 1 is the first residue.  A position of "0" denotes the previous flanking amino acid and a position
of 1 greater than the peptide length denotes the following flanking amino acid; these are relevant for
PEFF substitutions.  The "code" field can be "S" for a static modification, "V" for a variable modification,
"P" for a PEFF modification, and "p" for a PEFF substitution.  In the case of a PEFF substitution, the
original amino acid is listed in the "massdiff" field, e.g. "2_p_L" indicates the 2nd residue was originally
a leucine before the PEFF substitution.
- The "modifications" string can be appended with:
  - "_N" to denote a N-term protein modification, e.g. "1_S_-17.0265_N"
  - "_n" to denote a N-term peptide modification, e.g. "1_A_42.0146_n"
  - "_C" to denote a C-term protein modification, e.g. "9_R_356.1882_C"
  - "_c" to denote a C-term peptide modification, e.g. "12_K_42.0106_c"

Example:
```
output_txtfile = 0
output_txtfile = 1
```

Here's snippet of sample output below.  The first line of the output file is a
header line which contains the Comet version, search start time/date, and search
database.  The second line contains the column headers.

```
CometVersion 2017.01 rev. 0       20141219_Hela_01        06/05/2017, 04:34:13 PM 18mix_reverseSGD.fasta
scan   num  charge  exp_neutral_mass  calc_neutral_mass  e-value   xcorr   delta_cn  sp_score  ions_matched  ions_total  plain_peptide    modified_peptide              prev_aa  next_aa  protein                                                               protein_count   modifications
10685  1    2       1308.585447       1308.586732        3.16E-06  2.1508  0.6785    238.7     9             22          MDSTANEVEAVK     K.M[15.9949]DSTANEVEAVK.V     K        V        sp|P07237|PDIA1_HUMAN,tr|H0Y3Z3|H0Y3Z3_HUMAN,tr|H7BZ94|H7BZ94_HUMAN   3               1_V_15.994900
10685  2    2       1308.585447       1308.587600        6.85E+00  0.6914  0.7245    29.8      3             20          DMMDPVAVCKK      K.DM[15.9949]MDPVAVCKK.V      K        V        sp|Q6NUJ1|SAPL1_HUMAN                                                 1               2_V_15.994900,9_S_57.021464
10685  3    2       1308.585447       1308.595463        1.84E+01  0.5926  0.7527    18.6      2             18          DMAEMQRVWK       R.DM[15.9949]AEMQRVWK.E       R        E        sp|Q15058|KIF14_HUMAN                                                 1               2_V_15.994900
14937  1    3       1775.648171       1774.634519        4.18E-03  2.4234  0.7827    305.8     13            52          DCDLQEDACYNCGR   K.DCDLQEDACYNCGR.G            K        G        sp|P62633|CNBP_HUMAN,sp|P62633-2|CNBP_HUMAN,sp|P62633-3|CNBP_HUMAN    3               2_S_57.021464,9_S_57.021464,12_S_57.021464
14937  2    3       1775.648171       1774.655633        4.29E+02  0.5265  0.8161    5.1       2             56          EEESAMSSDRMDCGR  K.EEESAMSSDRM[15.9949]DCGR.K  K        K        sp|Q6WCQ1|MPRIP_HUMAN,tr|J3KSW8|J3KSW8_HUMAN,sp|Q6WCQ1-2|MPRIP_HUMAN  3               11_V_15.994900,13_S_57.021464
14937  3    3       1775.648171       1774.655633        5.79E+02  0.4773  0.8161    5.1       2             56          EEESAMSSDRMDCGR  K.EEESAM[15.9949]SSDRMDCGR.K  K        K        sp|Q6WCQ1|MPRIP_HUMAN,tr|J3KSW8|J3KSW8_HUMAN,sp|Q6WCQ1-2|MPRIP_HUMAN  3               6_V_15.994900,13_S_57.021464
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
scan    charge  spectrum precursor m/z  spectrum neutral mass   peptide mass    delta_cn        sp score        sp rank xcorr score     xcorr rank      b/y ions matched        b/y ions total  total matches/spectrum  sequence        modified sequence       modifications   protein id      flanking aa     e-value
2       2       1157.810000     2313.605447     2313.113325     0.1250  33.7558 4       0.7409  1       4       42      80614   MPTFSTPGAKGEGPDVHMTLPK  K.M[147]PTFSTPGAKGEGPDVHMTLPK.G 1_V_15.994900   sp|Q09666|AHNK_HUMAN    KG      9.26E+00
2       2       1157.810000     2313.605447     2313.975870     0.1309  21.1120 12      0.6483  2       3       34      80614   SSAMKKIESETTFSLIFR      R.S[167]S[167]AMKKIES[167]ETTFSLIFR.L   1_V_79.966331, 2_V_79.966331, 9_V_79.966331     sp|Q8WXI7|MUC16_HUMAN   RL      2.89E+01
2       2       1157.810000     2313.605447     2314.096792     0.1309  25.9061 7       0.6440  3       3       34      80614   RLLPGSSDWEQQRHQLER      K.RLLPGSS[167]DWEQQRHQLER.R     7_V_79.966331   sp|Q6ZT98|TTLL7_HUMAN   KR      3.06E+01
2       2       1157.810000     2313.605447     2314.096792     0.1950  25.9061 7       0.6440  4       3       34      80614   RLLPGSSDWEQQRHQLER      K.RLLPGS[167]SDWEQQRHQLER.R     6_V_79.966331   sp|Q6ZT98|TTLL7_HUMAN   KR      3.06E+01
2       2       1157.810000     2313.605447     2312.896434     0.1972  22.5144 10      0.5965  5       3       36      80614   IDESSLTGESDHVRKSADK     K.IDES[167]S[167]LTGESDHVRKS[167]ADK.D  4_V_79.966331, 5_V_79.966331, 16_V_79.966331    sp|Q16720|AT2B3_HUMAN   KD      5.48E+01
3       2       1125.870000     2249.725447     2249.032520     0.0051  29.4360 4       0.5709  1       4       40      80457   TLQEPVARPSGASSSQTPNDK   R.TLQEPVARPS[167]GASSSQTPNDK.E  10_V_79.966331  sp|Q8NBA8|DTWD2_HUMAN   RE      4.50E+01
3       2       1125.870000     2249.725447     2248.771604     0.0051  35.4842 1       0.5680  2       4       30      80457   VVKETSYEMMMQCVSR        K.VVKETS[167]Y[243]EMM[147]M[147]QC[160]VS[167]R.M      6_V_79.966331, 7_V_79.966331, 10_V_15.994900, 11_V_15.994900, 13_S_57.021464, 15_V_79.966331    sp|Q9NZJ7|MTCH1_HUMAN   KM      4.68E+01
3       2       1125.870000     2249.725447     2248.771604     0.0080  35.4842 1       0.5680  3       4       30      80457   VVKETSYEMMMQCVSR        K.VVKET[181]SY[243]EMM[147]M[147]QC[160]VS[167]R.M      5_V_79.966331, 7_V_79.966331, 10_V_15.994900, 11_V_15.994900, 13_S_57.021464, 15_V_79.966331    sp|Q9NZJ7|MTCH1_HUMAN   KM      4.68E+01
3       2       1125.870000     2249.725447     2248.771604     0.0114  35.4842 1       0.5664  4       4       30      80457   VVKETSYEMMMQCVSR        K.VVKET[181]S[167]YEMM[147]M[147]QC[160]VS[167]R.M      5_V_79.966331, 6_V_79.966331, 10_V_15.994900, 11_V_15.994900, 13_S_57.021464, 15_V_79.966331    sp|Q9NZJ7|MTCH1_HUMAN   KM      4.79E+01
3       2       1125.870000     2249.725447     2249.986133     0.0114  21.0194 12      0.5644  5       3       34      80457   GARASPRTLNLSQLSFHR      R.GARASPRT[181]LNLS[167]QLS[167]FHR.V   8_V_79.966331, 12_V_79.966331, 15_V_79.966331   sp|P29597|TYK2_HUMAN    RV      4.91E+01
4       2       1248.000000     2493.985447     2494.319641     0.0776  35.6025 12      0.9110  1       4       40      100697  KFSRPLLPATTTKLSQEEQLK   K.KFSRPLLPATT[181]TKLSQEEQLK.S  11_V_79.966331  sp|Q9NYI0|PSD3_HUMAN    KS      4.80E+00
4       2       1248.000000     2493.985447     2494.138504     0.0776  50.6368 8       0.8403  2       5       38      100697  LVSDVSATKIPHIWLMLSTK    R.LVSDVS[167]AT[181]KIPHIWLM[147]LST[181]K.M    6_V_79.966331, 8_V_79.966331, 16_V_15.994900, 19_V_79.966331    sp|Q9BVV2|CT195_HUMAN   RM      1.07E+01
4       2       1248.000000     2493.985447     2494.138504     0.0776  50.6368 8       0.8403  3       5       38      100697  LVSDVSATKIPHIWLMLSTK    R.LVSDVS[167]AT[181]KIPHIWLM[147]LS[167]TK.M    6_V_79.966331, 8_V_79.966331, 16_V_15.994900, 18_V_79.966331    sp|Q9BVV2|CT195_HUMAN   RM      1.07E+01
4       2       1248.000000     2493.985447     2494.138504     0.0776  50.6368 8       0.8403  4       5       38      100697  LVSDVSATKIPHIWLMLSTK    R.LVS[167]DVSAT[181]KIPHIWLM[147]LST[181]K.M    3_V_79.966331, 8_V_79.966331, 16_V_15.994900, 19_V_79.966331    sp|Q9BVV2|CT195_HUMAN   RM      1.07E+01
4       2       1248.000000     2493.985447     2494.138504     0.0776  50.6368 8       0.8403  5       5       38      100697  LVSDVSATKIPHIWLMLSTK    R.LVS[167]DVSAT[181]KIPHIWLM[147]LS[167]TK.M    3_V_79.966331, 8_V_79.966331, 16_V_15.994900, 18_V_79.966331    sp|Q9BVV2|CT195_HUMAN   RM      1.07E+01
```
