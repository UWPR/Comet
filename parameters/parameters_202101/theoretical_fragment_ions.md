### Comet parameter: theoretical_fragment_ions

- This parameter specifies how theoretical fragment ion peaks are represented.
- Even though Comet does not generate/store a theoretical spectrum,
it does calculate fragment ion masses and this parameter controls how
the acquired spectrum intensities at these theoretical mass locations
contribute to the correlation score.
- A value of 0 indicates that the fast correlation score will be
a sum of the intensities at each theortical fragment mass bin and half
the intensity of each flanking bin.
- A value of 1 indicates that the fast correlation score will be
the sum of the intensities at each theoretical fragment mass bin.
- For extremely coarse
[fragment_bin_tol](fragment_bin_tol.html)
values such as the historical ~1 Da bins, a theoretical_fragment_ions
value of 1 is optimal.
- But for narrower bins, such as ~0.3 for ion trap data or ~0.02 for
high-res MS/MS spectra, a value of 0 is optimal to incorporate
intensities from the flanking bins.
- Allowed values are 0 or 1.
- The default value is "1" if this parameter is missing.

Example:
```
theoretical_fragment_ions = 0
theoretical_fragment_ions = 1
```
