### Comet parameter: peff_format

- Specifies whether the database is a PEFF file or normal FASTA.
- Valid values are 0, 1, 2, 3, 4, 5.
- Set this parameter to 0 to search a normal FASTA file, ignoring any PEFF annotations if present.
- Set this parameter to 1 to search PEFF PSI-MOD modifications and amino acid variants.
- Set this parameter to 2 to search PEFF Unimod modifications and amino acid variants.
- Set this parameter to 3 to search PEFF PSI-MOD modifications, skipping amino acid variants.
- Set this parameter to 4 to search PEFF Unimod modifications, skipping amino acid variants.
- Set this parameter to 5 to search PEFF amino acid variants, skipping PEFF modifications.
- The default value is "0" if this parameter is missing.
- Modifications are read from the "\ModResPsi" (values 1 and 3) or "\ModResUnimod"
(values 2 and 4) header attributes, with masses looked up in the [peff_obo](peff_obo.html)
file. Amino acid variants are read from the "\VariantSimple" and "\VariantComplex"
attributes (values 1, 2 and 5).
- [peff_obo](peff_obo.html) must be set for any non-zero value, including 5.
- Each peptide is analyzed with at most one PEFF modification or one PEFF amino acid
variant, never both and never two of either. A PEFF modification can be combined with
regular [variable modifications](variable_modXX.html) on other residues and does not count
toward [max_variable_mods_in_peptide](max_variable_mods_in_peptide.html).
- PEFF annotations are applied only when searching the PEFF file itself, which is always a
regular (non-indexed) search regardless of the [index_search_type](index_search_type.html)
setting. They are not carried into an indexed database (.idx file) built from the PEFF file.
- Release 2026.03.0 fixes these PEFF header parsing issues:
  - Annotation identifiers ("# HasAnnotationIdentifiers=true", e.g.
    "\ModResPsi=(1:25|MOD:00798|half cystine)") are now handled correctly; earlier releases
    read the label as the position, so labeled modifications were placed on the wrong residue
    and labeled variants were dropped.
  - Modification names containing parentheses (e.g. "N6-(4-amino-2-hydroxybutyl)-L-lysine")
    no longer cause entries to be misread or dropped.
  - A "\VariantSimple" entry with an empty residue field is now ignored instead of being
    searched as a variant with the previous entry's residue.
- Set [peff_verbose_output](peff_verbose_output.html) to 1 to report annotation entries
that are ignored.

Example:
```
peff_format = 0
peff_format = 1
peff_format = 5
```
