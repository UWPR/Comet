<?php include "head.php" ; ?>
<body>

<?php include "analyticstracking.php" ; ?>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post">
         <h1>Notes 2016.03.09</h1>
            <div class="post hr">

               <p>Here's a Thermo Q Exactive datafile, human target + decoy database search (HeLa cells),
                  searched with both the high-high and high-low parameter settings.  This plot
                  compares the high res versus low res fragment ion settings above.  FDR/q-values
                  are calculated based on Comet's E-value scores.  This plot demonstrates the
                  advantages of high res search settings for high res MS/MS data.
               <p><img src="20160309_highres.png">

            </div>
      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>

</body>
</html>
