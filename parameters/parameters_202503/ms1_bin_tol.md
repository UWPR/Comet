### Comet parameter: ms1_bin_tol

- This parameter controls the bin size associated with MS1 spectra for MS1 alignment.
- The bin size defines the mass width associated with a peak
as it is stored internally in an array element.
- This parameter is only relevant for the real-time search interface and
is typically specified in the C# calling program.
- The default value is "1.0005" if the parameter is missing and this currently
the recommended value to use for MS1 real time alignment.


Example:
```
ms1_bin_tol = 1.0005
```
