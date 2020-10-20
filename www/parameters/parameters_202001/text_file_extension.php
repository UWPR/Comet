<?php include "head.php" ; ?>
<html>
<body>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post hr">

         <h2>Comet parameter: text_file_extension</h2>

         <ul>
         <li>This parameter controls whether the output text file, as
         controlled by the parameter
         <a href="output_txtfile.php">output_txtfile</a>,
         will have a default ".txt" file extension or a custom extension
         specified by this parameter.
         <li>If this parameter value is left blank, the output text file will
         have the default ".txt" file extension.
         <li>To specify a custom extension, set the parameter value to any
         string.  For example, a parameter value "csv" will generate text
         files of the name "BASENAME.csv".
         </ul>

         <p>Example:
         <br><tt>text_file_extension = </tt>
         <br><tt>text_file_extension = any_string_you_want_without_spaces</tt>

      </div>
   </div>
   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>
</html>
