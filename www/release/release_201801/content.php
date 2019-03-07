<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2018.01</h1>
            <ul>
               <b>release 2018.01 rev. 4 (2018.01.4), release date 2019/02/13</b>
               <li>Bug fix: the flanking/next amino acid is not reported correctly as a dash "-"
                   character when the last residue in the peptide is the last residue in the protein.
                   This appears to occur in short sequence entries where the identified peptide is the
                   full length sequence entry with a clipped/skipped N-term methionine residue
                   ("<a href="/parameters/parameters_201801/clip_nterm_methionine.php">clip_nterm_methionine</a>").
                   Thanks to F. Yu for reporting this.
               <li>Performance fix: in release 2018.01.3, a protein string length function call was
                   added to a termini check that gets called many times. This had a significant impact
                   on performance and has been addressed. For example, a search taking 7m:42s on version
                   2018.01.3 search was reduced back to 2m:33s with version 2018.01.4.
               <li>Performance fix: if the
                   "<a href="/parameters/parameters_201801/scan_range.php">scan_range</a>"
                   parameter is specified, Comet would sequentially read each scan header starting from
                   scan 1 in order to reach the first scan in the
                   "<a href="/parameters/parameters_201801/scan_range.php">scan_range</a>"
                   parameter. Comet will now jump to that first scan directly, saving on unnecessary
                   file parsing.
               <li>The Windows version of this release is now compiled with Microsoft Visual Studio 2017.
                   Previous Windows releases were compiled with VS 2010.
               <li>Known bug:  
                   "<a href="/parameters/parameters_201801/max_variable_mods_in_peptide.php">max_variable_mods_in_peptide</a>"
                   does not support "0". Until this is addressed, do not specify any variable mods
                   if you do not want to search with variable mods.
            </ul>
            <ul>
               <b>release 2018.01 rev. 3 (2018.01.3), release date 2018/12/05</b>
               <li>Bug fix: the
                   "<a href="/parameters/parameters_201801/clip_nterm_methionine.php">clip_nterm_methionine</a>"
                   parameter has been broken since the 2017.01 release; it works again.  Thanks to
                   A.T.Guler for reporting the bug.
               <li>Bug fix: add a missing tab in Crux-compiled text output.
            </ul>
            <ul>
               <b>release 2018.01 rev. 2 (2018.01.2), release date 2018/06/13</b>
               <li>Known bug: if no variable mod is specified, the static modification
                   mass reported in the "mod_aminoacid_mass" element of the pepXML output
                   can be corrupted. This bug is present as far back as the Comet 2017.01
                   release and only manifests itself when at least one static modification
                   is specified and no variable modifications are specified.
               <li>Bug fix: remove version update check. In some instances, timeout and
                   host access issues were causing Comet to abort searches. The
                   "<a href="/parameters/parameters_201801/skip_updatecheck.php">skip_updatecheck</a>"
                   parameter is now deprecated.
            </ul>
            <ul>
               <b>release 2018.01 rev. 1 (2018.01.1), release date 2018/05/16</b>
               <li>Bug fix:  
                   "<a href="/parameters/parameters_201801/equal_I_and_L.php">equal_I_and_L</a>"
                   was not properly implemented in previously releases.  This parameter controls
                   whether Comet treats isoleucine and leucine residues as being the same (yes
                   by default) since they cannot be distinguished in most data.
               <li>For Crux compiled version, set the output files to be <i>basename</i>.txt instead
                   of <i>basename</i>.target.txt for instances when 
                   "<a href="/parameters/parameters_201801/decoy_search.php">decoy_search</a>"
                   is not set to "2" i.e. targets only or combined targets plus decoys. This also
                   applies to .sqt and .pep.xml outputs.
            </ul>
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
                   also triggers a Comet Google analytics hit. Setting
                   "<a href="/parameters/parameters_201801/skip_updatecheck.php">skip_updatecheck</a> = 1"
                   will skip this.
               <li>"<a href="/parameters/parameters_201801/isotope_error.php">isotope_error</a> = 5"
                   will search the -1/0/1/2/3 C13 offsets which is what used to be available prior to
                   versions 2017.01. The -1 C13 isotope offset really makes no sense but we've seen
                   cases where a wrong isotope peak, one C13 less than the monoisotopic peak, is listed
                   as the precursor peak. This can occur when a noise peak or a peak from a different
                   peptide appears at the -1 mass location.
               <li>Bug fix:  Starting with 2017.01 rev. 0, not all permutations of variable modifications
                   get analyzed when multiple variable modification is specified. The permutations of
                   modifications would be terminated at the first permutation occurrence of two variable
                   modifications on the same residue. This is now fixed.
               <li>Bug fix:  PEFF parsing of modifications would previously terminate at the first space
                   which could occur within the text of a modification description causing an incomplete
                   set of PEFF modifications to be analyzed; this is now fixed.
               <li>This version of Comet will run with comet.params files from version 2017.01.
            </ul>

            <p>Documentation for parameters for release 2017.01
            <a href="/parameters/parameters_201701/">can be found here</a>.

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> page.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
