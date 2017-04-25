<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: peff_obo</h2>

         <ul>
         <li>A full or relative path to the OBO file used with a PEFF search.
         <li>Supported OBO formats are PSI-Mod and Unimod OBO files.  Which OBO file you
         use depends on your PEFF input file.
         <li>This parameter is ignored if "<a href="peff_format.php">peff_format = 0</a>".
         <li>There is no default value if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>peff_obo = /usr/local/obo/PSI-MOD.obo</tt>
         <br><tt>peff_obo = c:\local\obo\PSI-MOD.obo</tt>
         <br><tt>peff_obo = PSI-MOD.obo</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
