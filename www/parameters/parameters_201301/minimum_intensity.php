<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: minimum_intensity</h2>

         <ul>
         <li>A floating point number indicating the minimum intensity value
         for input the input peaks.
         <li>If an experimental MS/MS peak intensity is less than this value,
         it will not be read in and used in the analysis.
         <li>This is one mechanism to get rid of systemmatic background noise
         that has a near contant peak intensity.
         <li>If a peak does not pass this minimum intensity threshold, it will
         also not be counted towards the <a href="minimum_peaks.php">minimum_peaks</a>
         parameter.
         <li>Valid values are any floating point number.
         </ul>

         <p>Example:
         <br><tt>minimum_intensity = 1000.0</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
