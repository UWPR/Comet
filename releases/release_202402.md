### Comet releases 2024.02

Documentation for parameters for release 2024.02 [can be found here](/Comet/parameters/parameters_202402/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2024.02 rev. 0 (2024.02.0), release date 2024/10/14

- This is the first release to official support fragment ion indexing, which
is a method that was originally implemented by [MSFragger](https://msfragger.nesvilab.org/) in 2017.
In Comet's implementation, the fragment ion index is applied as a
candidate peptide filter prior to performing full cross-correlation scoring.
[Please see this note](https://uwpr.github.io/Comet/notes/20241001_FI.html)
for more details on Comet's fragment ion index. The following are
new search parameters specific to this feature.
 - Added [fragindex_max_fragmentmass](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_max_fragmentmass.html)
 - Added [fragindex_min_fragmentmass](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_min_fragmentmass.html)
 - Added [fragindex_min_ions_report](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_min_ions_report.html)
 - Added [fragindex_min_ions_score](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_min_ions_score.html)
 - Added [fragindex_num_spectrumpeaks](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_num_spectrumpeaks.html)
 - Added [fragindex_skipreadprecursors](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_skipreadprecursors.html)
Thanks to V. Sharma for implementing the modifications permutation code and
to E. Bergstrom, C. McGann, and D. Schweppe for driving the development and testing.
- Allow variable modifications to apply to a subet of proteins.
For example, one can apply mono-, di-, and tri-methylation
variable modifications to only histone proteins and not all
human proteins in the database. This functionality is controlled by the
[protein_modlist_file](https://uwpr.github.io/Comet/parameters/parameters_202402/protein_modlist_file.html)
parameter.  Note there will be issues for FDR analysis with
when applying this feature.  Feature requested by C. McGann.
