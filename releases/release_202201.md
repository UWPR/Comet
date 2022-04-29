### Comet releases 2022.01

Documentation for parameters for release 2022.01 [can be found here](/Comet/parameters/parameters_202201/).

Download release [here](https://github.com/UWPR/Comet/releases/tag/v2022.01.0).

#### release 2022.01 rev. 0 (2022.01.0), release date 2022/05/02
- Add support for the VariantComplex entries in PEFF databases.
  These are annotated as "sequence_substitution" elements in the pep.xml
  output.  This functionality was implemented by M. Hoopmann and was actually
  present in the 2021.02 release.  Additional updates to correctly support
  the clip_nterm_methionine parameter in conjunction with PEFF searches is
  implemented with this release.
- For the .pin output, decoy entries are now annotated with the "-1" decoy label
  under the "Label" column.  Previously, the decoy annotations were supported
  only with Comet's internal decoy searches. With this change, for "decoy_search = 0"
  searches,  any database entries that match the "decoy_prefix" text will be
  annotated with the "-1" decoy label.
- Bug fix:  addressed ThreadPool code that was causing memory leaks and possible
  segfaults observed under Linux.  Thanks to D. Shteynberg and M. Hoopmann.
- There are no parameters changes so this version will work with comet.params files annotated as being
  for versions 2022.01, 2021.01 and 2020.01.
