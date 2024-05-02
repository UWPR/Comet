### Comet parameter: decoy_prefix

- This parameter specifies the prefix string that is pre-pended to
the protein identifier and reported for decoy hits.
- This parameter is only valid when a [decoy_search](decoy_search.html) is performed.
- For example, if the prefix parameter is set to "decoy_prefix = reverse_",
a match to a decoy peptide from protein "ALBU_HUMAN" would return
"reverse_ALBU_HUMAN" as the protein identifier.
- The default value is "DECOY_" if this parameter is missing.

Example:
```
decoy_prefix = DECOY_
decoy_prefix = rev_
decoy_prefix = --any_string_you_want_without_spaces--
```
