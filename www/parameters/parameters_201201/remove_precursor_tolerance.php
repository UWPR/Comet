<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: remove_precursor_tolerance</h2>

         <ul>
         <li>This parameter specifies the mass tolerance around each precursor m/z
         that would be removed when the <a href="remove_precursor_peak.php">remove_precursor_peak</a>
         option is invoked.
         <li>The mass tolerance units is in Da (or Th if you prefer).
         <li>Any non-negative, non-zero floating point number is valid.
         </ul>

        <p>Example:
         <br><tt>remove_precursor_tolerance = 0.75</tt>
         <br><tt>remove_precursor_tolerance = 1.5</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
