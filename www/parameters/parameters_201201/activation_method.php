<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: activation_method</h2>

         <ul>
         <li>This parameter specifies which scan types are searched.
         <li>If "ALL" is specified, no filtering based on the activation method
         is applied.
         <li>If any other allowed entry is chosen, only spectra with activation
         method matching the specified entry are searched.
         <li>This parameter is valid only for mzXML, mzML and mz5 input.
         <li>Allowed values are: ALL, CID, ECD, ETD, PQD, HCD, IRMPD
         </ul>

         <p>Example:
         <br><tt>activation_method = ALL</tt>
         <br><tt>activation_method = CID</tt>
         <br><tt>activation_method = ETD</tt>
         <br><tt>activation_method = HCD</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
