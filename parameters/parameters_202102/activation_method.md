### Comet parameter: activation_method

- This parameter specifies which scan types are searched.
- If "ALL" is specified, no filtering based on the activation method is applied.
- If any other allowed entry is chosen, only spectra with activation.
- method matching the specified entry are searched.
- This parameter is valid only for mzXML, mzML and mz5 input.
- Allowed values are: ALL, CID, ECD, ETD, ETD+SA, PQD, HCD, IRMPD, SID
- The default value is "ALL" if this parameter is missing.

Example:
```
activation_method = ALL
activation_method = CID
activation_method = ETD
activation_method = HCD
```
