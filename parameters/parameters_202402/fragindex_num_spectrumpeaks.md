### Comet parameter: fragindex_num_spectrumpeaks

- This parameter defines the number of mass/intensity pairs query against the
  fragment ion index.
- Raw peaks from the query spectrum are stored, starting from the most intense peak.
- The larger this value is set, the slower a search will be as each additional
  peak will need to be queried against the fragment ion index.
- Valid value is a positive integer.

Example:
```
fragindex_num_spectrumpeaks = 100
```
