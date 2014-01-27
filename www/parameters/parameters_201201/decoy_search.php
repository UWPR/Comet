<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: decoy_search</h2>

         <ul>
         <li>This parameter controls whether or not an internal decoy search is performed.
         <li>Valid parameter values are 0, 1, or 2:
            <ul>
            <li>0 = no decoy search (default)
            <li>1 = concatenated decoy search.  Target and decoy entries will be scored against
            each other and a single result is performed for each spectrum query.
            <li>2 = separate decoy search.  Target and decoy entries will be scored separately
            and separate target and decoy search results will be reported.
            </ul>
         </ul>

         <p>Example:
         <br><tt>decoy_search = 0</tt>
         <br><tt>decoy_search = 1</tt>
         <br><tt>decoy_search = 2</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
