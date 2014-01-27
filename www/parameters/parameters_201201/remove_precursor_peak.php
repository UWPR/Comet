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
         <li>An input value of 0 will not perform any precursor removal.
         <li>An input value of 1 will remove all peaks around the precursor m/z.
         of the precursor m/z.
         <li>An input value of 2 will remove all charge reduced precursor peaks
         as expected to be present for ETD/ECD spectra.
         <li>This parameter works in conjuction with
         <a href="remove_precursor_tolerance.php">remove_precursor_tolerance</a>
         to specify the tolerance around each precuror m/z that will be removed.
         <li>Valid values are 0, 1, and 2.
         </ul>

         <p>Example:
         <br><tt>remove_precursor_peak = 0</tt>
         <br><tt>remove_precursor_peak = 1</tt>
         <br><tt>remove_precursor_peak = 2</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
