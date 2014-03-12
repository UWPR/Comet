<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2014.01</h1>
                              
            <ul>
               <b>release 2014.01 rev. 0 (2014.01.0), release date 2014/03/??</b>
               <li>For Windows binary only, revert previous memory changes (VirtualAlloc/VirtualLock)
                   as there were negative consequences associated with unexpected
                   out of memory errors.
               <li>Add "<a href="/parameters/parameters_201401/output_suffix.php">output_suffix</a>"
                   parameter option which appends a suffix to the
                   base name of pep.xml, pin.xml, txt, and sqt output files.
               <li>Add Native ID attribute to pep.xml output when searching mzML files.
               <li>Change in spectral processing to handle edge case where the
                   last peak in some spectra are excluded from the analysis.
               <li>Fix bug that required order of two loops to be swapped.  Before this
                   fix, a small number of fragment ions possibly do not get considered
                   for particular peptides/spectrum combinations.
                   Thanks to A. Kertesz-Farkas for identifying these last 2 issues.
               <li>Report the name of the file that's being searched in the runtime output.
               <li>Avoid creating empty/stub output files in the case where an input
                   file has been moved away during a search.
               <li>Modify text output, primarily change column order, for Crux-compiled binary.
            </ul>

            <p>Documentation for parameters for release 2014.01
            <a href="/parameters/parameters_201401/">can be found here</a>.

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> link.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
