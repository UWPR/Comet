<?php include "head.php" ; ?>
<body>

<?php include "analyticstracking.php" ; ?>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post">
         <h1>Notes 2015.01.01</h1>
            <div class="post hr">
               <p>If you notice performance issues specifying lots of input files on the command line
                  with Windows, i.e. a creep in memory use with corresponding CPU usage degredation,
                  there's a way to avoid the problem. You can do this by creating a wrapper script batch
                  program which is just a text file with a ".bat" file extension.  Name it something like
                  "runcomet.bat" with the contents as below and execute it instead of executing Comet
                  directly i.e. "<tt>runcomet.bat *.mzXML</tt>".  By running such a batch script, it
                  will invoke individual, sequential instances of Comet to search all input files. 

               <p>Contents of runcomet.bat (you can save-link to file): &nbsp; <a href="runcomet.bat"><tt>for %%A in (%*) do ( comet.exe %%A )</tt></a>

               <p>Specify the full path to the comet.exe binary in the batch file if it does not
                  reside in the same directory as the command is being executed.

               <p>For linux, try: &nbsp; <tt>find . -name '*.mzXML' -print | xargs comet</tt>.
               <br>Or try this simple <a href="runcomet.sh"><tt>runcomet.sh</tt></a> bash script.
                  Download it, make it executable with "chmod +x runcomet.sh" and place it
                  somewhere in your PATH.  Then run searches using "<tt>runcomet.sh *.mzXML</tt>".
                  This script expects the binary named "comet" to also be in your path.
            </div>
      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>

</body>
</html>

