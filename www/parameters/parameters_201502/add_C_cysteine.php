<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: add_C_cysteine</h2>

         <ul>
         <li>Specify a static modification to the residue C.
         <li>The specified mass is added to the unmodified mass of C.
         <li>The default value is "0.0" if this parameter is missing.
         If Comet was compiled for
         <a href="noble.gs.washington.edu/proj/crux/">Crux</a> compatibility,
         the default value is "57.021464".
         Note that in the example parameter file produced by Comet using the "-p"
         command line option, this parameter entry has a value of 57.021464 for
         cysteine carbamidomethylation.
         </ul>

         <p>Example:
         <br><tt>add_C_cysteine = 57.021464</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
