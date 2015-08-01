<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: sample_enzyme_number</h2>

         <ul>
         <li>This parameter is relevant only for pepXML output i.e. when
         <a href="output_pepxmlfile.php">output_pepxmlfile</a> is set to 1.
         <li>The pepXML format encodes the enzyme that is applied to the sample
         i.e. trypsin.  This enzyme is written to the "sample_enzyme" element.
         <li>The sample enzyme could be different from the search enzyme i.e.
         the sample enzyme is "trypsin" yet the search enzyme is "No-enzyme"
         for a non-specific search.  Hence the need for this separate parameter.
         <li>Valid values are any integer represented in the enzyme list.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>sample_enzyme_number = 1</tt>
         <br><tt>sample_enzyme_number = 3</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
