<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2015.02</h1>
                              
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

            <p>Documentation for parameters for release 2015.02
            <a href="/parameters/parameters_201502/">can be found here</a>.

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> page.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
