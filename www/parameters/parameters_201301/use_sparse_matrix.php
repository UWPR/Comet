<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: use_sparse_matrix</h2>

         <ul>
         <li>Controls whether or not internal sparse matrix data representation is used.
         <li>The sparse matrix data representation will use a <em>significantly</em> smaller amount
         of memory/RAM for small <a href="fragment_bin_tol.php">fragment_bin_tol</a> settings such
         as 0.05 or 0.01.  We're talking going from tens of GB (gigabytes) down to a few hundred
         megabytes (MB)!
         <li>In this release, the sparse matrix searches will always be slower than the classical
         data representation (i.e. use_sparse_matrix = 0).  So it should be used only when
         memory is an issue.  Alternately, the <a href="spectrum_batch_size.php">spectrum_batch_size</a>
         parameter can also be used to mitigate memory issues.
         <li>Valid values are 0 and 1.
         <li>To not use sparse matrix, set the value to 0.
         <li>To use sparse matrix, set the value to 1.
         </ul>

         <p>Example:
         <br><tt>use_sparse_matrix = 0</tt>
         <br><tt>use_sparse_matrix = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
