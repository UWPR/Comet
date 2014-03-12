<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: output_suffix</h2>

         <ul>
         <li>This parameter specifies the suffix string that is appended to
         the base output name for the pep.xml, pin.xml, txt and sqt output files.
         <li>Use this parameter to give output files a unique suffix base name.
         <li>For example, if the outout_suffix parameter is set to
         "output_suffix = _000", then a search of the file base.mzXML
         will generate output files named base_000.pep.xml, base_000.pin.xml,
         base_000.txt, and/or base_000.sqt.
         <li>Note that using this parameter could break downstream tools that
         expect the output base name to be the same as the input file base name.
         <li>The default value is blank if this parameter is missing i.e.
         base.mzXML will generate base.pep.xml.
         </ul>

         <p>Example:
         <br><tt>output_suffix = </tt>
         <br><tt>output_suffix = _some_suffix</tt>
         <br><tt>output_suffix = any_string_you_want_without_spaces</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
