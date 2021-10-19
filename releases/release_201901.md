### Comet releases 2019.01

Documentation for parameters for release 2019.01 [can be found
here](/Comet/parameters/parameters_201901/).
[Release download here](https://sourceforge.net/projects/comet-ms/files/).


#### release 2019.01 rev. 5 (2019.01.5), release date 2020/04/06
- Bug fix: a bug in the code could randomly set the "minimum_peaks" parameter
value to a random large number, causing no peaks to be read in, resulting in
blank search results reported. Thanks to C. Bielow for not only reporting the
bug but also debugging the code and implementing the fix.
- Known bug: Nucleotide database searches will fail. This bug was introduced in
version 2019.01.2. A fix has been implemented; the 2019.01.5 release binaries
were updated with this fix on 2020/05/28. Thanks to N. Tay for reporting the
issue.
- Known bug: Searches of mzML and mzXML files without scan index do not work
under Windows. A fix has been implemented; the 2019.01.5 release binaries were
updated with this fix on 2020/05/28. Thanks to R. Marrisen for reporting the
issue.

#### release 2019.01 rev. 4 (2019.01.4), release date 2020/01/15
- Add support for searching mzML and mzXML files that do not contain the
options scan index. Such files would previously have not been searched as
MSToolkit would throw an error message about the missing index.
- Bug fix: PEFF substitutions on flanking residues, generating a new peptide
that would otherwise not be analyzed, are treated more rigorously.

#### release 2019.01 rev. 3 (2019.01.3), release date 2019/12/06
- Bug fix: Searches against mzML files would report the wrong scan number. The
issue was due to the MSToolkit update for release 2019.01.0. Using the latest
MSToolkit commit 2021e7e from 12/3/19 fixes this error. Thanks to Z. Sun for
reporting the bug.

#### release 2019.01 rev. 2 (2019.01.2), release date 2019/11/18
- Bug fix: introduced in the 2019.01 rev. 0 release, Comet would not properly
handle a "clip_nterm_methionine" search. This bug would manifest as either a
segmentation fault or as a NULL character reported for a flanking residue due
to not properly tracking the shortened protein length when the start methione
is clipped off. Thanks to the Villen Lab and R. Johnson for reporting the
issue.

#### release 2019.01 rev. 1 (2019.01.1), release date 2019/09/06
- Known bug: a NULL character can show up as the flanking residue
(peptide_next_aa attribute in pep.xml output) for an internal decoy match.
- In Percolator .pin output, change ExpMass and CalcMass from neutral masses to
singly protonated masses.  Thanks to W. Fondrie and the Crux team for reporting
that Percolator expects these masses to be singly protonated.
- Bug fix:  correct missing residue in StaticMod header entry of SQT output.
Thanks to A. Zelter for reporting the issue.
- Bug fix:  update database indexing and index search to correctly handle
terminal variable modifications.  Thanks to T. Zhao for reporting the issue.

#### release 2019.01 rev. 0 (2019.01.0), release date 2019/08/19
- Known bug: database indexing fails with variable modifications; particularly
with variable terminal mods. See the release_2019011 branch which contains a
fix.
- Add support for user specified fragment neutral loss ions (such as phosphate
neutral loss) via the addition of an 8th field to the "variable_mod01 to
variable_mod09" parameters.
- Add support for user specified precursor neutral loss ions using the
"precursor_NL_ions" parameter.
- Add "max_duplicate_proteins" parameter which can limit the number of protein
identifiers reported/returned for a given peptide.
- Add "peptide_length_range" parameter. This parameter allows the specification
of a minimum and maximum peptide length.
- Add "search_enzyme2_number" parameter. Allows optional selection of a second
digestion enzyme. Enzyme specificity and missed cleavage settings are are
shared between both "search_enzyme_number" and "search_enzyme2_number".
- Update "max_variable_mods_in_peptide" parameter to support value 0.
- In the example comet.params files available here and the when generated using
"comet -p", the "spectrum_batch_size" parameter is now set to 15000 instead of
0. For high-res "fragment_bin_tol" settings, Comet should use less than 6GB of
memory with a 15K batch size with very little impact on search times compared
to loading and searching all spectra at once. If you have a computer with lots
of ram, feel free to set this parameter value to 0 to gain a bit of search
speed. Note that when this parameter is missing, its default value is 0.
- Added indexed database support. This was implemented to support real-time
searching. A database index can be created with the command "comet.exe -i"
which will create an indexed database of the database specified in the
comet.params file. Static and variable modifications specified in the
comet.params file will be stored in the index database. Note an indexed
database (with .idx extension) can be searched if you specify the indexed
database file as the search database ("database_name"). Searching an indexed
database only uses a single thread so the "num_threads" parameter is ignored if
an indexed database is specified.
- Support real-time searching of single spectra against an indexed database
with a C# interface for CometWrapper.dll. This includes an example C# program
(RealtimeSearch/Search.cs) that loops through all scans of a Thermo raw file
using Thermo's RawFileReader and sequentially calling DoSingleSpectrumSearch()
to run the search. Thanks to D. Schweppe and the Gygi lab at Harvard and D.
Bailey at Thermo for contributions to this project. Some
[notes on real-time search support are available here](/Comet/notes/20190820_indexdb.html).
- Update MSToolkit from b41b33f commit on 1/30/19
- Bug fix: error with logic in analysis in target-decoy searches where
erroneous target and decoy fragment ions could be generated.
- Bug fix: add support for parsing '*' in the sequence which was removed in the
2018.01 rev. 0 release. This character is treated as a stop codon; residues
flanking this character are valid search enzyme termini.
- Features that didn't make the cut and are targeted for the next release:
mzIdentML output and Comet-PTM functionality.
- 
