<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: output_txt_file</h2>

         <ul>
         <li>Controls whether to output search results into a tab-delimited text file (.txt).
         <li>Valid values are 0 (do not output) or 1 (output).
         </ul>

         <p>Example:
         <br><tt>output_txt_file= 0</tt>
         <br><tt>output_txt_file= 1</tt>

         <p>Here is an example output:

<pre>##      100     CometVersion 2013.01 rev. 0     08/23/2013, 01:34:23 PM 08/23/2013, 01:34:42 PM /db/SGD/SGDyeast.fasta
5129    1       600.9771        602.3620        9.62E+00        0.7456  0.0817  77.6    5/8             LISNR   K.LISNR.E       YNL297C 0
5385    1       602.3363        604.3301        1.60E+01        0.5316  0.0424  70.9    3/8             NLTEK   R.NLTEK.T       YBL024W 1
5496    1       602.3431        600.3828        2.16E+01        0.4527  0.0234  6.6     2/8             VVNIR   R.VVNIR.L       YJL012C 0
5430    1       603.3402        604.3665        1.48E+01        0.4519  0.0658  63.0    4/10            LIGSSK  R.LIGSSK.L      YDL090C 0
5861    1       604.3820        603.2885        1.17E+01        0.4390  0.1804  152.1   4/8             KHGDF   K.KHGDF.-       YKL062W 0
5897    1       604.3834        606.3457        9.39E+00        0.5628  0.0154  59.6    5/10            ITTSGK  K.ITTSGK.H      YPL015C 0
</pre></p>

          
         <p>The first line of the output file is a header line which contains the Comet version,
         search start and stop times/dates, and search database.
         Subsequent output lines contains the following columns:
         <ul>
            <li>scan number
            <li>precursor charge state
            <li>experimental peptide mass (singly protonated)
            <li>calculated peptide mass (singly protonated)
            <li>expectation-value
            <li>xcorr
            <li>dCn
            <li>Sp score
            <li>matched/total ions
            <li>plain peptide
            <li>peptide (with prev/next amino acid and mods)
            <li>protein
            <li># duplicate proteins
         </ul>
         </p>


      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
