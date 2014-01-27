<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: num_results</h2>

         <ul>
         <li>This parameter controls the number of peptide search results that
         are stored internally.
         <li>Depending on what post-processing tools are used, one may want to
         set this to the same value as <a href="num_output_lines.php">num_output_lines</a>.
         <li>When this parameter is set to a value greater than
         <a href="num_output_lines.php">num_output_lines</a>, it allows
         the SpRank value to span a larger range which may be helpful for
         tools like PeptideProphet or Percolator (not likely though).
         <li>The maximum num_results value possible is hardcoded into Comet and
         is currently 100.
         <li>Valid values are any integer greater than zero.
         </ul>

         <p>Example:
         <br><tt>num_results = 10</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
