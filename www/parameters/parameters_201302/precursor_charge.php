<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: precursor_charge</h2>

         <ul>
         <li>This parameter specifies the precursor charge range to search.
         <li>This parameter expects to integer values as input.
         <li>If the first input value is 0 then this parameter is ignored and all charge
         states are searched
         <li>If the first input value is not 0 then all charge states between (and inclusive of)
         the first and second input values are searched.
         <li>The default value is "0 0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>precursor_charge = 0 0</tt> &nbsp; &nbsp; <i>search all charge ranges</i>
         <br><tt>precursor_charge = 0 2</tt> &nbsp; &nbsp; <i>search all charge ranges (because first entry is 0)</i>
         <br><tt>precursor_charge = 2 6</tt> &nbsp; &nbsp; <i>search 2+ to 6+ precursors</i>
         <br><tt>precursor_charge = 3 3</tt> &nbsp; &nbsp; <i>search 3+ precursors</i>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
