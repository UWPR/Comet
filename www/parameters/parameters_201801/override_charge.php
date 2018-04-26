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
         <li>This parameter specifies whether or not to override existing precursor
         charge state information when present in the files with the charge
         range specified by the
         "<a href="precursor_charge.php">precursor_charge</a>" parameter.
         <li>Valid values are 0, 1, 2, and 3:
            <ul>
            <li>0 = keep any known precursor charge state values in the input files
            <li>1 = ignore known precursor charge state values in the input files 
                and instead use the charge state range specified by the
                "<a href="precursor_charge.php">precursor_charge</a>" parameter
            <li>2 = only search precursor charge state values that are within the
                range specified by the 
                "<a href="precursor_charge.php">precursor_charge</a>" parameter
            <li>3 = keep any known precursor charge state values. For unknown
                charge states, search as singly charged if there is no
                signal above the precursor m/z or use the
                "<a href="precursor_charge.php">precursor_charge</a>" range otherwise.
            </ul>
         </ul>

         <p>Example:
         <br><tt>override_charge = 0</tt>
         <br><tt>override_charge = 1</tt>
         <br><tt>override_charge = 2</tt>
         <br><tt>override_charge = 3</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
