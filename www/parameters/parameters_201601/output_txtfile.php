<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: output_txtfile</h2>

         <ul>
         <li>Controls whether to output search results into a tab-delimited text file (.txt).
         <li>Valid values are 0 (do not output) or 1 (output).
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>output_txtfile = 0</tt>
         <br><tt>output_txtfile = 1</tt>

         <p>Here's snippet of sample output below.  The first line of the output file is a
         header line which contains the Comet version, search start time/date, and search
         database.  The second line contains the column headers.

<pre>CometVersion 2016.01 rev. 3     JE102306_102306_18Mix4_Tube1_01 05/18/2017, 08:57:57 AM /net/pr/vol1/ProteomicsResource/dbase/UniProt/20160308/HUMAN.fasta.20160308
scan    num     charge  exp_neutral_mass        calc_neutral_mass       e-value xcorr   delta_cn        sp_score        ions_matched    ions_total      plain_peptide   peptide modifications   prev_aa next_aa protein duplicate_protein_count
2       1       2       2313.605447     2313.113325     1.29E+00        0.7409  0.2166  33.8    4       42      MPTFSTPGAKGEGPDVHMTLPK  K.M[16.0]PTFSTPGAKGEGPDVHMTLPK.G        K       G       sp|Q09666|AHNK_HUMAN    0
2       2       2       2313.605447     2313.140994     8.34E+00        0.5804  0.2412  16.8    3       40      IEDDVVVTQDSPLILSADCPK   R.IEDDVVVTQDSPLILSADCPK.E       R       E       sp|Q9NQH7|XPP3_HUMAN    2
2       3       2       2313.605447     2314.222236     1.03E+01        0.5622  0.2572  20.1    3       40      GAGWLFGAKVTNEFVHINNLK   R.GAGWLFGAKVTNEFVHINNLK.L       R       L       sp|O00743|PPP6_HUMAN    2
2       4       2       2313.605447     2313.236232     1.18E+01        0.5504  0.3103  18.3    3       44      RKGAILSEEELAAMSPTAAAVAK K.RKGAILSEEELAAMSPTAAAVAK.I     K       I       sp|P12270|TPR_HUMAN     1
2       5       2       2313.605447     2314.242028     1.87E+01        0.5111  0.3478  13.9    3       42      TGGYLEGSILIPDAILGLEEVR  R.TGGYLEGSILIPDAILGLEEVR.L      R       L       tr|H3BSC9|H3BSC9_HUMAN  0
3       1       2       2249.725447     2248.910007     6.30E+00        0.4870  0.1460  17.6    3       40      ESDASMDSDASMDSEPTPHLK   R.ESDASMDSDASMDSEPTPHLK.T       R       T       sp|Q99973|TEP1_HUMAN    3
3       2       2       2249.725447     2249.128790     1.84E+01        0.4159  0.1492  15.7    3       40      ENGLFSHSSLSNTSQKSLSVK   K.ENGLFSHSSLSNTSQKSLSVK.E       K       E       sp|Q5THJ4|VP13D_HUMAN   2
3       3       2       2249.725447     2250.212066     1.88E+01        0.4143  0.1932  16.6    3       36      QLLKFIPGLHRAVEEEESR     K.QLLKFIPGLHRAVEEEESR.F K       F       sp|O60759|CYTIP_HUMAN   1
3       4       2       2249.725447     2249.020487     2.60E+01        0.3929  0.1959  12.2    3       36      KKNITYYDSMGGINNEACR     R.KKNITYYDSM[16.0]GGINNEACR.I   R       I       sp|Q9P0U3|SENP1_HUMAN   1
3       5       2       2249.725447     2249.108528     2.65E+01        0.3916  0.2192  21.8    3       36      GTGEFDFLTMNQKMLKPHR     R.GTGEFDFLTMNQKMLKPHR.T R       T       sp|Q8NA69|CS045_HUMAN   0
4       1       2       2493.985447     2494.188555     5.11E+00        0.6132  0.0784  29.0    4       36      YRGQYQKYALLWMESVQCR     K.YRGQYQKYALLWM[16.0]ESVQCR.L   K       L       sp|Q8WXD0|RXFP2_HUMAN   1
4       2       2       2493.985447     2494.226120     9.12E+00        0.5651  0.2236  67.6    6       46      GSSLDILSSLNSPALFGDQDTVMK        R.GSSLDILSSLNSPALFGDQDTVMK.A    R       A       sp|P35712|SOX6_HUMAN    1
4       3       2       2493.985447     2493.431895     2.66E+01        0.4761  0.2641  13.2    3       46      IKLGGQVLGTKSVPTFTVIPEGPR        K.IKLGGQVLGTKSVPTFTVIPEGPR.S    K       S       tr|J3KQ70|J3KQ70_HUMAN  6
4       4       2       2493.985447     2493.228171     3.59E+01        0.4513  0.2745  26.5    4       44      ISGASEKDIVHSGLAYTMERSAR R.ISGASEKDIVHSGLAYTM[16.0]ERSAR.Q       R       Q       sp|P00367|DHE3_HUMAN    3
4       5       2       2493.985447     2494.376840     3.88E+01        0.4449  0.3140  34.4    4       44      RLQGTGVTTYAVHPGVVRSELVR K.RLQGTGVTTYAVHPGVVRSELVR.H     K       H       sp|Q96NR8|RDH12_HUMAN   0
</pre></p>

         <p>Note that there is a different text output if Comet is compiled with the
         Crux flag (i.e. add -DCRUX to the CXXFLAGS in the Makefiles under Linux or #define CRUX
         in Common.h).  Here's the Crux-specific text output where the files have a
         ".target.txt" or ".decoy.txt" extensions.

<pre>scan    charge  spectrum precursor m/z  spectrum neutral mass   peptide mass    delta_cn        sp score        sp rank xcorr score     xcorr rank      b/y ions matched        b/y ions total  total matches/spectrum  sequence        modified sequence       modifications   protein id      flanking aa     e-value
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
</pre></p>


      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
