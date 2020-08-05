<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2019.01</h1>
            <ul>
               <b>release 2019.01 rev. 5 (2019.01.5), release date 2020/04/06</b>
               <li>Bug fix: a bug in the code could randomly set the "minimum_peaks"
               parameter value to a random large number, causing no peaks to be read
               in, resulting in blank search results reported. Thanks to C. Bielow
               for not only reporting the bug but also debugging the code and
               implementing the fix.
               <li>Known bug: Nucleotide database searches will fail. This bug was
               introduced in version 2019.01.2. A fix has been implemented; the 2019.01.5
               release binaries were updated with this fix on 2020/05/28.
               Thanks to N. Tay for reporting the issue.
               <li>Known bug: Searches of mzML and mzXML files without scan index do
               not work under Windows. A fix has been implemented; the 2019.01.5
               release binaries were updated with this fix on 2020/05/28.
               Thanks to R. Marrisen for reporting the issue.
            </ul>
            <ul>
               <b>release 2019.01 rev. 4 (2019.01.4), release date 2020/01/15</b>
               <li>Add support for searching mzML and mzXML files that do not contain the
               options scan index.  Such files would previously have not been searched as
               MSToolkit would throw an error message about the missing index.
               <li>Bug fix: PEFF substitutions on flanking residues, generating a new
               peptide that would otherwise not be analyzed, are treated more rigorously.
            </ul>
            <ul>
               <b>release 2019.01 rev. 3 (2019.01.3), release date 2019/12/06</b>
               <li>Bug fix: Searches against mzML files would report the wrong scan number.
               The issue was due to the MSToolkit update for release 2019.01.0.  Using the
               latest MSToolkit commit 2021e7e from 12/3/19 fixes this error.  Thanks to
               Z. Sun for reporting the bug.
            </ul>
            <ul>
               <b>release 2019.01 rev. 2 (2019.01.2), release date 2019/11/18</b>
               <li>Bug fix: introduced in the 2019.01 rev. 0 release, Comet would not properly handle
               a "<a href="/parameters/parameters_201901/clip_nterm_methionine.php">clip_nterm_methionine_</i></a>"
               search. This bug would manifest as either a segmentation fault or as a NULL character
               reported for a flanking residue due to not properly tracking the shortened
               protein length when the start methione is clipped off.  Thanks to the
               Villen Lab and R. Johnson for reporting the issue.
            </ul>
            <ul>
               <b>release 2019.01 rev. 1 (2019.01.1), release date 2019/09/06</b>
               <li>Known bug: a NULL character can show up as the flanking residue 
               (peptide_next_aa attribute in pep.xml output) for an internal decoy match.
               <li>In Percolator .pin output, change ExpMass and CalcMass from neutral
               masses to singly protonated masses.  Thanks to W. Fondrie and the
               Crux team for reporting that Percolator expects these masses to be
               singly protonated.
               <li> Bug fix:  correct missing residue in StaticMod header entry of SQT
               output.  Thanks to A. Zelter for reporting the issue.
               <li> Bug fix:  update database indexing and index search to correctly
               handle terminal variable modifications.  Thanks to T. Zhao for
               reporting the issue.
            </ul>
            <ul>
               <b>release 2019.01 rev. 0 (2019.01.0), release date 2019/08/19</b>
               <li>Known bug: database indexing fails with variable modifications; particularly with
               variable terminal mods.  See the release_2019011 branch which contains a fix.
               <li>Add support for user specified fragment neutral loss ions
               (such as phosphate neutral loss) via the addition of an 8th field to
               the "<a href="/parameters/parameters_201901/variable_mod01.php">variable_mod01 to variable_mod09</i></a>" parameters.
               <li>Add support for user specified precursor neutral loss ions
               using the "<a href="/parameters/parameters_201901/precursor_NL_ions.php">precursor_NL_ions</a>" parameter.
               <li>Add "<a href="/parameters/parameters_201901/max_duplicate_proteins.php">max_duplicate_proteins</a>"
               parameter which can limit the number of protein
               identifiers reported/returned for a given peptide.
               <li>Add "<a href="/parameters/parameters_201901/peptide_length_range.php">peptide_length_range</a>" parameter.
               This parameter allows the specification of a minimum and maximum peptide length.
               <li>Add "<a href="/parameters/parameters_201901/search_enzyme2_number.php">search_enzyme2_number</a>"
               parameter. Allows optional selection of a second digestion enzyme.  
               <a href="/parameters/parameters_201901/num_enzyme_termini.php">Enzyme specificity</a>
               and <a href="/parameters/parameters_201901/allowed_missed_cleavage.php">missed cleavage</a>
               settings are are shared between both
               "<a href="/parameters/parameters_201901/search_enzyme_number.php">search_enzyme_number</a>"
               and "<a href="/parameters/parameters_201901/search_enzyme2_number.php">search_enzyme2_number</a>".
               <li>Update "<a href="/parameters/parameters_201901/max_variable_mods_in_peptide.php">max_variable_mods_in_peptide</a>"
               parameter to support value 0.
               <li>In the example comet.params files available <a href="/parameters/parameters_201901/">here</a>
               and the when generated using "comet -p", the
               "<a href="/parameters/parameters_201901/spectrum_batch_size.php">spectrum_batch_size</a>"
               parameter is now set to 15000 instead of 0.
               For high-res "<a href="/parameters/parameters_201901/fragment_bin_tol.php">fragment_bin_tol</a>"
               settings, Comet should use less than 6GB of memory with a 15K batch size with very little
               impact on search times compared to loading and searching all spectra at once.
               If you have a computer with lots of ram, feel free to set this parameter value to 0 to gain
               a bit of search speed.  Note that when this parameter is missing, its default value is 0.
               <li>Added indexed database support. This was implemented to support real-time
               searching. A database index can be created with the command "comet.exe -i" which
               will create an indexed database of the database specified in the comet.params file.
               Static and variable modifications specified in the comet.params file will be
               stored in the index database. Note an indexed database (with .idx extension) can
               be searched if you specify the indexed database file as the search database
               ("<a href="/parameters/parameters_201901/database_name.php">database_name</a>").
               Searching an indexed database only uses a single thread so the
               "<a href="/parameters/parameters_201901/num_threads.php">num_threads</a>"
               parameter is ignored if an indexed database is specified.
               <li>Support real-time searching of single spectra against an indexed database
               with a C# interface for CometWrapper.dll.
               This includes an example C# program (RealtimeSearch/Search.cs) that loops through all
               scans of a Thermo raw file using Thermo's RawFileReader and sequentially calling
               DoSingleSpectrumSearch() to run the search.
               Thanks to D. Schweppe and the Gygi lab at Harvard and D. Bailey at Thermo for
               contributions to this project.  Some 
               <a href="/notes/20190820_indexdb.php">notes on real-time search support are available  here</a>.
               <li>Update MSToolkit from b41b33f commit on 1/30/19
               <li>Bug fix: error with logic in analysis in target-decoy searches where
               erroneous target and decoy fragment ions could be generated.
               <li>Bug fix: add support for parsing '*' in the sequence which was removed in the
               2018.01 rev. 0 release. This character is treated as a stop codon; residues
               flanking this character are valid search enzyme termini.
               <li>Features that didn't make the cut and are targeted for the next release:
               mzIdentML output and Comet-PTM functionality.
            </ul>

            <p>Documentation for parameters for release 2019.01
            <a href="/parameters/parameters_201901/">can be found here</a>.

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> page.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
