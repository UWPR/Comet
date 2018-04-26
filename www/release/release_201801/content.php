<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2018.01</h1>
            <ul>
               <b>release 2018.01 rev. 0 (2018.01.0), release date 2018/04/26</b>
               <li>In the interest of run time performance, only a single PEFF modification at a time
                   is applied to any given peptide.  PEFF modifications are also not applied to a
                   peptide that contains a PEFF variant (amino acid substitution).
               <li>In the "modified_peptide" peptide string in the "modification_info" element of a pep.xml
                   file, static n- and c-terminal modifications were previously reported in the peptide
                   (e.g. "n[230]DIGSTK"). As only variable amino acid modifications are reported in the
                   "modified_peptide" string, Comet will now just report termini modifications 
                   in this peptide string if they contain a variable modification.
               <li>If any input file reports no spectra searched (such as an mzML files without a scan index),
                   the incomplete output files are removed and a non-zero exit code is returned.
               <li>Comet will now check if there is an updated version available and report if so. This
                   also triggers a Comet Google analytics hit . Setting
                   "<a href="/parameters/parameters_201801/skip_updatecheck.php">skip_updatecheck</a> = 1"
                   will skip this.
               <li>"<a href="/parameters/parameters_201801/isotope_offset.php">isotope_offset</a> = 5"
                   will search the -1/0/1/2/3 C13 offsets which is what used to be available prior to
                   versions 2017.01. The -1 C13 isotope offset really makes no sense but we've seen
                   cases where a wrong isotope peak, one C13 less than the monoisotopic peak, is listed
                   as the precursor peak. This occurs when a noise peak or a different
               <li>Bug fix:  PEFF parsing of modifications would previously terminate at the first space
                   which could occur within the text of a modification description causing an incomplete
                   set of PEFF modifications to be analyzed; this is now fixed.
               <li>Bug fix:  Starting with 2017.01 rev. 0, not all permutations of variable modifications
                   get analyzed when multiple variable modification is specified. The permutations of
                   modifications would be terminated at the first permutation occurrence of two variable
                   modifications on the same reisdue. This is fixed.
                   peptide peak shows up at that minus-1 offset in the precursor spectrum.
               <li>This version of Comet will run with comet.params files from version 2017.01.
            </ul>

            <p>Documentation for parameters for release 2017.01
            <a href="/parameters/parameters_201701/">can be found here</a>.

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> page.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
