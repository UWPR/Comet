<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: peptide_mass_tolerance</h2>

         <ul>
         <li>This parameter controls the mass tolerance value.
         <li>The mass tolerance is set at +/- the specified number i.e. an entered value of "1.0" applies a -1.0 to +1.0 tolerance.
         <li>The units of the mass tolerance is controlled by the parameter "<a href="peptide_mass_units.php">peptide_mass_units</a>".
         </ul>

         <p>Example:
         <br><tt>peptide_mass_tolerance = 3.0</tt>
         <br><tt>peptide_mass_tolerance = 10.0</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
