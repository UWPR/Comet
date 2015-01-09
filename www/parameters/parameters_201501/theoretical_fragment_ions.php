<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: theoretical_fragment_ions</h2>

         <ul>
         <li>This parameter specifies how theoretical fragment ion peaks are
         represented.
         <li>Even though Comet does not generate/store a theoretical spectrum,
         it does calculate fragment ion masses and this parameter controls how
         the acquired spectrum intensities at these theoretical mass locations
         contribute to the correlation score.
         <li>A value of 0 indicates that the fast correlation score will be
         a sum of the intensities at each theortical fragment mass bin and half
         the intensity of each flanking bin.
         <li>A value of 1 indicates that the fast correlation score will be
         the sum of the intensities at each theoretical fragment mass bin.
         <li>For extremely coarse <a href="fragment_bin_tol.php">fragment_bin_tol</a>
         values such as the historical ~1 Da bins, a theoretical_fragment_ions
         value of 1 is optimal.
         <li>But for narrower bins, such as ~0.3 for ion trap data or ~0.03 for
         high-res MS/MS spectra, a value of 0 is optimal to incorporate
         intensities from the flanking bins.
         <li>Allowed values are 0 or 1.
         <li>The default value is "1" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>theoretical_fragment_ions = 0</tt>
         <br><tt>theoretical_fragment_ions = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
