<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: clip_nterm_methionine</h2>

         <ul>
         <li>This parameter controls whether Comet will automatically remove
         the N-terminal methionine from a sequence entry.
         <li>If set to 0, the sequence is analyzed as-is.
         <li>If set to 1, any sequence with an N-terminal methionine will be
         analyzed as-is as well as with the methionine removed.  This means
         that any N-terminal modifications will also apply (if appropriate)
         to the peptide that is generated after the removal of the methionine.
         <li>Valid values are 0 and 1.
         </ul>

         <p>Example:
         <br><tt>clip_nterm_methionine = 0</tt>
         <br><tt>clip_nterm_methionine = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
