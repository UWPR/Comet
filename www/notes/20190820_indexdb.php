<?php include "head.php" ; ?>
<body>

<?php include "analyticstracking.php" ; ?>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post">
         <h1>Notes 2019.08.20:  indexed database and real-time search</h1>
            <div class="post hr">
               <p>Notes on Comet's indexed database and real-time search support

               <ul>
<li>PEFF modifications/substitutions are currently not supported.

<li>An indexed database search will only use 1 thread.  This is true whether searching via the
DoSingleSpectrumSearch() function or simply performing a standard Comet search against an indexed database.

<li>After setting search parameters (mass range, peptide length range, modifications, enzyme, and FASTA file),
   an indexed database is generated using the "-i" command line option e.g. "comet.exe -i".
   This will create an indexed file with the same file name as the specified FASTA file but with
   an ".idx" extension.  When the specified search database ends in an ".idx" extension, Comet assumes this
   file is a Comet indexed database file and will search this file format appropriately.

<li>Take a look at the Search.cs program in the RealtimeSearch project for an example how how to perform a search
and extract results via the DoSingleSpectrumSearch() function.  This example program will loop through a raw file
and search all MS/MS scans sequentially.  To run this example search:  "realtimesearch.exe example.raw database.fasta.idx".

<li>In order to maintain performance, you are encouraged to limit variable modifications, input database size, number
of missed cleavages, etc.  A 10 millisecond fully tryptic, oxidized methionine yeast search can easily balloon to
hundreds of milliseconds for tryptic human phosphorylation + oxidized methionine variable modification search.

<li>There is a hidden (i.e. doesn't appear in comet.params unless you manually add it) search parameter
"<a href="/parameters/parameters_201901/max_index_runtime.php">max_index_runtime</a>"
that takes an integer value representing the maximum run time in milliseconds for an
indexed database search.  This parameter is intended to abort any particular spectrum search that might
run a long time, returning the best peptide ID analyzed up to that time limit.

<li>Many search parameters are applied during the database indexing step to generate the list of candidate
peptides stored in the indexed database. These include (but are not limited to) enzyme specificity,
modifications, mass and length ranges, etc.  Once the indexed database is generated, setting or
changing these parameters will have no effect when searching the indexed database as they've already
been applied to generate the candidate peptides in the indexed database.  Here is the list of search
parameters that one can change and apply to an indexed database search which will affect the results.
The SearchSettings class in Search.cs gives an example of how these can be set programmatically.
<ul>
<li><a href="/parameters/parameters_201901/peptide_mass_tolerance.php">peptide_mass_tolerance</a>
<li><a href="/parameters/parameters_201901/peptide_mass_units.php">peptide_mass_units</a>
<li><a href="/parameters/parameters_201901/precursor_tolerance_type.php">precursor_tolerance_type</a>
<li><a href="/parameters/parameters_201901/isotope_error.php">isotope_error</a>
<li><a href="/parameters/parameters_201901/fragment_bin_tol.php">fragment_bin_tol</a>
<li><a href="/parameters/parameters_201901/fragment_bin_offset.php">fragment_bin_offset</a>
<li><a href="/parameters/parameters_201901/theoretical_fragment_ions.php">theoretical_fragment_ions</a>
<li><a href="/parameters/parameters_201901/max_fragment_charge.php">max_fragment_charge</a>
<li><a href="/parameters/parameters_201901/max_precursor_charge.php">max_precursor_charge</a>
<li><a href="/parameters/parameters_201901/equal_I_and_L.php">equal_I_and_L.php</a>
<li><a href="/parameters/parameters_201901/max_index_runtime.php">max_index_runtime</a>
</ul>
            </div>
      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>

</body>
</html>

