<meta charse ="UTF-8">

<div id="page">
   <div id="content_full">
      <div class="post hr">
         <h1>Comet release 2021.01</h1>

            <p>Documentation for parameters for release 2021.01
            <a href="/parameters/parameters_202101/">can be found here</a>.

            <ul>
               <b>release 2021.01 rev. 0 (2021.01.0), release date 2020/06/??</b>

               <li>New ThreadPool code by D. Shteynberg.

               <li>"old_mods_encoding" parameter for SQT output.

               <li>Make flanking (previous and next) residue reporting consistent when a peptide
               is present in a protein multiple times and thus could have different flanking
               residues within the same protein. Previous version did not consistently
               report the same set of flanking residues in repeated/replicate searches.

               <li>Update E-value calculation using a better xcorr cummulative distribution
               tail fit algorithm.

               <li>The Windows Visual Studio solution is updated to Visual Studio 2019 
               with v142 build tools.

            </ul>

            <p>Go download from the <a href="https://sourceforge.net/projects/comet-ms/files/">download</a> page.

      </div>
   </div>

   <div style="clear: both;">&nbsp;</div>
</div>
