<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: output_mzidentmlfile</h2>

         <ul>
         <li>Controls whether to output search results in an mzIdentML file.
         <li>Valid values are 0 (do not output) or 1 (output).
         <li>The default value is "1" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>output_mzidentmlfile = 0</tt>
         <br><tt>output_mzidentmlfile = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
