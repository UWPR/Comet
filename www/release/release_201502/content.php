<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2015.02</h1>
                              
            <p>Documentation for parameters for release 2015.02
            <a href="/parameters/parameters_201502/">can be found here</a>.

            <ul>
               <b>release 2015.02 rev. 5 (2015.02.5), release date 2016/01/22</b>
               <li>pepXML output: correctly report modified_peptide string; previously missing
                   static modifications and at times terminal modifications.
               <li>MGF file parsing: fix how fragment masses are adjusted when their fragment
                   ion charge states are present.
               <li>RAW file parsing: update MSToolkit to not report warning of unknown tokens
                   't' and 'E'.
            </ul>
            <ul>
               <b>release 2015.02 rev. 4 (2015.02.4), release date 2016/01/07</b>
               <li>Additional parsing changes for better MGF support.
               <li>pepXML output: correct "mod_cterm_mass" value and escape special characters
                   in "spectrumNativeID" values.
            </ul>
            <ul>
               <b>release 2015.02 rev. 3 (2015.02.3), release date 2015/11/24</b>
               <li>Fix incorrect MGF parsing where blank lines in the MGF file would cause
                   an error.
               <li>Fix n-term distance constraint variable mod searches.  Thanks to U.
                   Eckhard for reporting the above two issues.
               <li>Change output file extension to ".pin" from ".tsv" for Percolator output
                   files.
               <li>Fix negative deltaCn values in text file output when no second hit is
                   present.
               <li>Fix case where double decoy string is appended to the protein accession
                   for decoy matches.
            </ul>
            <ul>
               <b>release 2015.02 rev. 2 (2015.02.2), release date 2015/10/15</b>
               <li>Minor update to fix bug in binary modifications.  The bug manifested
                   itself in the simplest case where say two lysine residues are labeled
                   with different modification masses and set to different binary modification
                   groups.  In this case, the second modification is not analyzed. Bug
                   reported by Villen lab.
               <li>Known bug: when decoy searches are applied, the decoy prefix could be
                   added multiple times for certain cases where the same decoy peptide is
                   present in multiple decoy proteins.  A fix has been implemented and will
                   be included in the next release; let me know if you need a patched binary
                   sooner than that.
            </ul>
            <ul>
               <b>release 2015.02 rev. 1 (2015.02.1), release date 2015/09/30</b>
               <li>Modify behavior the binary modifications which is controlled by the
                   third parameter entry in the variable modifications 
                   (e.g. "<a href="http://comet-ms.sourceforge.net/parameters/parameters_201502/variable_mod01.php">variable_mod01</a>").
                   Instead of a binary 0 or 1 value to turn off or on each binary modification,
                   one can now set the third parameter entry to the same value across multiple
                   variable modifications effectively allowing an all-modified binary
                   behavior across multiple variable modifications.  See the examples at
                   the bottom of the variable modification help pages for further explanation.
                   NOTE: a bug was just discovered with binary modifications; a fix is
                   imminent.
               <li>Wide mass tolerance searches, such as those performed by the
                   <a href="http://www.ncbi.nlm.nih.gov/pubmed/26076430">Gygi lab's
                   mass-tolerant searches</a>, are now supported by Comet.  Previous versions
                   of Comet would crash when given large tolerances.
               <li>Update MSToolkit to support "possible charge state" cvParam in mzML files as
                   implemented by M. Hoopmann.
            </ul>

            <ul>
               <b>release 2015.02 rev. 0 (2015.02.0), release date 2015/07/31</b>
               <li>Associated with this release, a Windows GUI program to run
                   Comet searches and visualize results is available. The
                   Comet GUI supports 64-bit Windows only and can be found
                   <a href="/CometUI/">here</a>.
               <li>Updated to <a href=https://github.com/mhoopmann/mstoolkit">MSToolkit</a>
                   revision 81 which includes .mgf input file support.  Thanks to
                   M. Hoopmann for updating MSToolkit for this.
               <li>Add a fourth option to 
                   ("<a href="http://comet-ms.sourceforge.net/parameters/parameters_201502/override_charge.php">override_charge</a>")
                   which will either use the specified charge in the input file or
                   apply the charge states in the charge_range parameter but
                   include the 1+ charge rule.  Requested by D. Shteynberg.
               <li>Add "<a href="http://comet-ms.sourceforge.net/parameters/parameters_201502/mass_offsets.php">mass_offsets</a>"
                   parameter.  Using this parameter, one can search spectra for
                   peptides that have a mass offset from the experimental mass.
                   Requested by ISB.
               <li>The "<a href="http://comet-ms.sourceforge.net/parameters/parameters_201502/precursor_tolerance_type.php">precursor_tolerance_type</a>"
                   parameter makes its return.  It was not needed for precursor tolerances
                   specified as ppm, which is the reason it was removed. But it is still
                   relevant when amu and mmu are the units specified for the precursor
                   tolerance and is now only applied in these cases.
               <li>The "<a href="http://comet-ms.sourceforge.net/parameters/parameters_201501/use_sparse_matrix.php">use_sparse_matrix</a>"
                   parameter has been retired.  All searches now use this internal
                   data representation by default.
               <li>Corrected specification of terminal modifications in pep.xml output
                   in cases when both static peptide and protein terminal modifications
                   are specified.  Reported by D. Hernandez.
               <li>Fix small bug that inadvertantly removed .cms2 input file support
                   in previous release.  Reported by MacCoss lab.
            </ul>

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> page.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
