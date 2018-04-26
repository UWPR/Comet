<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: peff_verbose_output</h2>

         <ul>
         <li>Specifies whether the verbose output is reported during a PEFF search.  Examples of
             the reporting includes not finding an entry in the OBO file, amino acid variant
             same as original residue, invalid mod/variant amino acid position, etc.
         <li>Valid values are 0 and 1.
         <li>To suppress verbose output, set the value to 0.
         <li>To show verbose output, set the value to 1.
         <li>The default value is "0" if this parameter is missing.
         <li>This is a hidden parameter that is not included in the parameters file generated
             by "comet -p".  You must manually add this parameter if you want to set it.
         </ul>

         <p>Example:
         <br><tt>peff_verbose_output = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
