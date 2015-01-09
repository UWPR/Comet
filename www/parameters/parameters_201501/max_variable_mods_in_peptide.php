<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: max_variable_mods_in_peptide</h2>

         <ul>
         <li>This parameter takes in two input values.
         <li>The first input value is an integer that specifies the total/maximum number of
             residues that can be modified in a peptide.
             <ul>
             <li>As opposed to specifying the maximum number of variable modifications for each
                 of the 6 possible variable modifications, this entry limits the global number
                 of variable mods possible in each peptide.
             <li>The default value is "10" if this parameter is missing.
             </ul>
         <li>The second input value is an integer that controls whether the analyzed peptides
             must contain at least one variable modification i.e. force all reported peptides
             to have a variable modifiation.
             <ul>
             <li>0 = analyze both modified and unmodified peptides
             <li>1 = analyze only peptides that contain a variable modification
             </ul>
         </ul>

         <p>Example:
         <br><tt>max_variable_mods_in_peptide = 6 0</tt> &nbsp; &nbsp; ... <i>modifications not required</i>
         <br><tt>max_variable_mods_in_peptide = 10 1</tt> &nbsp; &nbsp; ... <i>peptides must contain a modification</i>


      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
