<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: variable_N_terminus_distance</h2>

         <ul>
         <li>This parameter affects how the <a href="variable_N_terminus.php">variable_N_terminus</a>
             parameter is applied.
         <li>The variable modification on the n-terminus can be applied to
            <ul>
            <li>all peptides analyzed by entering a value of -1
            <li>only peptides containing the protein's n-terminus by entering a value of 0
            <li>any positive integer <i>N</i> will have the program consider modifications on the n-terminus and the next <i>N</i> residues
                (effectively the first N+1 residues).
            </ul>
         </ul>

         <p>Example:
         <br><tt>variable_N_terminus_distance = -1</tt> &nbsp; <i>Applied to all peptides</i>
         <br><tt>variable_N_terminus_distance = 0</tt>  &nbsp; &nbsp; <i>Applied only to peptides containing protein's n-terminus</i>
         <br><tt>variable_N_terminus_distance = 1</tt>  &nbsp; &nbsp; <i>Applied on any peptide who's n-terminus is one of first 2 residues (n-term &amp; next 1)</i>
         <br><tt>variable_N_terminus_distance = 12</tt> &nbsp; <i>Applied on any peptide who's n-terminus is one of first 13 residues (n-term &amp; next 12)</i>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
