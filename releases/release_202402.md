### Comet releases 2024.02

Documentation for parameters for release 2024.02 [can be found here](/Comet/parameters/parameters_202402/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2024.02 rev. 0 (2024.02.0), release date 2024/10/14

- Add fragment ion indexing support.
While fragment ion indexing code was present in the 2024.01 rev. 0 release, 
this is the first Comet release to official support fragment ion indexing
which is a method that was originally implemented by
[MSFragger](https://www.nature.com/articles/nmeth.4256).
In Comet's implementation, the fragment ion index is applied as a
candidate peptide filter prior to performing full cross-correlation analysis.
[Please see this page](https://uwpr.github.io/Comet/notes/20241001_FI.html)
for more details on Comet's fragment ion index.
Thanks to V. Sharma for implementing the modifications permutation code and
to E. Bergstrom, C. McGann, and D. Schweppe for driving the development and testing.
The following are new search parameters specific to this feature.
  - [fragindex_max_fragmentmass](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_max_fragmentmass.html)
  - [fragindex_min_fragmentmass](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_min_fragmentmass.html)
  - [fragindex_min_ions_report](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_min_ions_report.html)
  - [fragindex_min_ions_score](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_min_ions_score.html)
  - [fragindex_num_spectrumpeaks](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_num_spectrumpeaks.html)
  - [fragindex_skipreadprecursors](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_skipreadprecursors.html)

- Allow variable modifications to apply to a subset of proteins.
For example, one can now limit mono-, di-, and tri-methylation
as variable modifications to only histone proteins and not have
to apply those modifications on all proteins in the human database.
This functionality is controlled by the
[protein_modlist_file](https://uwpr.github.io/Comet/parameters/parameters_202402/protein_modlist_file.html)
parameter.  Note there will be issues for post processing analysis, such
as FDR, when applying this feature.  Thanks to C. McGann for the feature request.
