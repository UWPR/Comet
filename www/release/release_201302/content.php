<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2013.02</h1>
                              
            <p>Documentation for parameters for release 2013.02
            <a href="/parameters/parameters_201302/">can be found here</a>.

            <ul>
               <b>release 2013.02 rev 2 (2013.02.2), release date 2014/01/23</b>
               <li>The major change with this maintenance release addresses Windows
                   search performance.  When searching using lots of memory (such as
                   with small "<a href="/parameters/parameters_201302/fragment_bin_tol.php">fragment_bin_tol</a>" values),
                   the search performance was greatly reduced.  We believe allocated memory was
                   being swapped to disk even when there is more than enough memory/RAM
                   on the host machine.  This issue is addressed by allocating memory
                   differently (using the VirtualAlloc function call) and locking memory
                   from being paged out to disk (using the VirtualLock function call).
               <li>Extend maximum protein accession string length from 40 to 512.
               <li>Change default "<a href="/parameters/parameters_201302/remove_precursor_tolerance.php">remove_precursor_tolerance</a>" value from 2.0 to 1.5.
               <li>Skip writing out blank search hit lines for Crux-compiled text output.
               <li>Corrects E-value calculation for decoy entries when using separate
                   target/decoy searches (i.e. "<a href="/parameters/parameters_201302/decoy_search.php">decoy_search</a> = 2").
            </ul>
            <ul>
               <b>release 2013.02 rev 1 (2013.02.1), release date 2013/11/25</b>
               <li>Fix bug for both pep.xml and pin.xml output where terminal variable
                   modifications were not being written out to these two output formats.
               <li>Fix bug where .sqt and .txt headers were not being written out to
                   subsequent output files after the first output file when multiple input
                   files are specified on the command line (i.e. "comet.exe *.mzXML").
               <li>Allow whitespace in "database = " parameter value i.e. the database
                   name and/or path can contain a whitespace.  Note that having whitespaces
                   in the database name will break some downstream tools (like TPP).
               <li>Change Percolator pin.xml features.  "dM" and "absdM" are now calculated
                   as (experimental_mass - calculated_mass)/calculated_mass.  Previously this feature
                   was reporting (calculated_mz - experimental_mz).  Sadly this also means
                   that the "calculatedMassToCharge" and "experimentalMassToCharge" attributes
                   are actually report MH+ masses now.  Although this is completely wrong, this
                   is to be consistent with a different tool (sqt2pin) that is also generating
                   these files.
               <li>Change behavior in pep.xml output where "summary_xml" and "base_name"
                   elements now include resolved full paths.  Also get rid of unused
                   xml-stylesheet directive.
               <li>Increase number of significant digits to 6 for reported masses in all of
                   the output formats.
               <li>Known issue:  The nucleotide database search options, invoked using the
                   "<a href="/parameters/parameters_201302/nucleotide_reading_frame.php">nucleotide_reading_frame</a>"
                   parameter entry, is not functional.
               <li>Known issue:  Windows performance when using large memory (i.e. when the
                   "<a href="/parameters/parameters_201302/fragment_bin_tol.php">fragment_bin_tol</a>"
                   parameter is set to a small value) is poor.  We believe this is due to disk
                   paging even for systems with sufficient memory.  A fix, using MSFT's
                   VirtualAlloc and VirtualLock functions, is in the works.
            </ul>

            <ul>
               <b>release 2013.02 rev 0 (2013.02.0), release date 2013/10/18</b>
               <li>There are quite a few behind-the-scenes changes where the core
                   search code has been moved into a library in preparation
                   for interfacing with other tools (thanks to T. Jahan).
               <li>Underlying changes to the code were made to facilitate integration
                   with the <a target="new" href="http://noble.gs.washington.edu/proj/crux/">Crux project</a>
                   (thanks to S. McIlwain).
               <li>This release incorporates an update to the <a target="new" href="http://code.google.com/p/mstoolkit/">MSToolkit</a>
                   file parsing library (thanks to M. Hoopmann).
               <li>Support for the mz5 file format has been dropped.  mzXML, mzML, and
                   ms2 formats are the supported input formats.
               <li>Comet will now report a warning for any unknown parameter due to manually
                   introduced typos.  In the past, these were ignored.  The recommended method to
                   generate a valid/current parameters file is still to use the command
                   "<tt>comet.exe -p</tt>" to avoid silly errors like this.
               <li>A new parameter "<a href="/parameters/parameters_201302/decoy_prefix.php">decoy_prefix</a>"
                   allows one to define the string pre-pended to the protein name for decoy matches
                   when the <a href="/parameters/parameters_201302/decoy_search.php">decoy_search</a>
                   parameter is used.
               <li>A new parameter "<a href="/parameters/parameters_201302/output_pinxmlfile.php">output_pinxmlfile</a>"
                   controls support for outputting <a href="http://per-colator.com/interface/">Percolator-IN</a>
                   pin.xml files.
               <li>The <a href="/parameters/parameters_201302/output_txtfile.php">text output</a>
                   format has changed to add new columns and column headers.  Please note that
                   neutral masses are now reported (vs. singly protonated masses as in the past).
               <li>During testing of this release, it was discovered that a subset of N-terminal
                   fragment ions for internally generated decoy peptides were incorrectly being
                   calculated; this has been corrected in this release.
               <li>The tab-delimited text output was reported the second best hit and not the
                   top ranked hit.  This has also been corrected in this release.
            </ul>

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> link.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
