<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: decoy_prefix</h2>

         <ul>
         <li>This parameter specifies the prefix string that is pre-pended to
         the protein identifier and reported for decoy hits.
         <li>This parameter is only valid when a <a href="decoy_search.php">decoy_search</a>
         is performed.
         <li>For example, if the prefix parameter is set to "decoy_prefix = reverse_",
         a match to a decoy peptide from protein "ALBU_HUMAN" would return
         "reverse_ALBU_HUMAN" as the protein identifier.
         <li>The default value is "DECOY_" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>decoy_prefix = DECOY_</tt>
         <br><tt>decoy_prefix = rev_</tt>
         <br><tt>decoy_prefix = --any_string_you_want_without_spaces--</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
