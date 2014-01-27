<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: show_fragment_ions</h2>

         <ul>
         <li>This parameter affects .out files only i.e.
         <a href="output_outfiles.php">output_outfiles</a> set to 1.
         <li>This parameter controls whether or not the theoretical
         fragment ion masses for the top peptide hit are calculated
         and dislayed at the end of an .out file.
         <li>Valid values are 0 and 1.
         </ul>

         <p>Example:
         <br><tt>show_fragment_ions = 0</tt>
         <br><tt>show_fragment_ions = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
