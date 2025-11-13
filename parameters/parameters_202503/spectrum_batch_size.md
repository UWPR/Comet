### Comet parameter: spectrum_batch_size

- When this parameter is set to a non-zero value, say 10000, this causes Comet
to load and search roughly 10000 spectra at a time, looping through sets of 10000
spectra until all data have been analyzed.
- This parameter was implemented to simplify searching large datasets that
might not fit into memory if searched all at once.
- The loaded batch sizes might be a little larger than the specified parameter
value (i.e. 10008 spectra loaded when the parameter is set to 10000) due to both
threading and potential charge state considerations when precursor charge state
is not known.
- Valid values are 0 or any positive integer.
- Set this parameter to 0 to load and search all spectra at once.
- Set this parameter to any other positive integer to loop through searching
this number of spectra at a time until all spectra have been analyzed.
- The default value is "0" if this parameter is missing.

Example:
```
spectrum_batch_size = 0
spectrum_batch_size = 5000
spectrum_batch_size = 10000
```
