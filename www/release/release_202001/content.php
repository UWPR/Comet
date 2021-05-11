<meta charse ="UTF-8">

<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2020.01</h1>

            <p>Documentation for parameters for release 2020.01
            <a href="/parameters/parameters_202001/">can be found here</a>.

            <ul>
               <b>release 2020.01 rev. 4 (2020.01.4), release date 2021/05/11</b>
               <li>Bug fix: for Percolator .pin output, correctly calculate the dM (mass difference)
               for all lower hit entries. The top hit's dM value was being reported for all lower
               hits for each spectrum query. Issue and fix reported by D. Goldfarb.
               <li>Bug fix: address potential ThreadPool sleep issue. Issue identified and fixed
               by D. Shteynberg.
            </ul>
            <ul>
               <b>release 2020.01 rev. 3 (2020.01.3), release date 2021/03/18</b>
               <li>Revert use of maps (ala HiXcorr) to spectral arrays for spectral preprocessing.
               The use of maps had a measurable performance hit under Windows that was
               especially noticeable for real-time search applications.

               <li> For each
               "<a href="/parameters/parameters_202001/variable_mod01.php">variable_mod0X</i></a>"
               parameter, extend the 4th field to allow specifying
               a minimum (in addition to the maximum) number of modified residues in each
               peptide. Feature request by J. Mohr.

               <li>In the text output, the xcorr rank column (2nd column) will now correctly
               handle ties in its rank reporting.

               <li>Extend the maximum length of reported protein accession string from 99 to 511
               characters. Feature request by L. Liu.

               <li>Real-time search: Comet was inadvertently looping over all PSMs for a
               spectrum query to retrieve matched protein names. This was unnecessary and
               caused a performance hit; protein names are now retrieved only for the top
               scoring peptide(s).

               <li>Bug fix: added a check to see if a directory path was specified as the sequence
               database. Under linux, if a directory were specified as the database, Comet
               would simply sit there and not report any error. Thanks to L. Mendoza for
               reporting this issue.

               <li>Bug fix: report proper flanking residues for a peptide when it is identified in
               multiple proteins; use the flanking residues from the first protein in the
               database. Similarly, flanking residues for start methionine clipped sequences
               were addressed as those were not being handled correctly either.

               <li>Bug fix: for real-time search using Comet's internal decoy peptides, storing
               and reporting of duplicate peptides, e.g. a peptide that is present in both
               target and decoy forms, was not being handled correctly; this has been
               addressed.

               <li>Bug fix: in the pep.xml output, the "index" attribute of the "spectrum_query"
               element was not being populated correctly. With each spectrum batch, the index
               value was being reset to 1. This has been corrected.

               <li>Known bug: <a href="https://groups.google.com/g/comet-ms/c/JL9zrbNWcQM">per this thread</a>,
               the mass difference (dM) column of the Percolator .pin output is always reporting
               the mass difference of the top hit for all lower ranked hits of a spectrum query.
            </ul>
            <ul>
               <b>release 2020.01 rev. 2 (2020.01.2), release date 2021/01/05</b>
               <li>Bug fix: Fixed issue where spectra were not being searched. This was due to
               the poor attempt at a fix in release 2020.01.1 for spectra with all zero
               intensity peaks.
            </ul>
            <ul>
               <b>release 2020.01 rev. 1 (2020.01.1), release date 2020/12/17</b>

               <li> For TIMS-TOF mzML files, changed the scan number reporting to be the scan
               "index" value plus "1".

               <li>Bug fix: Fixed issue where spectra that have all peak intensities of zero
               would cause the program to crash.  Issue reported by D. Shteynberg.

               <li>Bug fix: Fixed issue where mzML scans without a precursor charge were not being
               searched. This issue was limited to mzML and not a problem with mzXML files.
               Issue reported by D. Shteynberg.
            </ul>
            <ul>
               <b>release 2020.01 rev. 0 (2020.01.0), release date 2020/11/09</b>

               <li>Known bug: scans where all peak intensities are zero will cause the
               program to crash.

               <li>Known bug: scans in mzML files without a precursor charge will not be
               searched. This issue appears limited to mzML files themselves and is not
               a problem with mzXML inputs.

               <li>Implemented mzIdentML output via the parameter entry
               "<a href="/parameters/parameters_202001/output_mzidentmlfile.php">output_mzidentmlfile</i></a>".
               The mzIdentML format does not fully support the reporting of Comet results
               so this is considered preliminary support.  Issues with the mzIdentML format for Comet:
               (a) It appears as if the mzIdentML format expects decoy entries to exist in
               the input FASTA file; at least that seems to be the expectation in the
               documentation that I've read.  Comet's mzIdentML output will report decoy
               protein references even though they do not exist in the underlying FASTA file.
               The reported decoy protein, like in the other output formats, is generated by
               appending the decoy prefix to the protein accession that the decoy peptide was
               generated from.  I have no idea if the mzIdentML format allows for this or not.
               (b) Within the "FragmentTolerance" element, the requirement of fragment
               "search tolerance plus value" (MS:1001412) and
               "search tolerance minus value" (MS:1001413) make no sense in the context of
               spectral correlation matching used in Comet, SEQUEST and other tools
               that perform the cross-correlation score. A fragment bin value
               and bin offset are needed to encapsulate the corresponding fragment settings
               in Comet. Currently 1/2 of the 
               "<a href="/parameters/parameters_202001/fragment_bin_offset.php">fragment_bin_offset</i></a>"
               value is reported for the search tolerance plus/minus values but this is a
               sad hack that should not be required.

               <li>The preliminary score has been modified in a few ways.  First, I extended the
               number of preliminary score ion "bins" considered from 200 to 1000.
               This allows matching many more fragment ions in the preliminary score algorithm
               to get a more representative matched ion count reported in the output files.
               Before this change, the matched fragment numbers are likely under-reported for
               many spectral matches of longer peptides.  I have also gotten rid of the
               peak picking, smoothing, and "stair-stepping" that used to be applied to the
               preliminary score spectrum.  All of these things contribute to a change in the
               calculated preliminary scores.  This score is still reported because some post-processing
               tools make use of it but if it were up to me, I would get rid of it entirely.

               <li>Added the parameter entry
               "<a href="/parameters/parameters_202001/use_Z1_ions.php">use_Z1_ions</i></a>"
               to consider "Z&bull; + 1" ions (typically for ETD/ECD searches).
               Feature requested by A. Grimaud.

               <li>Add support for Comet's internal decoy peptides in the indexed database search
               (intended for real-time search application).
               As decoy peptides don't need to be explicitly present in the indexed database,
               this reduces the index database size and indexing time by nearly half compared
               to indexing a FASTA file composed of target+decoy sequences.

               <li>Add support for reporting multiple matched proteins in an indexed database search.
               Previously only one protein name was returned for each peptide identification. Now
               up to 20 matched/duplicate proteins are stored and reported.

               <li>Corrected/changed residue 'O' from Ornithine to Pyrrolysine. This
               entails retiring the
               "<a href="/parameters/parameters_201901/add_O_ornithine.php">add_O_ornithine</i></a>"
               parameter entry and adding 
               "<a href="/parameters/parameters_202001/add_O_pyrrolysine.php">add_O_pyrrolysine</i></a>".
               Thanks to P. Charles for reporting the correction.

               <li>Added support for changing the text file output file extension from its default
               "txt" extension to a custom file extension via a new parameter entry
               "<a href="/parameters/parameters_202001/text_file_extension.php">text_file_extension</i></a>".
               Text file outputs are generated when the
               "<a href="/parameters/parameters_202001/output_txtfile.php">output_txtfile</i></a>"
               parameter is set to "1".
               This custom/hidden parameter will need to be manually added to your params file for use (i.e.
               it is not present in the example params file available for download nor is it written
               in the comet.params.new file generated by the command "comet -p").
               Feature requested by
               <a href="http://www.patternlabforproteomics.org/">PatternLab for Proteomics</a>.

               <li>Added a parameter entry
               "<a href="/parameters/parameters_202001/explicit_deltacn.php">explicit_deltacn</i></a>"
               which controls how the deltaCn output score is calculated.
               By default, Comet will very crudely analyze sequence similarity when calculating
               the deltaCn score. This results in the deltaCn being calculated between the top
               hit and the first dissimilar peptide in order to avoid very small deltaCn values
               when the top N peptides are all the same (such as different modified forms of the
               same peptide).  When 
               "<a href="/parameters/parameters_202001/explicit_deltacn.php">explicit_deltacn</i></a>"
               is set to "1", the sequence similarity analysis is not used and the deltaCn is
               calculated as the difference between the top scoring peptide and the second best
               scoring peptide.
               This custom parameter will need to be manually added to your params file for use.
               Feature requested by
               <a href="http://www.patternlabforproteomics.org/">PatternLab for Proteomics</a>.

               <li>Added "sp_rank" and "retention_time_sec" columns to the text output.
               Feature requested by
               <a href="http://www.patternlabforproteomics.org/">PatternLab for Proteomics</a>.

               <li>Added preliminary support for searching TIMS-TOF mzML files.  MSToolkit updates
               now allow TIMS-TOF mzML files to be searched. Previous parsing of the mzML file
               would return no spectra to search.  I should note that I'm not sure what the
               returned scan numbers represent.  Also, a function to return the file's last
               scan number was not updated for these TIMS-TOF files; this causes
               Comet's search progress percentage reporting to return nonsensical values.
               Thanks to D. Shteynberg and M. Hoopmann for the MSToolkit mods to support this.

               <li>Extend the
               "<a href="/parameters/parameters_202001/activation_method.php">activation_method</i></a>"
               <li>Removed reporting of "deltacnstar" in the pepXML output.  It appears that
               the score has been reported as "0.0" for every result and I've never understood
               what it represented so I'm taking this opportunity to get rid of it now.
               NOTE: PeptideProphet apparently expects to parse this score so stick to
               <a href="http://comet-ms.sourceforge.net/release/release_201901/">Comet 2019.01.5</a>
               if you need PeptideProphet compatibility.

               <li>Changed the "No_enzyme" text to "Cut_everywhere" in the enzyme definition
               of the exported comet.params file.  Hopefully this clarifies the purpose
               of that enzyme definition of cleaving everywhere (as opposed to no digestion
               which "No_enzyme" could convey). The enzyme text strings in the params file
               have no function in Comet and can be named anything.

               <li>In the spectral processing, large spectral arrays were replaced by peak
               vectors as in the HiXcorr implementation of Comet.  This makes the spectral
               processing a bit more efficient, especially for small
               <a href="http://comet-ms.sourceforge.net/parameters/parameters_202001/fragment_bin_tol.php">fragment_bin_tol</a>
               settings.

               <li>Bug fix:  When a protein N-terminal static modification is specified, it
               was not being applied in the preliminary score routine for N-terminal peptides
               resulting from a
               <a href="http://comet-ms.sourceforge.net/parameters/parameters_201901/clip_nterm_methionine.php">clipped methionine</a>.
               Thanks to Thermo's BioPharma Finder group for reporting the bug.

               <li>Bug fix:  In the text file output, the position of static C-terminal
               modifications in the "modifications" column was always incorrectly reported
               as "1" in the encoded modfication string; this has been corrected.
               Thanks to Thermo's BioPharma Finder group for reporting the bug.

               <li>Bug fix:  When running multiple searches via interfacing with
               CometWrapper.dll under Windows, the output file of subsequent searches was
               not updating. Thus searches were simply overwriting the first output file.
               This has been addressed with a simple variable initialization.
               Thanks to Thermo's BioPharma Finder group for reporting the bug.

               <li>Bug fix:  Corrected a bug where every matched/duplicate protein was not
               always reported due to a rounding/precision issue with storing of the xcorr.
               This was noticed for an edge case (bad spectrum, small database) that should
               not affect users in practice.

               <li>I've sadly had to push off Comet-PTM integration to the next release again.

            </ul>

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> page.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
