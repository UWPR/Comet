<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: skip_researching</h2>

         <ul>
         <li>This parameter is valid only when <a href="output_outfiles.php">output_outfiles</a> is set
         to 1 and each of
         <a href="output_pepxmlfile.php">output_pepxmlfile</a>,
         <a href="output_sqtfile.php">output_sqtfile</a>, and
         <a href="output_sqtstream.php">output_sqtstream</a>  are set to 0.
         <li>When .out files only are set to be exported, this parameter will look to see if
         an .out file already exists for each query spectrum.  If so, it will not re-search
         that particular spectrum.
         <li>When set to 0, all spectra are re-searched.  When set to 1, the search is skipped
         for those spectra where an .out file already exists.
         <li>Valid values are 0 and 1.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>skip_researching = 0</tt>
         <br><tt>skip_researching = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
