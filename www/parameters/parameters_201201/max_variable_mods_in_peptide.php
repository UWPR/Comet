<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: max_variable_mods_in_peptide</h2>

         <ul>
         <li>Specifies the total/maximum number of residues that can be modified in a peptide.
         <li>As opposed to specifying the maximum number of variable modifications for each
             of the 6 possible variable modifications, this entry limits the global number
             of variable mods possible in each peptide.
         </ul>

         <p>Example:
         <br><tt>max_variable_mods_in_peptide = 6</tt>
         <br><tt>max_variable_mods_in_peptide = 10</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
