<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: max_permutations</h2>

         <ul>
         <li>Some peptides with many potentially modified residues can generate A LOT of
             modification combinations, adversely affecting search times.  This parameter
             applies a limit to the maximum number of modification permutations that will
             be tested for each peptide.
         <li>The default value is "10000" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>max_permutations = 10000</tt>
         <br><tt>max_permutations = 50000</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
