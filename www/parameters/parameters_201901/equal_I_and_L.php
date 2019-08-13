<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: equal_I_and_L</h2>

         <ul>
         <li>This parameter controls whether the Comet treats isoleucine (I) and leucine (L)
         as the same/equivalent with respect to a peptide identification.
         <li>For low-energy fragmentation, there's no way to distinguish between an I and L
         in a spectrum.  Because of this, it doesn't make sense to assign a spectrum to peptide
         DIGSTK but not DLGSTK.  With "equal_I_and_L = 1", Comet will treat these peptides as
         the same identification and map proteins from either peptide to the output protein list.
         <li>A user can change this behavior and treat a I residue as being different than an L
         residue by setting "equal_I_and_L = 0".
         <li>Valid values are 0, 1:
            <ul>
            <li>0 treats I and L as different
            <li>1 treats I and L as the same
            </ul>
         <li>The default value is "1" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>equal_I_and_L = 0</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
