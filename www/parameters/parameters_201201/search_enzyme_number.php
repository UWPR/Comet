<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: search_enzyme_number</h2>

         <ul>
         <li>The search enzyme is specified by this parameter.
         <li>The list of search enzymes is specified at the end of the comet.params file
         beginning with the line <tt>[COMET_ENZYME_INFO]</tt>.  The actual enzyme list and
         digestion parameters are read here.  So one can edit/add/delete enzyme definitions
         simply be changing the enzyme information.
         <li>This parameter works in conjection with the <a href="num_enzyme_termini.php">num_enzyme_termini</a>
         parameter to define the cleavage rule for fully-digested vs. semi-digested search options.
         <li>This parameter works in conjection with the <a href="allowed_missed_cleavage.php">allowed_missed_cleavage</a>
         parameter to define the miss cleavage rule.


         <p>Example:
         <br><tt>search_enzyme_number = 0</tt> &nbsp; &nbsp; <i>typically no-enzyme</i>
         <br><tt>search_enzyme_number = 1</tt> &nbsp; &nbsp; <i>typically trypsin</i>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
