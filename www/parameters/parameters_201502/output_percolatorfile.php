<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: output_percolatorfile</h2>

         <ul>
         <li>Controls whether to output search results in a <a href="http://per-colator.com">Percolator's</a>
         tab-delimited input format.
         <li>Valid values are 0 (do not output) or 1 (output).
         <li>The default value is "0" if this parameter is missing.
         <li>The created file will have a ".tsv" file extension.
         <li>This parameter replaces the now defunct "output_pinxmlfile".
         </ul>

         <p>Example:
         <br><tt>output_percolatorfile = 0</tt>
         <br><tt>output_percolatorfile = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
