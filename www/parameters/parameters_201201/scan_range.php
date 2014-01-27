<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: scan_range</h2>

         <ul>
         <li>Defines the scan range to search.  Only spectra within (and inclusive) of the specified
         scan range are searched.
         <li>This parameter works only with mzXML and mzML inputs files.
         <li>Two digits are specified for this parameter.  The first digit is the start scan and the
         second digit is the end scan.
         <li>When the start scan is set to 0, this parameter setting is ignored irrespective of what
         the end scan is set to.
         <li>When the end scan is less than the start scan, this parameter setting is ignored.
         </ul>

         <p>Example:
         <br><tt>scan_range = 0 0</tt> &nbsp; &nbsp; <i>search all scans</i>
         <br><tt>scan_range = 0 1000</tt> &nbsp; &nbsp; <i>search all scans (because first entry is 0)</i>
         <br><tt>scan_range = 1000 1500</tt> &nbsp; &nbsp; <i>search only scans 1000 to 1500</i>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
