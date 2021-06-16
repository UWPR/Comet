<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: max_duplicate_proteins</h2>

         <ul>
         <li>This parameter defines the maximum number of proteins (identifiers/accessions)
         to report.  If a peptide is present in 6 total protein sequences, there is one
         (first) reference protein and 5 additional duplicate proteins.  This parameter
         controls how many of those 5 additional duplicate proteins are reported.
         <li>If "<a href="decoy_search.php">decoy_search</a> = 2"
         is set to report separate target and decoy results, this parameter will be
         applied to the target and decoy outputs separately.
         <li>Valid values are any integer greater than or equal to 0.
         <li>If set to "-1", there will be no limit on the number of reported additional proteins.
         <li>The default value is "20" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>max_duplicate_proteins = 0&nbsp;&nbsp;&nbsp;</tt>    <i>// one reference protein reported, no additional duplicates</i>
         <br><tt>max_duplicate_proteins = 10&nbsp;&nbsp;</tt>    <i>// one reference protein reported along with ten additional duplicates</i>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
