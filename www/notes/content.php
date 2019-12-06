<div id="page">
   <div id="content_full">
      <div class="post">
         <h1>Miscellaneous Notes</h1>

            <div class="post hr">
               <p>Support:
                  <ul>
                     <li><a target="new" href="http://groups.google.com/group/comet-ms">Comet's Google group</a>.
                     Post your questions, problems, and feature requests here.
                     <li>If you would rather not post to this public forum, feel
                     free to e-mail me directly (jke000 at gmail dot com or engj at uw dot edu).
                  </ul>
            </div>
            <div class="post hr">
               <p>To run Comet, you need one or more input spectral files in mzXML, mzML, mgf, or
                  ms2/cms2 formats and a comet.params file.  Then issue a command such as:
               <ul>
                  <li><tt>comet.exe input.mzXML</tt>
                  <li><tt>comet.exe input.mzML</tt>
                  <li><tt>comet.exe input.mgf</tt>
                  <li><tt>comet.exe input.ms2</tt>
                  <li><tt>comet.exe *.ms2</tt>   <i>multiple inputs supported</i>
               </ul>
            </div>
            <div class="post hr">
               <ul>
               <li>2019/11/04:  <a href="20191104_threading.php">Quick analysis on search times vs. number of search threads.</a>
               <li>2019/08/20:  <a href="20190820_indexdb.php">Misc. notes on indexed database and real-time search support</a>
               <li>2019/03/14:  <a href="20190314_decoys.php">Misc. notes on Comet's decoy peptides</a>
               <li>2017/10/05:  <a href="20171005_isotopiclabeling.php">Parameter settings for isotopic/isobaric labeling (iTRAQ, TMT, SILAC)</a>
               <li>2017/09/06:  <a href="20170906_n15params.php">Parameter settings for N15 heavy search</a>
               <li>2016/06/13:  <a href="20160613_reporting_issues.php">Information I will need when you report a problem/bug</a>
               <li>2016/03/09:  <a href="20160309_highres.php">high res vs. low res fragmention comparison on Q Exactive data</a>
               <li>2016/01/01:  <a href="20160101_parameters.php">Comet parameters info and suggested high-res/low-res settings</a>
               <li>2015/01/01:  <a href="20150101_batch.php">Comet batch processing scripts and avoiding memory creep</a>
               </ul>

            </div>
                  <!--
            <div class="post hr">
               <p>Here's an example of how Comet scales with increasing core count using an 16-core 
                  linux machine with Comet 2015.01 rev 0 (analyzed on 2015/02/24):
<pre>
# threads  runtime (mm:ss)   scaling compared to baseline
-------------------------------------------------
    1          23:55          100% (baseline)
    2          14:23           83%
    4          14:16           42%
    8          19:13           80%
 </pre>
            </div>
            -->

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
