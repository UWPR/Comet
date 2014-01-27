<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: variable_C_terminus_distance</h2>

         <ul>
         <li>This parameter affects how the <a href="variable_C_terminus.php">variable_C_terminus</a>
             parameter is applied.
         <li>The variable modification on the c-terminus can be applied to
            <ul>
            <li>all peptides analyzed by entering a value of -1
            <li>only peptides containing the protein's c-terminus by entering a value of 0
            <li>any positive interger <i>N</i> will have the program consider modifications on
                the c-terminus and next <i>N</i> residues (effectively N+1 residues).
            </ul>
         <li>The default value is "-1" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>variable_C_terminus_distance = -1</tt> &nbsp; <i>Applied to all peptides</i>
         <br><tt>variable_C_terminus_distance = 0</tt>  &nbsp; &nbsp; <i>Applied only to peptides containing protein's c-terminus</i>
         <br><tt>variable_C_terminus_distance = 3</tt>  &nbsp; &nbsp; <i>Applied on any peptide who's c-terminus is one of last 4 residues (c-term &amp; next 3)</i>
         <br><tt>variable_C_terminus_distance = 20</tt> &nbsp; <i>Applied on any peptide who's c-terminus is one of last 21 residues (c-term &amp; next 20)</i>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
