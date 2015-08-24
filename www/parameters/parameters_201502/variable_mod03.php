<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: variable_mod03</h2>

         <ul>
         <li>This parameter specifies the 3rd of 9 variable modifications.
         <li>There are 7 entries/settings that are associated with this parameter:
            <ul>
            <li>The first entry is a decimal value specifying the modification mass difference.
            <li>The second entry is the residue(s) that the modifications are possibly applied to.
                If more than a single residue is modified by the same mass difference, list them
                all as a string.  Use 'n' for N-terminal modfication and 'c' for C-terminal modification.
            <li>The third entry is a integer 0 or 1 to specify whether the modification is a
                variable modification (0) or a binary modification (1).
                <ul>
                <li>0 = variable modification analyzes all permutations of modified and unmodified
                    residues.
                <li>1 = A binary modification analyzes peptides where all residues are either
                    modified or all residues are not modified.
                </ul>
            <li>The fourth entry is an integer specifying the maximum number of modified residues
                possible in a peptide for this modification entry.
            <li>The fifth entry specifies the distance the modification is applied to from the
                respective protein terminus:
                <ul>
                <li>-1 = no distance contraint
                <li>0 = only applies to terminal residue
                <li>1 = only applies to terminal residue and next residue
                <li>2 = only applies to terminal residue through next 2 residues
                <li><i>N</i> = only applies to terminal residue through next <i>N</i> residues where <i>N</i> is a positive integer
                </ul>
            <li>The sixth entry specifies whichprotein terminus the distance constraint is applied to:
                <ul>
                <li>0 = protein N-terminus
                <li>1 = protein C-terminus
                <li>2 = peptide N-terminus
                <li>3 = peptide C-terminus
                </ul>
            <li>The seventh entry specifies whether peptides are must contain this modification
                <ul>
                <li>0 = not forced to be present
                <li>1 = modification is required 
                </ul>

            </ul>
         <li>In the output, this first modification is encoded with the character '@' in the peptide string.
         <li>The default value is "0.0 null 0 4 -1 0 0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>variable_mod03 = 15.9949 M 0 3 -1 0 0</tt>
         <br><tt>variable_mod03 = 79.966331 STY 0 3 -1 0 0</tt> &nbsp; &nbsp; ... <i>possible phosphorylation on any S, T, Y residue</i>
         <br><tt>variable_mod03 = 79.966331 STY 0 3 -1 0 1</tt> &nbsp; &nbsp; ... <i>force peptide IDs to contain at least one phosphorylation mod</i>
         <br><tt>variable_mod03 = 42.010565 nK 0 3 -1 0 0</tt> &nbsp; &nbsp; ... <i>acetylation mod to lysine and N-terminus of all peptides</i>
         <br><tt>variable_mod03 = 15.994915 n 0 3 0 0 0</tt> &nbsp; &nbsp; ... <i>oxidation of protein's N-terminus</i>
         <br><tt>variable_mod03 = 28.0 c 0 3 8 1 0</tt> &nbsp; &nbsp; ... <i>modification applied to C-terminus as lon as the C-term residue is one of last 9 residues in protein</i>
         <br><tt>variable_mod03 = -17.026549 Q 0 1 0 2 0</tt> &nbsp; &nbsp; ... <i>cyclization of N-terminal glutamine to form pyroglutamic acid (elimination of NH3)</i>
         <br><tt>variable_mod03 = -18.010565 E 0 1 0 2 0</tt> &nbsp; &nbsp; ... <i>cyclization of N-terminal glutamic acid to form pyroglutamic acid (elimination of H2O)</i>


      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
