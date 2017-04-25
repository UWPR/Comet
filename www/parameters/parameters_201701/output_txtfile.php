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

<pre>CometVersion 2015.01 rev. 0     BASE_NAME_OF_FILE       02/23/2015, 12:15:44 PM /net/database/path/YEAST.fasta.20141030
scan  num   charge   exp_neutral_mass  calc_neutral_mass e-value  xcorr delta_cn sp_score ions_matched   ions_total  plain_peptide  peptide  prev_aa  next_aa  protein  duplicate_protein_count
716     1       3       1312.605337     1311.614541     5.24E+00        0.7965  0.0126  163.4   9       44      PVSVMSLASPTK    R.PVSVM[16.0]SLAS[80.0]PTK.F    R       F       DECOY_sp|Q06108|RGC1_YEAST      0
716     2       3       1312.605337     1311.614541     5.66E+00        0.7864  0.0239  163.4   9       44      PVSVMSLASPTK    R.PVSVM[16.0]S[80.0]LASPTK.F    R       F       DECOY_sp|Q06108|RGC1_YEAST      0
716     3       3       1312.605337     1309.610124     6.07E+00        0.7774  0.0402  62.0    6       40      HASSTIMLVQK     K.HASST[80.0]IM[16.0]LVQK.K     K       K       DECOY_sp|Q99321|DDP1_YEAST      0
716     4       3       1312.605337     1312.613644     6.71E+00        0.7645  0.0583  79.1    7       40      HKTTTSSTKSR     K.HKTTTSSTKS[80.0]R.T   K       T       sp|Q08931|PRM3_YEAST    0
716     5       3       1312.605337     1312.613644     7.49E+00        0.7501  0.0589  111.3   8       40      HKTTTSSTKSR     K.HKTT[80.0]TSSTKSR.T   K       T       sp|Q08931|PRM3_YEAST    0
720     1       3       1313.602896     1310.606404     2.40E+00        1.0726  0.0053  149.1   8       40      DPLDEFMTSLK     K.DPLDEFM[16.0]TSLK.E   K       E       sp|P21372|PRP5_YEAST    0
720     2       3       1313.602896     1314.609819     2.50E+00        1.0669  0.1870  180.8   8       40      VNARPSLSAIK     K.VNARPS[80.0]LS[80.0]AIK.Y     K       Y       sp|Q12078|SMF3_YEAST    0
720     3       3       1313.602896     1310.567285     1.04E+01        0.8720  0.1919  127.7   7       36      LVDIGSYRTK      K.LVDIGSY[80.0]RT[80.0]K.H      K       H       DECOY_sp|P37254|PABS_YEAST      0
720     4       3       1313.602896     1310.567285     1.08E+01        0.8667  0.2093  127.7   7       36      LVDIGSYRTK      K.LVDIGS[80.0]YRT[80.0]K.H      K       H       DECOY_sp|P37254|PABS_YEAST      0
720     5       3       1313.602896     1312.591177     1.23E+01        0.8481  0.2135  52.8    5       44      EDSSGIEGKALK    K.EDS[80.0]SGIEGKALK.T  K       T       DECOY_sp|Q06263|VTA1_YEAST      0
724     1       2       1246.454929     1245.426708     1.32E+00        0.6254  0.0316  38.4    3       16      ALTCVSSLR       R.ALT[80.0]CVS[80.0]S[80.0]LR.T R       T       DECOY_sp|P38752|YHA5_YEAST      0
724     2       2       1246.454929     1244.472716     1.64E+00        0.6056  0.2458  59.2    4       18      ASDKLSSSYK      R.ASDKLS[80.0]S[80.0]SYK.I      R       I       DECOY_sp|Q07351|STP4_YEAST      0
724     3       2       1246.454929     1243.463548     7.18E+00        0.4717  0.3011  33.4    3       18      DDALAHSSIR      K.DDALAHS[80.0]S[80.0]IR.F      K       F       sp|P25374|NFS1_YEAST    0
724     4       2       1246.454929     1244.460451     1.05E+01        0.4370  0.3113  37.6    3       16      SKQSISSLR       K.S[80.0]KQSIS[80.0]S[80.0]LR.S K       S       DECOY_sp|P38308|CS111_YEAST     0
724     5       2       1246.454929     1244.460451     1.13E+01        0.4307  0.3268  37.6    3       16      SKQSISSLR       K.SKQS[80.0]IS[80.0]S[80.0]LR.S K       S       DECOY_sp|P38308|CS111_YEAST     0
</pre></p>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
