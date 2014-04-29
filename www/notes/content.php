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
                     free to e-mail me directly (jke000 at gmail dot com).
                  </ul>
            </div>
            <div class="post hr">
               <p>We appear to have Windows memory management issues.  This is limited to Windows and
                  is not an issue for Linux.  Avoid specifying multiple
                  input files to Comet when run under windows i.e. do not run "<tt>comet.exe *.mzXML</tt>".

               <p>Instead, create a wrapper script batch program which is just a text file with a ".bat"
                  file extension.  Name it something like "runcomet.bat" with the contents as below
                  and execute it instead of executing Comet directly i.e. "<tt>runcomet.bat *.mzXML</tt>".
                  By running such a batch script, it will invoke individual instances of Comet to
                  search all input files as opposed to one instance of Comet to search all input
                  files.

               <p>Contents of runcomet.bat: &nbsp; <tt>for %%A in (%*) do {comet.exe %%A}</tt>

            </div>
            <div class="post hr">
               <p>To run Comet, you need one or more input spectral files in mzXML, mzML, ms2/cms2 formats
                  and a comet.params file.  Then issue a command such as:
               <ul>
                  <li><tt>comet.exe input.mzXML</tt>
                  <li><tt>comet.exe input.mzXML:1000-3000</tt> &nbsp; &nbsp; <i>(searches only scans 1000 to 3000)</i>
                  <li><tt>comet.exe input.mzML</tt>
                  <li><tt>comet.exe input.ms2</tt>
               </ul>

               <p>Note specifying the scan range to search on the command line, 2nd example above,
                  works only for mzXML and mzML input files.  One can also specify searching a specific
                  scan range using the "<a href="/parameters/scan_range.php">scan_range</a>" parameter in the comet.params file.
            </div>
            <div class="post hr">
               <p>To create a comet.params file, run the following command and rename the create
                  file from "comet.params.new" to "comet.params".
               <ul>
                  <li><tt>comet.exe -p</tt>
               </ul>
            </div>

            <div class="post hr">
               <p>For low-res ms/ms spectra, try the following settings:
               <ul>
                  <li><a href="/parameters/parameters_201401/fragment_bin_tol.php">fragment_bin_tol</a> = 1.0005
                  <li><a href="/parameters/parameters_201401/fragment_bin_offset.php">fragment_bin_offset</a> = 0.4
                  <li><a href="/parameters/parameters_201401/theoretical_fragment_ions.php">theoretical_fragment_ions</a> = 1
                  <li><a href="/parameters/parameters_201401/spectrum_batch_size.php">spectrum_batch_size</a> = 0
               </ul>

               <p>For high-res ms/ms spectra, try the following settings:
               <ul>
                  <li><a href="/parameters/parameters_201401/fragment_bin_tol.php">fragment_bin_tol</a> = 0.05
                  <li><a href="/parameters/parameters_201401/fragment_bin_offset.php">fragment_bin_offset</a> = 0.0
                  <li><a href="/parameters/parameters_201401/theoretical_fragment_ions.php">theoretical_fragment_ions</a> = 0
                  <li><a href="/parameters/parameters_201401/spectrum_batch_size.php">spectrum_batch_size</a> = 5000 (depending on available RAM)
               </ul>
            </div>

            <div class="post hr">
               <p>Regarding high-res ms/ms searches (i.e. narrow fragment_bin_tol settings):
                  On my system (linux, 8-core, 16GB RAM), searching ~70K Q-Exactive spectra will complete
                  significantly faster when I make use of the batch search option (iterating through
                  batches of spectra) instead of using the sparse matrix (minimizing memory use, allowing
                  all spectra to be loaded and searched at once).
               <p>So for high-res ms/ms, I typically set:
               <ul>
                  <li><a href="/parameters/parameters_201401/use_sparse_matrix.php">use_sparse_matrix</a> = 0
                  <li><a href="/parameters/parameters_201401/spectrum_batch_size.php">spectrum_batch_size</a> = 5000
               </ul>
               <p>as opposed to
               <ul>
                  <li><a href="/parameters/parameters_201401/use_sparse_matrix.php">use_sparse_matrix</a> = 1
                  <li><a href="/parameters/parameters_201401/spectrum_batch_size.php">spectrum_batch_size</a> = 0
               </ul>
            </div>

            <div class="post hr">
               <p>To generate a comet.params file appropriate for your Comet binary, issue the command "comet -p".
               <br>Example version 2014.01 comet.params files (primary differences are the MS and MS/MS mass tolerance settings):
               <ul>
                  <li><a href="/parameters/parameters_201401/comet.params.low-low">comet.params.low-low</a> - low res MS1 and low res MS2 e.g. ion trap
                  <li><a href="/parameters/parameters_201401/comet.params.high-low">comet.params.high-low</a> - high res MS1 and low res MS2 e.g. LTQ-Orbitrap
                  <li><a href="/parameters/parameters_201401/comet.params.high-high">comet.params.high-high</a> - high res MS1 and high res MS2 e.g. Q Exactive or Q-Tof
               </ul>
            </div>

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
