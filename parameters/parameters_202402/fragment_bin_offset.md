### Comet parameter: fragment_bin_offset

- This parameter controls how each fragment bin of size [fragment_bin_tol](fragment_bin_tol)
is defined in terms of where each bin starts.
- For example, assume a [fragment_bin_tol](fragment_bin_tol) of 1.0.  Most intuitively,
the fragment bins would be 0.0 to 1.0, 1.0 to 2.0, 2.0 to 3.0, etc.
This set of bins corresponds to a fragment_bin_offset of 0.0.  However,
consider if we set fragment_bin_offset to 0.5; this would cause the
bins to be 0.5 to 1.5, 1.5 to 2.5, 2.5 to 3.5, etc.
- So this fragment_bin_offset gives one a mechanism to define
where each bin starts and is centered.
- For ion trap data with a [fragment_bin_tol](fragment_bin_tol) of 1.0005,
it is recommended to set fragment_bin_offset to 0.4.
- For high-res MS/MS data, one might use a [fragment_bin_tol](fragment_bin_tol)
of 0.02 and a corresponding fragment_bin_offset of 0.0.
- Allowed values are between 0.0 and 1.0.  The actual offset value is
scaled by the [fragment_bin_tol](fragment_bin_tol) value.
- I know this is esoteric and any normal user should not give this
parameter any thought beyond using the recommended settings.
- The default value is "0.4" if this parameter is missing.

Example:
```
fragment_bin_offset = 0.4
fragment_bin_offset = 0.0
```
