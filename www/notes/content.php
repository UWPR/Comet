<div id="page">
   <div id="content_full">
      <div class="post">
         <h1>Miscellaneous Notes</h1>

            <div class="post hr">
               <p>Support:
                  <ul>
                     <li><a target="new" href="http://groups.google.com/group/comet-ms">Comet's Google group</a>.
                     Post your questions, problems, and feature requests here.
                     <li>If you would rather not post to this public forum, feel
                     free to e-mail me directly (jke000 at gmail.com).
                  </ul>
            </div>

            <div class="post hr">
               <p>To run Comet:
               <ul>
                  <li><tt>comet.exe input.mzXML</tt>
                  <li><tt>comet.exe input.mzXML:1000-3000</tt> &nbsp; &nbsp; <i>(searches only scans 1000 to 3000)</i>
                  <li><tt>comet.exe input.ms2</tt>
               </ul>

               <p>Note specifying the scan range to search on the command line, 2nd example above,
                  works only for mzXML and mzML input files.  One can also specify searching a specific
                  scan range using the "<a href="/parameters/scan_range.php"><tt>scan_range</tt></a>" parameter in the comet.params file.
               </div>

            <div class="post hr">
               <p>For low-res ms/ms spectra, try the following settings:
               <ul>
                  <li><a href="/parameters/parameters_201302/fragment_bin_tol.php">fragment_bin_tol</a> = 1.0005
                  <li><a href="/parameters/parameters_201302/fragment_bin_offset.php">fragment_bin_offset</a> = 0.4
                  <li><a href="/parameters/parameters_201302/theoretical_fragment_ions.php">theoretical_fragment_ions</a> = 1
                  <li><a href="/parameters/parameters_201302/spectrum_batch_size.php">spectrum_batch_size</a> = 0
               </ul>

               <p>For high-res ms/ms spectra, try the following settings:
               <ul>
                  <li><a href="/parameters/parameters_201302/fragment_bin_tol.php">fragment_bin_tol</a> = 0.05
                  <li><a href="/parameters/parameters_201302/fragment_bin_offset.php">fragment_bin_offset</a> = 0.0
                  <li><a href="/parameters/parameters_201302/theoretical_fragment_ions.php">theoretical_fragment_ions</a> = 0
                  <li><a href="/parameters/parameters_201302/spectrum_batch_size.php">spectrum_batch_size</a> = 5000 (depending on available RAM)
               </ul>
            </div>

            <div class="post hr">
               <p>Regarding high-res ms/ms searches (i.e. narrow fragment_bin_tol settings):
                  On my system (linux, 8-core, 16GB RAM), searching ~70K Q-Exactive spectra will complete
                  significantly faster when I make use of the batch search option (iterating through
                  batches of spectra) instead of using the sparse matrix (minimizing memory use, allowing
                  all spectra to be loaded and searched at once).
               <p>So for high-res ms/ms, I typically set
                  "<a href="/parameters/parameters_201302/use_sparse_matrix.php">use_sparse_matrix</a> = 0" and
                  "<a href="/parameters/parameters_201302/spectrum_batch_size.php">spectrum_batch_size</a> = 5000"
                  (as opposed to "<a href="/parameters/parameters_201302/use_sparse_matrix.php">use_sparse_matrix</a> = 1" and
                   "<a href="/parameters/parameters_201302/spectrum_batch_size.php">spectrum_batch_size</a>= 0").
            </div>

            <div class="post hr">
               <p>To generate a comet.params file appropriate for your Comet binary, issue the command "comet -p".
               <br>Example version 2013.02 comet.params files (primary differences are the MS and MS/MS mass tolerance settings):
               <ul>
                  <li><a href="/parameters/parameters_201302/comet.params.low-low">comet.params.low-low</a> - low res MS1 and low res MS2 e.g. ion trap
                  <li><a href="/parameters/parameters_201302/comet.params.high-low">comet.params.high-low</a> - high res MS1 and low res MS2 e.g. LTQ-Orbitrap
                  <li><a href="/parameters/parameters_201302/comet.params.high-high">comet.params.high-high</a> - high res MS1 and high res MS2 e.g. Q Exactive or Q-Tof
               </ul>
            </div>

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
