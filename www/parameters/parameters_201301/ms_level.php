<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: ms_level</h2>

         <ul>
         <li>This parameter specifies which scans are searched.
         <li>An input value of 2 will search MS/MS scans.
         <li>An input value of 3 will search MS^3 scans.
         <li>This parameter is only valid for mzXML, mzML, and mz5 input files.
         <li>Allowed values are 2 or 3.
         </ul>

         <p>Example:
         <br><tt>ms_level = 2</tt>
         <br><tt>ms_level = 3</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
