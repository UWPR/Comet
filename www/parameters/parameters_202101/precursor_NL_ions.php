<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: precursor_NL_ions</h2>

         <ul>
         <li>Controls whether or not precursor neutral loss peaks are considered in the xcorr scoring.
         <li>If left blank, this parameter is ignored.
         <li>To consider precursor neutral loss peaks, add one or more neutral loss mass value separated by a space.
         <li>Each entered mass value will be subtracted from the experimentral precursor mass and resulting
         neutral loss m/z values for all charge states (from 1 to precursor charge) will be analyzed.
         <li>As these neutral loss peaks are analyzed along side fragment ion peaks,
         the fragment tolerance settings (fragment_bin_tol, fragment_bin_offset, theoretical_fragment_ion)
         apply to the precursor neutral loss peaks.
         <li>The default value is blank/unused if this parameter is missing.
         <li>A value of "0" or "0.0" will caues Comet to consider the intact precursor peaks (m/z's of the
         precursor in all fragment charge states) as ions to analyze in the ms/ms scan.
         <li>Negative mass values will be ignored.
         </ul>

         <p>Example:
         <br><tt>precursor_NL_ions =</tt> &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; ... <i>entry blank; unused</i>
         <br><tt>precursor_NL_ions = 79.96633 97.97689</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
