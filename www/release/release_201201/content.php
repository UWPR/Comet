<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2012.01</h1>
                              
            <ul>
               <b>release 2012.01 rev 3 (2012.01.3), release date 2013/02/04</b>
               <li>This is a maintenance release.  
               <li>Addresses issue with .out file generation when full/relative
               path is specified for input file.
               <li>Removes full/relative paths in pep.xml output (i.e.
               in "summary_xml" and "spectrum" elements).
               <li>Fixes issue where spurrious modifications are reported if
               no variable modification is specified in the search.
               <li>Known issue with pep.xml modification reporting, see
               <a href="https://groups.google.com/forum/#!topic/comet-ms/KrbM57S050M">this post
               describing the problem</a>.
               <li>Reported issue with reading certain mzXML files.  The program
               will segfault when reading particular scans (not known why).
               <a href="https://groups.google.com/forum/#!topic/comet-ms/aHb_cP_5bXw">More details here</a>.
               <li>Known bug:  -D command line parameter to set database does not
               work; you must set the database in the params file.
            </ul>
            <ul>
               <b>release 2012.01 rev 2 (2012.01.2), release date 2013/01/10</b>
               <li>This is a maintenance release.  
               <li>Change is in pep.xml output only.
               <li>This release implements the "deltacnstar" score in pep.xml
               output which is important for things like phospho-searches
               containing homologous top-scoring peptides when analyzed by
               PeptideProphet (using the "leave alone entries with asterisked
               score values" option).
            </ul>
            <ul>
               <b>release 2012.01 rev 1 (2012.01.1), release date 2012/12/14</b>
               <li>This is a maintenance release.  
               <li>Covers most changes from the comet-ms SourceForge project
               revisions <a href="https://sourceforge.net/p/comet-ms/code/24/">r24</a> through
               <a href="https://sourceforge.net/p/comet-ms/code/34/">34</a>.
               <li>Fixes/changes include correcting modified internal decoy
               peptides, corrects modification reporting in pep.xml output,
               and adds more complete headers to SQT output.
               <li>Source and binary release files are named comet_source.2012011.zip
               and comet_binaries.2012011.zip, respectively.
               <li>Known bug: in pep.xml output, the "deltacnstar" and "deltacn" parameters
               currently still does not implement the code logic of noting "similar" peptides.
               This is important for the "leave alone asterisked score values" option
               in PeptideProphet in conjunction with variable modification searches.
            </ul>
            <ul>
               <b>release 2012.01 rev 0 (2012.01.0), release date 2012/08/16</b>
               <li>This is the initial release of Comet.
               <li>Known bug: modifications with internal decoy search will cause a
               segfault if the enzyme cuts n-terminal to specified residues. This
               bug has been fixed in the sources files in trunk as of 2012/10/09.
            </ul>

            <p>Documentation for parameters for release 2012.01
            <a href="http://comet-ms.sourceforge.net/parameters/parameters_201201/">can be found here</a>.

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> link.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
