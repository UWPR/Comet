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
         <li>Valid values range for this parameter are numbers ranging from -64 to 64.
         <li>A value of 0 will cause Comet to poll the system and launch the same number of threads
             as CPU cores.
         <li>To set an explicit thread count, enter any value between 1 and 64.
         <li>A negative value will spawn the same number of threads as CPU cores less this negative
             value.  For example, a parameter value of "-1" will launch 3 threads for a 4-core CPU
             or 7 threads for an 8-core CPU.  If a large negative value is entered, equal to or
             greater than the number of CPU cores, then 2 search threads will be spawned.
         <li>The default value is "0" if this parameter is missing.
         <li>If the environment variable NSLOTS is defined, the value of that environment variable
             will override this parameter setting and will be the number of threads used in the search.
             This environment variable is typically set in cluster/grid engine software environments.
         </ul>

         <p>Example:
         <br><tt>num_threads = 0</tt>
         <br><tt>num_threads = 8</tt>
         <br><tt>num_threads = -1</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
