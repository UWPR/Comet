### Notes 2016.06.13

If you have a problem running a Comet search, please follow these steps:

First, always make sure you are running the latest version of Comet.
I'm going to always ask you to do this first when reporting a
bug as there's a chance that the issue has already been resolved with the most
recent release.  If you're already using the latest release, good for you!

If you are running under Windows, the file you want from the zip archive is
"comet.20XXXXX.win64.exe".  For TPP users, find your current Comet binary
(should be at c:\inetpub\tpp-bin\comet.exe) and replace it with the binary in
this zip file (rename comet.20XXXXX.win64.exe to c:\inetpub\tpp-bin\comet.exe).
Also update your Comet search parameters file comet.params with a version
appropriate for release 20XX.0X rev X.  You can find updated example
comet.params files in the "parameters" tab above.  If you download one of
those, such a the file comet.params.high-low for high-res MS1 and low-res MS2
data, just remember to change the name to simply "comet.params" and update any
parameter settings as needed.

If you are running under linux, try the binary "comet.XXXXXXX.linux.exe".  This
binary was compiled under Red Hat Enterprise Linux.  If this binary doesn't
work for you, I have access to Ubuntu, Fedora, and Mint machines on which I can
compile the latest release for you.  Email me at jke000 at gmail.com for these.
Or simply download the source release "comet_source.XXXXXXX.zip", unpack, and
compile by running "make".

If  you still have a bug to report after running your search with the latest
Comet, I will need access to your spectrum file (raw, mzXML, mzML, mgf, ms2),
comet.params, and sequence database.  Send those to me via some file sharing
tool like Dropbox, OneDrive, or an FTP/html link.  Also include a description
of the problem and error message, ideally with a screen capture of the output
and I'll do my best to address the issue as time permits.

