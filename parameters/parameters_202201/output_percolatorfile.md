### Comet parameter: output_percolatorfile

- Controls whether to output search results in a [Percolator's](http://percolator.ms)
tab-delimited input format.
- Valid values are 0 (do not output) or 1 (output).
- The default value is "0" if this parameter is missing.
- The created file will have a ".pin" file extension.
- Output columns are:
  - SpecID: an identifier composed of the inputBaseName_scanNumber_chargeState_resultNumber
  - Label: a value of 1 for a target peptide or -1 for a decoy peptide
  - ScanNr: spectrum scan number
  - ExpMass: experimental (measured) peptide neutral mass
  - CalcMass: calculated peptide neutral mass
  - lnrSp: natural log of the preliminary score rank (aka Sp rank)
  - deltLCn:  last dCn in the output list 
  - deltCn:  deltaCn which is the normalized xcorr difference between the top hit and next best hit
  - lnExpect:  natural log of the expectation value or E-value
  - Xcorr:  cross-correlation score
  - Sp:  preliminary score
  - IonFrac:  decimal value representing matched fragment ions count divided by total fragment ions count
  - Mass:  repeat of ExpMass (this column may not even be relevant and could be deprecated in the future) 
  - PepLen:  length of peptide
  - Charge[n]:  boolean, is this a charged n spectrum
  - enzN: boolean, is the peptide's n-terminus consistent with the sample enzyme
  - enzC: boolean, is the peptide's c-terminus consistent with the sample enzyme
  - enzInt: number of missed cleavages
  - lnNumSp: natural log of the number of analyzed peptides within the precursor tolerance
  - dM: normalized mass difference calculated as (ExpMass - CalcMass)/CalcMass.
  - absdM: absolute value of dM
  - Peptide: peptide sequence
  - Proteins: tab delimited list of proteins that contain the peptide

Example:
```
output_percolatorfile = 0
output_percolatorfile = 1
```
