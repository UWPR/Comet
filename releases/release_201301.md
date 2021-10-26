### Comet releases 2013.01

Documentation for parameters for release 2013.01 [can be found
here](http://comet-ms.sourceforge.net/parameters/parameters_201301/).
Download release [here](https://sourceforge.net/projects/comet-ms/files/).

#### release 2013.01 rev 0 (2013.01.0), release date 2013/06/05
- This is the second major release of Comet.
- Comet now supports searching multiple input files i.e. "comet.exe *.mzXML".
Comet will search each input file sequentially, one after the other.
- New command line option '-F<num>' and '-L<num>' to specify the first and last
scan to search. This scan range can also be specified using the scan_range
parameter. Additionally, the scan range can still be specified appended to each
input file i.e. comet.exe file.mzXML:1500-5000. Scan ranges specified appended
to each mzXML file will override the -F/-L command line options which in turn
will override the scan_range parameter.
- Fix bug with '-D' command line parameter not applying the specified sequence
database to override the database specified in the params file.
- Fix bug associated with application of use_NL_ions. An error in the code
logic was not applying neutral loss peaks to all relevant ion series.
- Updated MSToolkit code for more robust reading of input files.
- New parameter: use_sparse_matrix
- Implemented by Mike Hoopman, the main new feature of Comet release 2013.01 is
the ability to use a sparse matrix data representation internally. When using a
small fragment_bin_tol value (i.e. 0.01), the original implementation of Comet
will use a *huge* amount of memory. The sparse
matrix implementation reduces memory use tremendously but searches are slower
than the classical implementation.
- New parameter: spectrum_batch_size
- This parameter defines how many spectra are searched at a time. For example,
if the parameter value were set to 1000 then Comet would loop through searching
about 1000 spectra at a time until all data are analyzed. This can be used for
searching a large dataset that might not fit in memory when searched all at
once.
- New parameter: clear_mz_range
- This parameter removes (i.e. clears out) all peaks within the specified m/z
range prior to any data processing and searching. This functionality was
intended for iTRAQ/TMT type analysis with the goal to ignore the reporter ion
signals from the search.
- Revised parameter: fragment_bin_offset
- In the 2012.01 release, the valid value of this parameter ranged from 0.0 to
the fragment_bin_tol value. Per request, the meaning of this parameter has been
changed such that valid values now range from 0.0 to 1.0 where the actual
offset value scales by fragment_bin_tol. This is not a parameter that most
users should care about; use the recommended values written in the params file
notes.
- Add version number to head of comet.params file.
- Comet will now validate that a compatible params file is being used by
checking the version number written in the comet.params file. This was added to
avoid running Comet using an old/outdated parameters file where an existing
parameter name might have a change in behavior (such as fragment_bin_offset
above). Comet will not run if the version string is not compatible.
- Change format of output filenames when input scan range is specified.
- When searching a scan range, say 1-5000, release 2012.01 would create output
files with the name extension basename.pep.xml:1-5000 or basename.sqt:1-5000.
This version 2013.01 will now name these files as basename.1-5000.pep.xml and
basename.1-5000.sqt. This keeps the file extensions (.pep.xml and .sqt) at the
end of the filenames for better compatibility with downstream tools. Ideally
one uses the spectrum_batch_size parameter and not run individual subset scan
range searches to avoid this naming issue.

For low-res ms/ms spectra, try the following settings:
- fragment_bin_tol = 1.0005
- fragment_bin_offset = 0.4
- theoretical_fragment_ions = 1

For high-res ms/ms spectra, try the following settings:
- fragment_bin_tol = 0.02
- fragment_bin_offset = 0.0
- theoretical_fragment_ions = 0
