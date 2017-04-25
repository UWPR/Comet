<?php include "head.php" ; ?>
<body>

<?php include "analyticstracking.php" ; ?>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post">
         <h1>Notes 2016.01.01</h1>

            <div class="post hr">
               <p>To create a comet.params file, run the following command and rename the create
                  file from "comet.params.new" to "comet.params".
               <ul>
                  <li><tt>comet.exe -p</tt>
               </ul>

               Better yet, just download them from this site.  Example version 2016.01 comet.params files (primary differences are the MS and MS/MS mass tolerance settings):
               <ul>
                  <li><a href="/parameters/parameters_201601/comet.params.low-low">comet.params.low-low</a> - low res MS1 and low res MS2 e.g. ion trap
                  <li><a href="/parameters/parameters_201601/comet.params.high-low">comet.params.high-low</a> - high res MS1 and low res MS2 e.g. LTQ-Orbitrap
                  <li><a href="/parameters/parameters_201601/comet.params.high-high">comet.params.high-high</a> - high res MS1 and high res MS2 e.g. Q Exactive or Q-Tof
               </ul>

               <p>NOTE:  These links might not be updated to point to parameter files from the latest version of Comet.
               To get the latest versions, just click on the "parameters" tab above and access the example parameter files there.</p>
            </div>

            <div class="post hr">
               <p>For low-res ms/ms spectra, try the following settings:
               <ul>
                  <li><a href="/parameters/parameters_201601/fragment_bin_tol.php">fragment_bin_tol</a> = 1.0005
                  <li><a href="/parameters/parameters_201601/fragment_bin_offset.php">fragment_bin_offset</a> = 0.4
                  <li><a href="/parameters/parameters_201601/theoretical_fragment_ions.php">theoretical_fragment_ions</a> = 1
                  <li><a href="/parameters/parameters_201601/spectrum_batch_size.php">spectrum_batch_size</a> = 0
               </ul>

               <p>For high-res ms/ms spectra, try the following settings:
               <ul>
                  <li><a href="/parameters/parameters_201601/fragment_bin_tol.php">fragment_bin_tol</a> = 0.02
                  <li><a href="/parameters/parameters_201601/fragment_bin_offset.php">fragment_bin_offset</a> = 0.0
                  <li><a href="/parameters/parameters_201601/theoretical_fragment_ions.php">theoretical_fragment_ions</a> = 0
                  <li><a href="/parameters/parameters_201601/spectrum_batch_size.php">spectrum_batch_size</a> = 10000 (depending on free memory)
               </ul>
            </div>
      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>

</body>
</html>

