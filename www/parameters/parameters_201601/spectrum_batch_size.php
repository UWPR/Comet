<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: spectrum_batch_size</h2>

         <ul>
         <li>When this parameter is set to a non-zero value, say 5000, this causes Comet
         to load and search about 5000 spectra at a time, looping through sets of 5000
         spectra until all data have been analyzed.
         <li>This parameter was implemented to simplify searching large datasets that
         might not fit into memory if searched all at once.
         <li>The loaded batch sizes might be a little larger than the specified parameter
         value (i.e. 5014 spectra loaded when the parameter is set to 5000) due to both
         threading and potential charge state considerations when precursor charge state
         is not known.
         <li>Valid values are 0 or any positive integer.
         <li>Set this parameter to 0 to load and search all spectra at once.
         <li>Set this parameter to any other positive integer to loop through searching
         this number of spectra at a time until all spectra have been analyzed.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>spectrum_batch_size = 0</tt>
         <br><tt>spectrum_batch_size = 1000</tt>
         <br><tt>spectrum_batch_size = 5000</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
