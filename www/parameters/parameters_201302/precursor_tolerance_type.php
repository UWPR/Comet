<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: precursor_tolerance_type</h2>

         <ul>
         <li>This parameter controls how the <a href="peptide_mass_tolerance.php">peptide_mass_tolerance</a>
         parameter is applied.  That tolerance can be applied to the singly charged peptide mass or it can
         be applied to the precursor m/z.
         <li>Valid values are 0 or 1.
         <li>Set this parameter to 0 to specify that the mass tolerance is applied to the singly charged peptide mass.
         <li>Set this parameter to 1 to specify that the mass tolerance is applied to the precursor m/z.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>precursor_tolerance_type = 0</tt>
         <br><tt>precursor_tolerance_type = 1</tt>

         <p>For example, assume a 2.1 Da <a href="peptide_mass_tolerance.php">peptide_mass_tolerance</a> was
         specified.  If "precursor_tolerance_type = 0" then a peptide with MH+ mass of 1250.4 will be queried
         against peptide sequences with MH+ masses between 1248.3 to 1252.5.

         <p>If "precursor_tolerance_type = 1" for a 10 ppm
         <a href="peptide_mass_tolerance.php">peptide_mass_tolerance</a>, then the 10 ppm tolerance
         is applied at the precursor m/z level.  Assuming the precursor charge is 3+ for the 1320.8 MH+ mass,
         this means the precursor m/z is 440.93855.  +/- 10 ppm on 440.93855 gives the
         range 440.9341 to 440.94296.  This range corresponds to peptides with MH+ masses between
         1320.7868 and 1320.8132.

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
