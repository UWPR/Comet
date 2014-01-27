<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: variable_mod2</h2>

         <ul>
         <li>This parameter specifies the 2nd of 6 variable modifications.
         <li>There are 4 entries that are associated with this parameter.
            <ul>
            <li>The first entry is a decimal value specifying the modification mass difference.
            <li>The second entry is the residue(s) that the modifications are possibly applied to.
                If more than a single residue is modified by the same mass difference, list them
                all as a string.
            <li>The third entry is a integer 0 or 1 to specify whether the modification is a
                variable modification (0) or a binary modification (1).
                <ul>
                <li>A variable modification analyzes all permutations of modified and unmodified
                    residues.
                <li>A binary modification analyzes peptides where all residues are either
                    modified or all residues are not modified.
                </ul>
            <li>The fourth entry is an integer specifying the maximum number of modified residues
                possible in a peptide for this modification entry.
            </ul>
         <li>In the output, this first modification is encoded with the character '#' in the peptide string.
         <li>The default value is "0.0 null 0 4" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>variable_mod2 = 15.9949 M 0 3</tt>
         <br><tt>variable_mod2 = 79.966331 STY 0 3</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
