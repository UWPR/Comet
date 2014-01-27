<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: mass_type_fragment</h2>

         <ul>
         <li>Controls the mass type, average or monoisotopic, applied to fragment ion calculations.
         <li>Valid values are 0 or 1:
            <ul>
            <li>0 for average masses
            <li>1 for monoisotopic masses
            </ul>


         </ul>

         <p>Example:
         <br><tt>mass_type_fragment = 0</tt>
         <br><tt>mass_type_fragment = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
