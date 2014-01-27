<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: output_pinxmlfile</h2>

         <ul>
         <li>Controls whether to output search results in a Percolator-IN or "pin.xml" file.
         pin.xml files are used as inputs to <a href="http://per-colator.com">Percolator</a>.
         <li>When you choose to output a pin.xml file, you must also be performing a
         <a href="decoy_search.php">decoy_search</a> or else the search will not run
         and an error will be reported.
         <li>The pin.xml format specifies the UniMod accession number for each modification.
         As this information is not available, Comet will write bogus values for the
         UniMod accession.  Each variable modification will have the variable modification
         number (1 thru 6) as the UniMod accession.  Each static modification will have
         the ASCII value of the residue as the UniMod accession.  If you really care about
         having valid UniMod accession numbers, you'll have to handle the find and replace
         yourself on the produced pin.xml file.
         <li>Valid values are 0 (do not output) or 1 (output).
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>output_pinxmlfile = 0</tt>
         <br><tt>output_pinxmlfile = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
