<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: fragment_bin_tol</h2>

         <ul>
         <li>This parameter controls the bin size associated with fragment ions.
         <li>The bin size defines the mass width associated with a single MS/MS peak
         as it is stored internally in an array element.
         <li>Although it's not explicitly a fragment ion tolerance, it's probably
         easiest to think of it as such.
         <li>Note, there is a direct correlation between the value selected for
         the fragment_bin_tol and the memory used in a search.  The lower the
         fragment_bin_tol setting, the more memory a search will use.  A test of
         4,515 query spectra used 724MB RAM with a 0.36 fragment_bin_tol value,
         1.4GB RAM with a 0.10 value, and 9.6GB RAM with a 0.01 value.
         </ul>

         <p>Example:
         <br><tt>fragment_bin_tol = 0.36</tt>
         <br><tt>fragment_bin_tol = 0.03</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
