<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: max_duplicate_proteins</h2>

         <ul>
         <li>This parameter defines the maximum number of proteins (identifiers/accessions)
         to report.  Some peptides sequences, especially short ones,  are not very specific
         and can be found in a large number of sequence entries.  In these situations, this
         parameter can be used to keep the reported protein list short.
         <li>Valid values are any integer greater than or equal to 0.
         <li>If set to "0", there will be no limit on the number of reported proteins.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>max_duplicate_proteins = 0</tt>
         <br><tt>max_duplicate_proteins = 20</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
