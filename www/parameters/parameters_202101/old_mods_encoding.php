<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: old_mods_encoding</h2>

         <ul>
         <li>This parameter enables using the old character based modification encodings
             (e.g. DLYM*NCK) instead mass based encodings (e.g. DLYM[15.9949]NCK) in the
             SQT output files.
         <li>A value of "1" will cause Comet to use the old character based modification
             encodings (e.g. DLYM*NCK).
         <li>A value of "0" will cause Comet to use the mass based modification
             encodings (e.g. DLYM[15.9949]NCK).
         <li>This parameter affects SQT output files only.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>old_mods_encoding = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
