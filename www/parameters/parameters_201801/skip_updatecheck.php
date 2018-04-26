<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: skip_updatecheck</h2>

         <ul>
         <li>Comet will check if there is an updated version available and report if so. This
             also triggers a Comet Google analytics hit.
         <li>When set to 1, the update check will not be performed.
         <li>Valid values are 0 and 1.
         <li>The default value is "0" if this parameter is missing.
         </ul>

         <p>Example:
         <br><tt>skip_researching = 1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
