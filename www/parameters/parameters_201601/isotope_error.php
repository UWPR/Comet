<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: isotope_error</h2>

         <ul>
         <li>This parameter controls whether the <a href="peptide_mass_tolerance.php">peptide_mass_tolerance</a>
         takes into account possible isotope errors in the precursor mass measurement.
         <li>It is possible that an accurately read precursor mass is not measured on the monoisotopic
         peak of a precursor isotopic pattern. In these cases, the precursor mass is measured on the
         first isotope peak (one C13 atom) or possibly even the second or third isotope peak. To address
         this problem, this "isotope_error" parameter allows you to perform an accurate mass search
         (say 10 ppm) even if the precursor mass measurement is off by one or more C13 offsets.
         <li>Valid values are 0, 1, and 2:
            <ul>
            <li>0 performs no isotope error searches
            <li>1 searches -1, 0, +1, +2, and +3 isotope offsets
            <li>2 searches -8, -4, 0, +4, +8 isotope offsets (for +4/+8 stable isotope labeling)
            </ul>
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>isotope_error = 0</tt>
         <br><tt>isotope_error = 1</tt>
         <br><tt>isotope_error = 2</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
