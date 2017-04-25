<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: variable_mod01 through variable_mod09</h2>

         <ul>
         <li>There are 7 entries/settings that are associated with these parameters:
            <ul>
            <li>The first entry is a decimal value specifying the modification mass difference.
            <li>The second entry is the residue(s) that the modifications are possibly applied to.
                If more than a single residue is modified by the same mass difference, list them
                all as a string.  Use 'n' for N-terminal modfication and 'c' for C-terminal modification.
            <li>The third entry is a integer to specify whether the modification is a
                variable modification (0) or a binary modification (non-zero value).
                <ul>
                <li>0 = variable modification analyzes all permutations of modified and unmodified
                    residues.
                <li>non-zero value = A binary modification analyzes peptides where all residues are either
                    modified or all residues are not modified.
                </ul>
                Note that if you set the same binary modification value in multiple "variable_mod?" parameter
                entries, Comet will treat those variable modifications as a "binary set".  This means
                that all modifiable residues in the "binary set" must be unmodified or modified.  Multiple
                "binary sets" can be specified by setting a different binary modification value e.g.
                use "1" for all modifications in set 1, and "2" or all modifications in set 2.
                Binary groups were added with version 2015.02 rev. 1.
            <li>The fourth entry is an integer specifying the maximum number of modified residues
                possible in a peptide for this modification entry.
            <li>The fifth entry specifies the distance the modification is applied to from the
                respective terminus:
                <ul>
                <li>-1 = no distance contraint
                <li>0 = only applies to terminal residue
                <li>1 = only applies to terminal residue and next residue
                <li>2 = only applies to terminal residue through next 2 residues
                <li><i>N</i> = only applies to terminal residue through next <i>N</i> residues where <i>N</i> is a positive integer
                </ul>
            <li>The sixth entry specifies which terminus the distance constraint is applied to:
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
         <li>Modification codes for variable_mod01 through variable_mod09 (for some outputs):  *, #, @, ^, ~, $, %, !, +
         <li>The default value is "15.9959 M 0 3 -1 0 0" if this parameter is missing for
             variable_mod01.
         <li>The default value is "0.0 X 0 3 -1 0 0" if this parameter is missing for
             variable_mod02 through variable_mod09.
         </ul>

         <p>Example:
         <br><tt>variable_mod01 = 15.9949 M 0 3 -1 0 0</tt>
         <br><tt>variable_mod02 = 79.966331 STY 0 3 -1 0 0</tt> &nbsp; &nbsp; ... <i>possible phosphorylation on any S, T, Y residue</i>
         <br><tt>variable_mod02 = 79.966331 STY 0 3 -1 0 1</tt> &nbsp; &nbsp; ... <i>force peptide IDs to contain at least one phosphorylation mod</i>
         <br><tt>variable_mod01 = 42.010565 nK 0 3 -1 0 0</tt> &nbsp; &nbsp; ... <i>acetylation mod to lysine and N-terminus of all peptides</i>
         <br><tt>variable_mod01 = 15.994915 n 0 3 0 0 0</tt> &nbsp; &nbsp; ... <i>oxidation of protein's N-terminus</i>
         <br><tt>variable_mod01 = 28.0 c 0 3 8 1 0</tt> &nbsp; &nbsp; ... <i>modification applied to C-terminus as long as the C-term residue is one of last 9 residues in protein</i>
         <br><tt>variable_mod03 = -17.026549 Q 0 1 0 2 0</tt> &nbsp; &nbsp; ... <i>cyclization of N-terminal glutamine to form pyroglutamic acid (elimination of NH3)</i>
         <br><tt>variable_mod04 = -18.010565 E 0 1 0 2 0</tt> &nbsp; &nbsp; ... <i>cyclization of N-terminal glutamic acid to form pyroglutamic acid (elimination of H2O)</i>

         <p>Here is a binary modification search example of triple SILAC plus acetylation of lysine.
         The SILAC modifications are "R +6 and K +4" (medium) and "R +10 and K +8" (heavy).
         In conjunction with K +42 acetylation, the binary modification sets would be
         "R +6, K +4, K +4+42" for <font color="blue">SILAC medium (binary group 1)</font> and
         "R +10, K +8, K +8+42" for <font color="red">SILAC heavy (binary group 2)</font>.
         Mass values are listed with no precision for clarity; definitely use precise
         modification masses in practice.

         <br><tt>variable_mod01 = 42.0 K 0 3 -1 0 0</tt>
         <br><tt>variable_mod02 =&nbsp; 6.0 R <font color="blue">1</font> 3 -1 0 0</tt>
         <br><tt>variable_mod03 =&nbsp; 4.0 K <font color="blue">1</font> 3 -1 0 0</tt>
         <br><tt>variable_mod04 = 46.0 K <font color="blue">1</font> 3 -1 0 0</tt>
         <br><tt>variable_mod05 = 10.0 R <font color="red">2</font> 3 -1 0 0</tt>
         <br><tt>variable_mod06 =&nbsp; 8.0 K <font color="red">2</font> 3 -1 0 0</tt>
         <br><tt>variable_mod07 = 50.0 K <font color="red">2</font> 3 -1 0 0</tt>
         <br><tt>variable_mod08 =&nbsp; 0.0 X 0 3 -1 0 0</tt>
         <br><tt>variable_mod09 =&nbsp; 0.0 X 0 3 -1 0 0</tt>


      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
