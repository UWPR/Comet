<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: require_variable_mod</h2>

         <ul>
         <li>This parameter takes in one input value.
         <li>The input value is an integer that controls whether the analyzed peptides
             must contain at least one variable modification i.e. force all reported peptides
             to have a variable modifiation.
             <ul>
             <li>0 = consider both modified and unmodified peptides (default)
             <li>1 = analyze only peptides that contain a variable modification
             </ul>
         </ul>

         <p>Example:
         <br><tt>require_variable_mod = 0</tt> &nbsp; &nbsp; ... <i>modifications not required</i>
         <br><tt>require_variable_mod = 1</tt> &nbsp; &nbsp; ... <i>peptides must contain a variable modification</i>


      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
