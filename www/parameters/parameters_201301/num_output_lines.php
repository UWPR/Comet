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
         <li>If you are only interested in seeing one top each per
         query, set this value to 1.
         <li>The maximum possible number of reported hits is hardcoded
         into each binary and that value is currently set to 100.
         <li>Valid values are any positive integer 1 or greater.
         </ul>

         <p>Example:
         <br><tt>num_output_lines = 1</tt>
         <br><tt>num_output_lines = 10</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
