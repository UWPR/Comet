<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: peff_format</h2>

         <ul>
         <li>Specifies whether the database is a PEFF file or normal FASTA.
         <li>Valid values are 0 and 1.
         <li>To search a normal FASTA file, set the value to 0.
         <li>To search a PEFF file, set the value to 1.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>peff_format = 0</tt>
         <br><tt>peff_format = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
