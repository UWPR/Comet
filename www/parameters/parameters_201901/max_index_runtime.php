<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: max_index_runtime</h2>

         <ul>
         <li>This parameter sets the maximum indexed database search run time for a
             scan/query.
         <li>Valid values are integers 0 or higher representing the maximum run time
             in milliseconds.
         <li>As Comet loops through analyzing peptides from the database index file,
             it checks the cummulative run time of that spectrum search after each
             peptide is analyzed.  If the run time exceeds the value set for this
             parameter, the search is aborted and the best peptide result analyzed
             up to that point is returned.
         <li>To have no maximum search time, set this parameter value to "0".
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>max_index_runtime = 0</tt>
         <br><tt>max_index_runtime= 150</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
