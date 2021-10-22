### Notes 2016.01.01

To create a comet.params file, run the following command and rename the create
file from "comet.params.new" to "comet.params".
```
comet.exe -p
```

Better yet, just download them from this site.  Example 
comet.params files (primary differences are the MS and MS/MS mass tolerance
settings), available on the [parameters page](/Comet/parameters/):
- comet.params.low-low - low res MS1 and low res MS2 e.g. ion trap
- comet.params.high-low - high res MS1 and low res MS2 e.g. LTQ-Orbitrap
- comet.params.high-high - high res MS1 and high res MS2 e.g. Q Exactive or Q-Tof

NOTE:  These links might not be updated to point to parameter files from the
latest version of Comet.  To get the latest versions, just click on the
"Params" button above and access the example parameter files there.

For low-res ms/ms spectra, try the following settings:
```
fragment_bin_tol = 1.0005
fragment_bin_offset = 0.4
theoretical_fragment_ions = 1
spectrum_batch_size = 0
```

For high-res ms/ms spectra, try the following settings:
```
fragment_bin_tol = 0.02
fragment_bin_offset = 0.0
theoretical_fragment_ions = 0
spectrum_batch_size = 10000 (depending on free memory)
```

