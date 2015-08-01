<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: database_name</h2>

         <ul>
         <li>A full or relative path to the sequence database, in FASTA format, to search. Example databases
         include RefSeq or UniProt.
         <li>Database can contain amino acid sequences or nucleic acid sequences.  If sequences are amino acid
         sequences, set the parameter "nucleotide_reading_frame = 0".  If the sequences are nucleic acid
         sequences, you must instruct Comet to translate these to amino acid sequences.  Do this by setting
         "nucleotide_reading_frame" to a value between 1 and 9.
         <li>There is no default value if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>database_name = /usr/local/db/yeast.fasta</tt>
         <br><tt>database_name = c:\local\db\yeast.fasta</tt>
         <br><tt>database_name = yeast.fasta</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
