<?php include "head.php" ; ?>
<body>

<?php include "analyticstracking.php" ; ?>
<?php include "topmenu.php" ; ?>
<?php include "imgbar.php" ; ?>

<div id="page">
   <div id="content_full">
      <div class="post">
         <h1>Notes 2016.06.13</h1>
            <div class="post hr">
               <p>If you have a problem running a Comet search, please follow these steps:

               <p>First, always make sure you are running <a href="/release/">the latest version of Comet</a>.
               I'm going to always ask you to do this first when reporting a bug as there's a
               chance that the issue has already been resolved with the most recent release.
               If you're already using the latest release, good for you!

               <p>To update your Comet binary, go click on the "<a href="https://sourceforge.net/projects/comet-ms/files/">download</a>"
               tab above.  Then click on the file named "comet_binaries_<i>XXXXXXX</i>.zip" to download the
               most recent release of Comet (where <i>XXXXXXX</i> is the version string). Note that as
               of 2016/06/13, the most recent release is 2016.01 rev. 2 and the actual file to download
               is "comet_binaries_2016012.zip".  The rest of the instructions below assume this is the
               latest release.  If not, please update the version numbers in place of "2016012" when
               referenced below.
              
               <p>If you are running under Windows, the file you want from the zip archive is
               "comet.2016012.win64.exe".  For TPP users, find your current Comet binary
               (should be at c:\inetpub\tpp-bin\comet.exe) and replace it with the binary in this
               zip file (rename comet.2016012.win64.exe to c:\inetpub\tpp-bin\comet.exe).  Also
               update your Comet search parameters file comet.params with a version appropriate
               for release 2016.01 rev 2.  You can find updated example comet.params files in the
               "parameters" tab above.  If you download one of those, such a the file comet.params.high-low
               for high-res MS1 and low-res MS2 data, just remember to change the name to simply
               "comet.params" and update any parameter settings as needed.

               <p>If you are running under linux, try the binary "comet.2016012.linux.exe".  This
               binary was compiled under Red Hat Enterprise Linux.  If this binary doesn't work
               for you, I have access to Ubuntu, Fedora, and Mint machines on which I can compile the
               latest release for you.  Email me at jke000 at gmail.com for these.  Or simply
               download the source release "comet_source.2016012.zip", unpack, and compile by
               running "make".

               <p>If  you still have a bug to report after running your search with the latest Comet,
               I will need access to your spectrum file (raw, mzXML, mzML, mgf, ms2), comet.params,
               and sequence database.  Send those to me via some file sharing tool like Dropbox,
               OneDrive, or an FTP/html link.  Also include a description of the problem and error
               message, ideally with a screen capture of the output and I'll do my best to address
               the issue as time permits.
            </div>
      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>

<?php include "footer.php" ; ?>

</body>
</html>

