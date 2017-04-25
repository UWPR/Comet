<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: mass_offsets</h2>

         <ul>
         <li>This parameter allows the user to specify one or more "mass offsets" to apply.
         <li>This value is effective subtracted from
         each precursor mass such that peptides that are smaller than the precursor mass
         by the offset value can still be matched to the respective spectrum.
         The application of this parameter is for those uses cases where say a chemical
         tag is applied and always falls off the peptide before/during fragmentation.
         <li>Only positive numbers only are allowed.
         <li>When this parameter is applied, one must add the offset "0.0" if you want
         the search to also analyze peptides that match the base precursor mass.
         </ul>

         <p>Example:
         <br><tt>mass_offsets = 42.0123 48.3812 82.030</tt>
         <br><tt>mass_offsets = 0.0 42.0123 48.3812 82.030</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
