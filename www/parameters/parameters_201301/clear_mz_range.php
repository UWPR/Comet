<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: clear_mz_range</h2>

         <ul>
         <li>This parameter is intended for iTRAQ/TMT type data where one might
         want to remove the reporter ion signals in the MS/MS spectra prior to searching.
         <li>Defines the m/z range to clear out in each MS/MS spectra
         <li>This parameter has two decimal values.
         <li>The first value is the lower mass cutoff and the second value is
         the high mass cutoff.
         <li>Valid values are two decimal numbers where the first number must
         be less or equal to the second number.
         <li>All peaks between the two decimal values are cleared out.
         </ul>

         <p>Example:
         <br><tt>clear_mz_range = 0.0 0.0</tt> &nbsp; &nbsp; <i>parameter is ignored</i>
         <br><tt>clear_mz_range = 112.5 121.5</tt> &nbsp; &nbsp; <i>ignore all peaks between 112.5 and 121.5 m/z for iTRAQ 8-plex</i>
         <br><tt>clear_mz_range = 125.5 131.5</tt> &nbsp; &nbsp; <i>ignore all peaks between 125.5 and 131.5 m/z for TMT</i>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
