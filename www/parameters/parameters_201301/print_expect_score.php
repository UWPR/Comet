<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: print_expect_score</h2>

         <ul>
         <li>A boolean flag this determines whether or not the expectation
         score (E-value) is reported in .out and SQT formats.  Note that the
         E-value is always reported in pepXML output.
         <li>This parameter is only pertinant for results reported in .out and SQT formats.
         <li>If expect scores are chosen to be reported (i.e. value set to 1), they will replace
         the number reported for the traditional "spscore" i.e. "spscore" will
         be replaced by an E-value.  Also an expectation value histogram will
         be output at the end of each .out file; this histogram is not present
         for SQT output.
         <li>Valid values are 0 and 1.
         </ul>

         <p>Example:
         <br><tt>print_expect_score = 0</tt>
         <br><tt>print_expect_score = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
