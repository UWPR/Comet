<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: allowed_missed_cleavage</h2>

         <ul>
         <li>Number of allowed missed enzyme cleavages in a peptide.
         <li>Parameter is not applied of the no-enzyme option is specified
         in the <a href="search_enzyme_number.php">search_enzyme_number</a>
         parameter.
         <li>The default value is "2" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>allowed_missed_cleavage = 0</tt> &nbsp; &nbsp; <i>for no missed cleavages</i>
         <br><tt>allowed_missed_cleavage = 2</tt> &nbsp; &nbsp; <i>allow two missed cleavages</i>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
