<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: override_charge</h2>

         <ul>
         <li>This parameter specifies the whether to override existing precursor
         charge state information when present in the files with the charge
         range specified by the
         "<a href="precursor_charge.php">precursor_charge</a>" parameter.
         <li>Valid values are 0 and 1.
         <li>To keep any known charge state values in the input files,
         set the value to 0.
         <li>To ignore any known charge values in the input files and instead
         use the charge state range specified by the
         "<a href="precursor_charge.php">precursor_charge</a>" parameter,
         set the value to 1.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>override_charge = 0</tt>
         <br><tt>override_charge = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
