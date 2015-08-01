<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: search_enzyme_number</h2>

         <ul>
         <li>The search enzyme is specified by this parameter.
         <li>The list of search enzymes is specified at the end of the comet.params file
         beginning with the line <tt>[COMET_ENZYME_INFO]</tt>.  The actual enzyme list and
         digestion parameters are read here.  So one can edit/add/delete enzyme definitions
         simply be changing the enzyme information.
         <li>This parameter works in conjection with the <a href="num_enzyme_termini.php">num_enzyme_termini</a>
         parameter to define the cleavage rule for fully-digested vs. semi-digested search options.
         <li>This parameter works in conjection with the <a href="allowed_missed_cleavage.php">allowed_missed_cleavage</a>
         parameter to define the miss cleavage rule.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>search_enzyme_number = 0</tt> &nbsp; &nbsp; <i>typically no-enzyme</i>
         <br><tt>search_enzyme_number = 1</tt> &nbsp; &nbsp; <i>typically trypsin</i></p>


         <li>The format of the parameter definition looks like the following:
         <p><pre>[COMET_ENZYME_INFO]
0.  No_enzyme              0      -           -
1.  Trypsin                1      KR          P
2.  Trypsin/P              1      KR          -
3.  Lys_C                  1      K           P
4.  Lys_N                  0      K           -
5.  Arg_C                  1      R           P
6.  Asp_N                  0      D           -
7.  CNBr                   1      M           -
8.  Glu_C                  1      DE          P
9.  PepsinA                1      FL          P
10. Chymotrypsin           1      FWYL        P</pre></p>

         <p>The first column of the parameter definition is the enzyme number. This number list
         must start from 0 and sequentially increase by 1.  The second column is the enzyme name;
         no spaces are allowed in this name field.  The third column is the digestion "sense"
         i.e. a value of "0" specifies cleavage N-teriminal to (before) the specified residues
         in column 4 and a value of "1" specifies cleavage C-terminal to (after) the specified
         residues in column 4.  Column 4 contains the residue(s) that the enzyme cleaves at.
         Column 5 contains the flanking residue(s) that negate cleavage.

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
