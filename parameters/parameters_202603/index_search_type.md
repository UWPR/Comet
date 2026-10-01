### Comet parameter: index_search_type

- This parameter was introduced with release v2026.02.2, corresponding with
a unified .idx format for both peptide index and fragment ion index searches.
- This parameter applies only when the .idx file specified as the search database does
not exist yet and is generated automatically from its FASTA file. It then controls which
type of index is built and searched.
- A value of "0" builds a peptide index and runs a peptide index search.
- A value of "1" builds a fragment ion index and runs a fragment ion index search.
- A value of "-1" means not set; a fragment ion index is built. This is the default if the
parameter is missing.
- An existing .idx file records the index type it was built with (Comet's "-i" command
line option builds a fragment ion index and "-j" builds a peptide index), and a search
against it uses that type; this parameter is ignored. A plain FASTA or PEFF search has no
index and ignores it as well.
- Starting with release 2026.03.0, this parameter is not written by "comet -p"; it is
written as "index_search_type = -1" by "comet -q" (the extended parameter set). A value of
0 or 1 in a params file therefore expresses intent, and Comet reports a warning when such a
value has no effect: the database is a FASTA or PEFF file, the .idx file exists and was
built as the other index type (the warning names the file's type and the command line
option to rebuild it), or an explicit "-i"/"-j" build disagrees with the value (the command
line option wins). A value other than -1, 0 or 1 reports a warning and is treated as -1.

Example:
```
index_search_type = -1     // not set (default); auto-build creates a fragment ion index
index_search_type = 0      // auto-build creates a peptide index
index_search_type = 1      // auto-build creates a fragment ion index
```
