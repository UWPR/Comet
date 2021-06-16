<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: explicit_deltacn</h2>

         <ul>
         <li>This parameter controls whether or not Comet reports the deltaCn value calculated
         between the top two peptides ("explicit_deltacn = 1") or between the top peptide
         and the first dissimilar peptide (default behavior, "explicit_deltacn = 0").
         <li>The deltaCn score is the normalized difference between two cross-correlation scores.
         So deltaCn between the top two peptides is calculated as "(xcorr1 - xcorr2) / xcorr1"
         where xcorr1 is the top scoring peptide and xcorr2 is the second best scoring peptide.
         <li>However, there are cases were the top two (or top N) scoring peptides are very
         similar.  They may be different modified forms of the same peptide e.g. DLRS*TWDK
         and DLRST*WDK.  In this case, the deltaCn score will be very small because the two
         peptides are very similar and will have very similar xcorr scores.
         <li>To mitigate this issue of reporting the deltaCn score for similar peptides, Comet
         by default performs a crude sequence similarity analysis and reports the deltaCn
         score as the difference between the top scoring peptide and the first dissimilar
         peptide.  This is the default behavior.
         <li>This parameter was added to ignore the similarity analysis and always report
         deltaCn as the normalized difference between the top two xcorr scores.
         <li>To keep the default behavior of using similarity analysis, set this parameter to "0".
         <li>To calculate deltaCn between the top two scoring peptides even if their sequences
         are similar, set this parameter to "1".
         </ul>

         <p>Example:
         <br><tt>explicit_deltacn = 0</tt>
         <br><tt>explicit_deltacn = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
