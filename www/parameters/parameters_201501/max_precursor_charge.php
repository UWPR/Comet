<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: max_precursor_charge</h2>

         <ul>
         <li>This parameter defines the maximum precursor charge state that
         will be analyzed.
         <li>Only spectra with this number of precursor charges or less will be searched.
         <li>Valid values are any integer greater than 1.
         <li>The default value is "6" if this parameter is missing.  A maximum
         allowed value of "9" is enforced for this parameter.
         </ul>

         <p>Example:
         <br><tt>max_precursor_charge = 5</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
