<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: peptide_length_range</h2>

         <ul>
         <li>Defines the length range of peptides to search. 
         <li>This parameter has two integer values.
         <li>The first value is the minimum length cutoff and the second value is
         the maximum length cutoff.
         <li>Only peptides within the specified length range are analyzed.
         <li>The maximum peptide length that Comet can analyze is 63.
         <li>The default values are "1 63" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>peptide_length_range = 6 50</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
