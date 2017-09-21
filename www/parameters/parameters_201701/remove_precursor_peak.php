<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: remove_precursor_peak</h2>

         <ul>
         <li>This parameter controls excluding/removing any precursor signals
         from the input MS/MS spectrum.
         <li>Valid values are 0, 1, 2, and 3.
         <li>Set this parameter to 0 to not perform any precursor peak removal.
         <li>Set this parameter to 1 to remove all peaks around the precursor m/z.
         <li>Set this parameter to 2 to remove all charge reduced precursor peaks
         as expected to be present for ETD/ECD spectra.
         <li>Set this parameter to 3 to remove the HPO3 (-80) and H3PO4 (-98)
         precursor phosphate neutral loss peaks.
         <li>This parameter works in conjuction with
         <a href="remove_precursor_tolerance.php">remove_precursor_tolerance</a>
         to specify the tolerance around each precuror m/z that will be removed.
         <li>Valid values are 0, 1, and 2.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>remove_precursor_peak = 0</tt>
         <br><tt>remove_precursor_peak = 1</tt>
         <br><tt>remove_precursor_peak = 2</tt>
         <br><tt>remove_precursor_peak = 3</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
