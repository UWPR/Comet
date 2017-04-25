<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: nucleotide_reading_frame</h2>

         <ul>
         <li>This parameter is used to search nucleotide sequence databases.
         <li>It controls how the nucleotides are translated specifically
         which sets of reading frames are translated.
         <li>Valid values are 0 through 9.
         <li>Set this parameter to 0 for a protein sequence database.
         <li>Set this parameter to 1 to search the 1st forward reading frame.
         <li>Set this parameter to 2 to search the 2nd forward reading frame.
         <li>Set this parameter to 3 to search the 3rd forward reading frame.
         <li>Set this parameter to 4 to search the 1st reverse reading frame.
         <li>Set this parameter to 5 to search the 2nd reverse reading frame.
         <li>Set this parameter to 6 to search the 3rd reverse reading frame.
         <li>Set this parameter to 7 to search all 3 forward reading frames.
         <li>Set this parameter to 8 to search all 3 reverse reading frames.
         <li>Set this parameter to 9 to search all 6 reading frames.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>nucleotide_reading_frame = 0</tt>
         <br><tt>nucleotide_reading_frame = 9</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
