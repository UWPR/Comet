<?php include "head.php" ; ?>
<body>

<?php include "analyticstracking.php" ; ?>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post">
         <h1>Notes 2017.10.05</h1>
            <div class="post hr">
               <p>Here's some notes on setting Comet search parameters for various isotopic/isobaric labeling strategies.

               <ul>
<li>ITRAQ 4-plex

<p>The 4-plex reagent has different  monoisotopic mass values for 114 (144.105918), 115 (144.09599), and 116/117 (144.102063).  The mass value used below is derived from averaging the three monoisotopic masses.  The "clear_mz_range" parameter is used to remove any reporter ion signal from the spectra so that they aren't matched as fragment ions.

   <p><tt>add_Nterm_peptide = 144.10253
   <br>add_K_lysine = 144.10253
   <br>clear_mz_range = 113.5 117.5</tt>


<li>ITRAQ 8-plex:

<p>Similarly, the 8-plex reagent has two different set of masses for 115/118/119/121 (304.199040) and 113/114/116/117 (304.205360).  The mass modification below is the average of the two.

   <p><tt>add_Nterm_peptide = 304.2022
   <br>add_K_lysine = 304.2022
   <br>clear_mz_range = 112.5 121.5</tt>


<li>TMT 2-plex: 

   <p><tt>add_Nterm_peptide = 225.155833
   <br>add_K_lysine = 225.155833
   <br>clear_mz_range = 125.5 127.5</tt>


<li>TMT 6-plex and 10-plex:

   <p><tt>add_Nterm_peptide = 229.162932
   <br>add_K_lysine = 229.162932
   <br>clear_mz_range = 125.5 131.5</tt>


<li>SILAC 4 Da: 

<p>There are a number of different SILAC reagents with a ~4 Da  modification (based on combinations of C13 and N15), each with different sites of specificity.  My example below is for the 15N(4) reagent applied to R residues.  Adjust the modification mass and residue(s) applied to as necessary.

<p>To perform a mixed light/heavy search using a variable modification search in binary mode (binary mode = no mixing light and heavy modifications within a peptide so all lysine residues are considered light or all lysine residues are considered light):

   <p><tt>variable_mod01 = 3.988140 R 1 3 -1 0 0</tt>

<p>To search just the heavy labeled sample, you can apply a static modification:

   <p><tt>add_R_lysine = 3.988140</tt>


<li>SILAC 6 Da: 

<p>I'm using the 13C(6) SILAC mass in the example below assuming it's applied to both K and R; adjust as necessary.  There's at least one more SILAC reagent with ~6 Da modification mass and different residue specificity: 13C(5) 15N(1)

<p>To perform a mixed light/heavy search using a variable modification search in binary mode (binary mode = no mixing light and heavy modifications within a peptide so all lysine residues are considered light or all lysine residues are considered light):

   <p><tt>variable_mod01 = 6.020129 KR 1 3 -1 0 0</tt>

<p>To search just the heavy labeled sample, you can apply a static modification:

   <p><tt>add_K_lysine = 6.020129
   <br>add_R_arginine = 6.020129</tt>


<li>SILAC 8 Da: 

<p>The example below is for 13C(6) 15N(2) on K residues.

<p>Variable (binary) modification search:

   <p><tt>variable_mod01 = 8.014199 K 1 3 -1 0 0</tt>

<p>Static modification for just the heavy labeled search:

   <p><tt>add_K_lysine = 8.014199</tt>


            </div>
      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>

</body>
</html>

