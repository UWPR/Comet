<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2013.01</h1>
                              
            <ul>
               <b>release 2013.01 rev 0 (2013.01.0), release date 2013/06/05</b>
               <li>This is the second major release of Comet.
               <li>Comet now supports searching multiple input files i.e. "<tt>comet.exe *.mzXML</tt>".
                   Comet will search each input file sequentially, one after the other.
               <li>New command line option '-F<i>&lt;num&gt;</i>' and '-L<i>&lt;num&gt;</i>'
                   to specify the first and last scan to search.  This scan range can also be
                   specified using the <a href="/parameters/parameters_201301/scan_range.php">scan_range</a> parameter.
                   Additionally, the scan range can still be specified appended to
                   each input file i.e. <tt>comet.exe file.mzXML:1500-5000</tt>.
                   Scan ranges specified appended to each mzXML file will override
                   the -F/-L command line options which in turn will override the
                   <a href="/parameters/parameters_201301/scan_range.php">scan_range</a>
                   parameter.
               <li>Fix bug with '-D' command line parameter not applying the specified
                   sequence database to override the database specified in the params file.
               <li>Fix bug associated with application of <a href="/parameters/parameters_201301/use_NL_ions.php">use_NL_ions</a>.
                   An error in the code logic was not applying neutral loss peaks
                   to all relevant ion series.
               <li>Updated MSToolkit code for more robust reading of input files.
               <li>New parameter:  <a href="/parameters/parameters_201301/use_sparse_matrix.php">use_sparse_matrix</a>
                   <br>Implemented by Mike Hoopman, the main new feature of
                   Comet release 2013.01 is the ability to use a sparse matrix
                   data representation internally.
                   When using a small fragment_bin_tol value (i.e. 0.01), the original
                   implementation of Comet will use a *huge* amount of memory (see 
                   the <a href="http://proteomicsresource.washington.edu/sequest_release/release_201201.php#mem_use">memory use table here</a>).
                   The sparse matrix implementation reduces memory use tremendously
                   but searches are slower than the classical implementation.
               <li>New parameter:  <a href="/parameters/parameters_201301/spectrum_batch_size.php">spectrum_batch_size</a>
                   <br>This parameter defines how many spectra are searched at
                   a time.  For example, if the parameter value were set to 1000 then
                   Comet would loop through searching about 1000 spectra at a time until
                   all data are analyzed. This can be used for searching a large dataset
                   that might not fit in memory when searched all at once.
               <li>New parameter:  <a href="/parameters/parameters_201301/clear_mz_range.php">clear_mz_range</a>
                   <br>This parameter removes (i.e. clears out) all peaks within the
                   specified m/z range prior to any data processing and searching.
                   This functionality was intended for iTRAQ/TMT type analysis with
                   the goal to ignore the reporter ion signals from the search.
               <li>Revised parameter:  <a href="/parameters/parameters_201301/fragment_bin_offset.php">fragment_bin_offset</a>
                   <br>In the 2012.01 release, the valid value of this parameter
                   ranged from 0.0 to the fragment_bin_tol value.  Per request,
                   the meaning of this parameter has been changed such that valid
                   values now range from 0.0 to 1.0 where the actual offset value
                   scales by fragment_bin_tol. This is not a parameter that most
                   users should care about; use the recommended values written in
                   the params file notes.
               <li>Add version number to head of comet.params file.
                   <br>Comet will now validate that a compatible params file is
                   being used by checking the version number written in the comet.params
                   file.  This was added to avoid running Comet using an old/outdated
                   parameters file where an existing parameter name might have a change
                   in behavior (such as fragment_bin_offset above).  Comet will not
                   run if the version string is not compatible.
               <li>Change format of output filenames when input scan range is specified.
                   <br> When searching a scan range, say 1-5000, release 2012.01
                   would create output files with the name extension basename.pep.xml:1-5000
                   or basename.sqt:1-5000.  This version 2013.01 will now name these
                   files as basename.1-5000.pep.xml and basename.1-5000.sqt.  This
                   keeps the file extensions (.pep.xml and .sqt) at the end of the
                   filenames for better compatibility with downstream tools.
                   Ideally one uses the <a href="/parameters/parameters_201301/spectrum_batch_size.php">spectrum_batch_size</a>
                   parameter and not run individual subset scan range searches to
                   avoid this naming issue.
            </ul>


            <p>Documentation for parameters for release 2013.01
            <a href="http://comet-ms.sourceforge.net/parameters/parameters_201301/">can be found here</a>.

            <p>For low-res ms/ms spectra, try the following settings:
               <ul>
               <li>fragment_bin_tol = 1.0005
               <li>fragment_bin_offset = 0.4
               <li>theoretical_fragment_ions = 1
               </ul>

            <p>For high-res ms/ms spectra, try the following settings:
               <ul>
               <li>fragment_bin_tol = 0.02
               <li>fragment_bin_offset = 0.0
               <li>theoretical_fragment_ions = 0
               </ul>

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> link.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
