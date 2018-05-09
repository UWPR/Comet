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
         <br><tt>skip_updatecheck = 1</tt>

         <p>If an update is available, you will see "**UPDATE AVAILABLE**" after the version string when a search a run:
         <pre>
Comet version "2018.01 rev. 0"  **UPDATE AVAILABLE**

Search start:  05/08/2018, 06:45:31 AM
- Input file: JE102306_102306_18Mix4_Tube1_01.mzXML
  - Load spectra: 5164
    - Search progress: 100%
    - Post analysis:  done
Search end:    05/08/2018, 06:47:24 AM, 1m:53s</pre>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
