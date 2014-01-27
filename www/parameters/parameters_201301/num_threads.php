<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: num_threads</h2>

         <ul>
         <li>This parameter controls the number of processing threads that will be spawned for a search.
         Ideally the number of threads is set to the same value as the number of CPU cores available.
         <li>Valid values range for this parameter are numbers ranging from 0 to 32.
         <li>A value of 0 will cause Comet to poll the system and launch the same number of threads
         as CPU cores.  This is the default setting.
         <li>To set an explicit thread count, enter any value between 1 and 32.
         </ul>

         <p>Example:
         <br><tt>num_threads = 0</tt>
         <br><tt>num_threads = 8</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
