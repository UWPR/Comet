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
         <li>Note that this parameter is applied only when amu or mmu tolerances are specified.  It is
         ignored when ppm tolerances are specified.
         <li>Valid values are 0 or 1.
         <li>Set this parameter to 0 to specify that the mass tolerance is applied to the singly charged peptide mass.
         <li>Set this parameter to 1 to specify that the mass tolerance is applied to the precursor m/z.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>precursor_tolerance_type = 0</tt>
         <br><tt>precursor_tolerance_type = 1</tt>

         <p>For example, assume a 1.0 Da <a href="peptide_mass_tolerance.php">peptide_mass_tolerance</a> was
         specified.  If "precursor_tolerance_type = 0" then a peptide with MH+ mass of 1250.4 will be queried
         against peptide sequences with MH+ masses between 1249.4 to 1251.4.  If "precursor_tolerance_type = 1"
         then say the 2+ m/z is 625.7 so the search mass range would be 624.7 m/z to 626.7 m/z which
         corresponds to MH+ masses between 1248.4 and 1252.4, effectively scaling the mass tolerance by
         the charge state.

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
