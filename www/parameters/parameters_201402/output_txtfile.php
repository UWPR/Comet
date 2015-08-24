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
         database.  The second line contains the column headers.  Only the top hit for each
         search is reported in the text output; the parameter
         "<a href="num_output_lines.php">num_output_lines</a>" does not apply to the text output.

<pre>CometVersion 2013.02 rev. 0   100  10/04/2013, 08:37:09 AM  /net/pr/vol1/ProteomicsResource/dbase/SGD/SGDyeast.fasta.20101117
scan  charge  exp_neutral_mass  calc_neutral_mass  e-value  xcorr  delta_cn  sp_score  ions_matched  ions_total  plain_peptide  peptide  prev_aa  next_aa  protein  duplicate_protein_count
5129  1  599.9698  602.3136  1.73E+01  0.6847  0.0995  44.3  4  8  DAKNR  R.DAKNR.I   R  I  YML019W  0
5385  1  601.3290  603.3228  1.73E+01  0.5316  0.1905  70.9  3  8  NLTEK  R.NLTEK.T   R  T  YBL024W  1
5496  1  601.3358  599.3755  1.67E+01  0.4527  0.1315  6.6   2  8  VVNIR  R.VVNIR.L   R  L  YJL012C  0
5430  1  602.3329  605.2591  2.33E+01  0.4222  0.0854  73.2  4  8  DVGCR  K.DVGCR.I   K  I  YER085C  0
5829  1  616.4184  615.3414  5.26E+01  0.4321  0.0658  8.5   2  8  KPPMK  -.KPPM*K.Q  -  Q  DECOY__YBR025C  2

</pre></p>

         <p>Comet's text output is different if Comet was compiled for
         <a href="noble.gs.washington.edu/proj/crux/">Crux</a> compatibility.
         Here's the example text output if you are running Comet in Crux.
         Note that up to 5 hits per spectrum are reported and there is just
         a single column header line.

<pre>scan  charge  spectrum precursor m/z  spectrum neutral mass  matches/spectrum  peptide mass  e-value  xcorr score  xcorr rank  delta_cn  sp score  sp rank  b/y ions matched  b/y ions total  sequence  flanking aa  protein id
5129  1  600.9771  599.9698  5440  602.3136  1.73E+01  0.6847  1  0.0995  44.3   19  4  8   DAKNR   RI  YML019W
5129  1  600.9771  599.9698  5440  602.3249  1.96E+01  0.6714  2  0.1016  46.1   17  5  10  RTGGGR  RI  YGR054W
5129  1  600.9771  599.9698  5440  602.3136  1.99E+01  0.6698  3  0.1131  77.6   6   5  8   NISNR   KV  YMR033W
5129  1  600.9771  599.9698  5440  601.2820  2.16E+01  0.6612  4  0.1240  38.6   22  4  8   EPSNR   RD  YPL161C
5129  1  600.9771  599.9698  5440  600.3231  0.00E+00  0.6532  5  0.1389  104.6  3   6  8   IESPR   KM  YLL021W
5385  1  602.3363  601.3290  6518  603.3228  1.73E+01  0.5316  1  0.1905  70.9   2   3  8   NLTEK   RT  YBL024W
5385  1  602.3363  601.3290  6518  600.3707  1.86E+01  0.5249  2  0.2022  31.0   8   3  8   ARNIK   KT  DECOY_YIL159W
5385  1  602.3363  601.3290  6518  602.3249  2.02E+01  0.5173  3  0.2134  47.4   4   3  8   ARNSR   KI  DECOY_YLL040C
5385  1  602.3363  601.3290  6518  601.3184  2.19E+01  0.5100  4  0.2148  7.1    19  2  10  GAVNNK  KS  DECOY_YMR125W
5385  1  602.3363  601.3290  6518  599.4119  0.00E+00  0.5091  5  0.2148  28.3   9   3  8   ALLKR   RI  YML107C
</pre></p>


      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
