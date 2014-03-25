<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2014.01</h1>
                              
            <ul>
               <b>release 2014.01 rev. 0 (2014.01.0), release date 2014/03/25</b>
               <li>Add "<a href="/parameters/parameters_201401/output_suffix.php">output_suffix</a>"
                   parameter option which appends a suffix to the
                   base name of pep.xml, pin.xml, txt, and sqt output files.
               <li>Add Native ID attribute to pep.xml output when searching mzML files.
                   This entails an update to the <a href="http://code.google.com/p/mstoolkit/">MSToolkit</a>
                   file parsing library which was made by M. Hoopman.
               <li>Report the name of the file that's being searched in the runtime output.
               <li>Avoid creating empty/stub output files in the case where an input
                   file has been moved away during a search.  These first four features
                   were based on requests by D. Tabb.
               <li>Change how E-values are calculated when
                   "<a href="/parameters/parameters_201401/decoy_search.php">decoy_search</a> = 2"
                   is applied. This setting performs a separate target and decoy search as if
                   two different databases were searched separately resulting in two sets of
                   search results.  In the past, the E-value calculations for the target hits
                   were calculated on the underlying target xcorr score distribution only.  And
                   the decoy E-value calculations were based only on the decoy xcorr score
                   distribution.  Now, the target and decoy score hits contribute to one xcorr
                   score distribution that is used to calculate the target and decoy E-values
                   per suggestion by the Noble lab.
               <li>Sort results by scan number; previously results were reported in order of
                   increasing peptide mass.  With results sorted by scan number, this allows
                   more efficient analysis using quantification tools that cache spectral
                   data in memory.  Requested by D. Shteynberg.
               <li>Bug fix:  Change in spectral processing to handle edge case where the
                   last peak in some spectra are excluded from the analysis.
               <li>Bug fix:  Correct a bug that required order of two loops to be swapped.
                   Before this fix, a small number of fragment ions possibly do not get
                   considered for particular peptide/spectrum/charge combinations.
                   Thanks to A. Kertesz-Farkas for identifying these last two issues.
               <li>For Windows binary only: revert previous memory changes (using VirtualAlloc
                   and VirtualLock functions) as there were negative consequences associated
                   with unexpected out of memory errors.
               <li>For Crux compiled version only:  modify text output, primarily changing column
                   order using changes submitted by S. McIlWain.
               <li>For Crux compiled version only:  report all peptide hits, including those with
                   negative xcorr scores.
               <li>As the "<a href="/parameters/parameters_201401/output_suffix.php">output_suffix</a>"
                   is the only new parameter entry which most researchers likely won't use,
                   this version of Comet will also run with comet.params files from
                   version 2013.02.
            </ul>

            <p>Documentation for parameters for release 2014.01
            <a href="/parameters/parameters_201401/">can be found here</a>.

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> link.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
