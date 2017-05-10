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
         <li>This parameter works with MS2, mzXML and mzML inputs files.
         <li>Two digits are specified for this parameter.  The first digit is the start scan and the
         second digit is the end scan.
         <li>You can set either just the start scan (leaving end scan 0) or just the end scan
         (leaving start scan 0).  
         <li>When the end scan is less than the start scan, no scan can satisfy that scan range
         so no spectra will be searched.
         <li>The default value is "0 0" if this parameter is missing. The entire file will be searched
         with a "0 0" scan setting.
         <li>Any time a non-zero value is specified for either the start scan or the end scan, the
         output files will have the scan range encoded in the output file name.
         </ul>

         <p>Example:
         <br><tt>scan_range = 0 0</tt> &nbsp; &nbsp; <i>search all scans</i>
         <br><tt>scan_range = 0 1000</tt> &nbsp; &nbsp; <i>search scans up to 1000</i>
         <br><tt>scan_range = 2000 0</tt> &nbsp; &nbsp; <i>search scans from 2000 to last scan in file</i>
         <br><tt>scan_range = 1000 1500</tt> &nbsp; &nbsp; <i>search only scans 1000 to 1500</i>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
