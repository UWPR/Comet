### Comet parameter: minimum_xcorr

- A decimal value to set the minimum xcorr score for reporting.
- Peptides must score higher than this minimum_xcorr value for storing/reporting.
- The default value is 0.0 (specifically 1e-8).  For Crux compiled Comet, the default value is -999.
- Very poor scoring peptide matches can generate negative xcorr scores so set this cutoff to a large negative value to get all PSMs reported.

Example:
```
minimum_xcorr = -999.0
```
