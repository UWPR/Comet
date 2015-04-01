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
                     free to e-mail me directly (jke000 at gmail dot com or engj at uw dot edu).
                  </ul>
            </div>
            <div class="post hr">
               <p>If you notice performance issues specifying lots of input files on the command line
                  with Windows, i.e. a creep in memory use with corresponding CPU usage degredation,
                  avoid the problem. You can do this by creating a wrapper script batch program which
                  is just a text file with a ".bat" file extension.  Name it something like
                  "runcomet.bat" with the contents as below and execute it instead of executing Comet
                  directly i.e. "<tt>runcomet.bat *.mzXML</tt>".  By running such a batch script, it
                  will invoke individual, sequential instances of Comet to search all input files. 

               <p>Contents of runcomet.bat (you can save-link to file): &nbsp; <a href="runcomet.bat"><tt>for %%A in (%*) do ( comet.exe %%A )</tt></a>

               <p>Specify the full path to the comet.exe binary in the batch file if it does not
                  reside in the same directory as the command is being executed.

               <p>For linux, try: &nbsp; <tt>find . -name '*.mzXML' -print | xargs comet</tt>.
               <br>Or try this simple <a href="runcomet.sh"><tt>runcomet.sh</tt></a> bash script.
                  Download it, make it executable with "chmod +x runcomet.sh" and place it
                  somewhere in your PATH.  Then run searches using "<tt>runcomet.sh *.mzXML</tt>".


            </div>
            <div class="post hr">
               <p>To run Comet, you need one or more input spectral files in mzXML, mzML, ms2/cms2 formats
                  and a comet.params file.  Then issue a command such as:
               <ul>
                  <li><tt>comet.exe input.mzXML</tt>
                  <li><tt>comet.exe input.mzML</tt>
                  <li><tt>comet.exe input.ms2</tt>
               </ul>
            </div>
            <div class="post hr">
               <p>To create a comet.params file, run the following command and rename the create
                  file from "comet.params.new" to "comet.params".
               <ul>
                  <li><tt>comet.exe -p</tt>
               </ul>

               Example version 2015.01 comet.params files (primary differences are the MS and MS/MS mass tolerance settings):
               <ul>
                  <li><a href="/parameters/parameters_201501/comet.params.low-low">comet.params.low-low</a> - low res MS1 and low res MS2 e.g. ion trap
                  <li><a href="/parameters/parameters_201501/comet.params.high-low">comet.params.high-low</a> - high res MS1 and low res MS2 e.g. LTQ-Orbitrap
                  <li><a href="/parameters/parameters_201501/comet.params.high-high">comet.params.high-high</a> - high res MS1 and high res MS2 e.g. Q Exactive or Q-Tof
               </ul>
            </div>

            <div class="post hr">
               <p>For low-res ms/ms spectra, try the following settings:
               <ul>
                  <li><a href="/parameters/parameters_201501/fragment_bin_tol.php">fragment_bin_tol</a> = 1.0005
                  <li><a href="/parameters/parameters_201501/fragment_bin_offset.php">fragment_bin_offset</a> = 0.4
                  <li><a href="/parameters/parameters_201501/theoretical_fragment_ions.php">theoretical_fragment_ions</a> = 1
                  <li><a href="/parameters/parameters_201501/use_sparse_matrix.php">use_sparse_matrix</a> = 1
                  <li><a href="/parameters/parameters_201501/spectrum_batch_size.php">spectrum_batch_size</a> = 0
               </ul>

               <p>For high-res ms/ms spectra, try the following settings:
               <ul>
                  <li><a href="/parameters/parameters_201501/fragment_bin_tol.php">fragment_bin_tol</a> = 0.02
                  <li><a href="/parameters/parameters_201501/fragment_bin_offset.php">fragment_bin_offset</a> = 0.0
                  <li><a href="/parameters/parameters_201501/theoretical_fragment_ions.php">theoretical_fragment_ions</a> = 0
                  <li><a href="/parameters/parameters_201501/use_sparse_matrix.php">use_sparse_matrix</a> = 1
                  <li><a href="/parameters/parameters_201501/spectrum_batch_size.php">spectrum_batch_size</a> = 10000 (depending on free memory)
               </ul>
            </div>

                  <!--
            <div class="post hr">
               <p>Here's an example of how Comet scales with increasing core count using an 16-core 
                  linux machine with Comet 2015.01 rev 0 (analyzed on 2015/02/24):
<pre>
# threads  runtime (mm:ss)   scaling compared to baseline
-------------------------------------------------
    1          23:55          100% (baseline)
    2          14:23           83%
    4          14:16           42%
    8          19:13           80%
 </pre>
            </div>
            -->

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
