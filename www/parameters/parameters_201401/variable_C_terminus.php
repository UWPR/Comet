<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: variable_C_terminus</h2>

         <ul>
         <li>Specify a variable modification to peptide's c-terminus.
         <li>Works in conjunction with <a href="variable_C_terminus_distance.php">variable_C_terminus_distance</a>
             to specify scope of which peptides this parameter is applied to.
         <li>The default value is "0.0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>variable_C_terminus = 14.0</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
