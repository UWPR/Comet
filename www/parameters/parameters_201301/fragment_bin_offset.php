<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: fragment_bin_offset</h2>

         <ul>
         <li>This parameter controls how each fragment bin of size <a href="fragment_bin_tol.php">fragment_bin_tol</a>
         is defined in terms of where each bin starts.
         <li>For example, assume a <a href="fragment_bin_tol.php">fragment_bin_tol</a> of 1.0.  Most intuitively,
         the fragment bins would be 0.0 to 1.0, 1.0 to 2.0, 2.0 to 3.0, etc.
         This set of bins corresponds to a fragment_bin_offset of 0.0.  However,
         consider if we set fragment_bin_offset to 0.5; this would cause the
         bins to be 0.5 to 1.5, 1.5 to 2.5, 2.5 to 3.5, etc.
         <li>So this fragment_bin_offset gives one a mechanism to define
         where each bin starts and is centered.
         <li>For ion trap data with a <a href="fragment_bin_tol.php">fragment_bin_tol</a> of 1.0005,
         it is recommended to set fragment_bin_offset to 0.4.
         <li>For high-res MS/MS data, one might use a <a href="fragment_bin_tol.php">fragment_bin_tol</a> of 0.02
         and a corresponding fragment_bin_offset of 0.0.
         <li>Allowed values are between 0.0 and 1.0.  The actual offset value is
         scaled by the <a href="fragment_bin_tol.php">fragment_bin_tol</a> value.
         <li>I know this is esoteric and any normal user should not give this
         parameter any thought beyond using the recommended settings.
         </ul>

         <p>Example:
         <br><tt>fragment_bin_offset = 0.4</tt>
         <br><tt>fragment_bin_offset = 0.0</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
