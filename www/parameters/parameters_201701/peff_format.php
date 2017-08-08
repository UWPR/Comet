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
         <li>Valid values are 0, 1 and 2.
         <li>Set this parameter to 0 to search a normal FASTA file, ignoring any PEFF headers
         <li>Set this parameter to 1 to search PEFF amino acid variants and PSI-MOD modifications.
         <li>Set this parameter to 2 to search PEFF amino acid variants and Unimod modifications.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>peff_format = 0</tt>
         <br><tt>peff_format = 1</tt>
         <br><tt>peff_format = 2</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
