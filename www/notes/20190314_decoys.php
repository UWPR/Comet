<?php include "head.php" ; ?>
<body>

<?php include "analyticstracking.php" ; ?>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post">
         <h1>Notes 2019.03.14</h1>
            <div class="post hr">
               <p>Here's some notes on Comet's internally-generated decoy peptides.

               <ul>
<li>A decoy peptide is generated for each target peptide scored. This guarantees a 1:1 ratio of target to decoy peptides.

<li>Decoy peptides keeps a terminal residue fixed (e.g. the last K or R for a tryptic peptide) and reverses every other residue in the peptide.
    For an enzyme that cleaves n-terminal to a residue, such as AspN, the first residue in the peptide is fixed and every other residue is reversed.
    For example, target tryptic peptide CLSTWGK will generate a decoy peptide GWTSLCK.  A target AspN peptide DSANLPQ will generate a decoy peptide DQPLNAS.

<li>If a residue is modified, the modification will move with the residue in the decoy peptide e.g. M[15.9949]QEATLSK will generate a
    decoy peptide SLTAEQM[15.9949]K. If there were a distance constraint forcing this modification to only appear on the n-terminal
    residue of the peptide, this constraint is not enforced for the decoys.

            </div>
      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>

</body>
</html>

