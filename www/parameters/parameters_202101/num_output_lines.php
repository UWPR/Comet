<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: num_output_lines</h2>

         <ul>
         <li>This parameter controls the number of search result
         hits (peptides) that are reported for each spectrum query.
         <li>If you are only interested in seeing one top hit each
         per query, set this value to 1.
         <li>This parameter value cannot be larger than the value
         entered for "<a href="num_results.php">num_results</a>"
         which itself is limited to a maximum value of 100.
         <li>Valid values are any positive integer 1 or greater.
         <li>If a value less than 1 is entered, this parameter is set to 1.
         <li>The default value is "5" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>num_output_lines = 1</tt>
         <br><tt>num_output_lines = 5</tt>
         <br><tt>num_output_lines = 10</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
