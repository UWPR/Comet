<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: output_sqt_stream</h2>

         <ul>
         <li>Controls whether to output search results to standard out
         (i.e. to the screen unless otherwise directed) in SQT format.
         <li>Just the search results (M and L lines and not any H lines)
         are output to standard out.
         <li>Valid values are 0 (do not output) or 1 (output).
         </ul>

         <p>Example:
         <br><tt>output_sqt_stream = 0</tt>
         <br><tt>output_sqt_stream = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
