<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: use_NL_ions</h2>

         <ul>
         <li>Controls whether or not neutral loss ions (-NH3 and -H2O from b- and y-ions) are considered in the search.
         <li>Valid values are 0 and 1.
         <li>To not use neutral loss ions, set the value to 0.
         <li>To use neutral loss ions, set the value to 1.
         </ul>

         <p>Example:
         <br><tt>use_NL_ions = 0</tt>
         <br><tt>use_NL_ions = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
