<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: num_enzyme_termini</h2>

         <ul>
         <li>This parameter specifies the number of enzyme termini a peptide must
         have.
         <li>For example, if trypsin were specified as the search enzyme, only 
         fully tryptic peptides would be analyzed if "num_enzyme_termini = 2"
         whereas semi-tryptic peptides would be analyzed if "num_enzyme_termini = 1".
         <li>This parameter is unused if a no-enzyme search is specified.
         <li>Valid values are 1 and 2.


         <p>Example:
         <br><tt>num_enzyme_termini = 1</tt>
         <br><tt>num_enzyme_termini = 2</tt> 

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
