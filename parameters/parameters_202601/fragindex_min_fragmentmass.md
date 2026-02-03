### Comet parameter: fragindex_min_fragmentmass

- This parameter defines the minimum fragment ion mass to include
  in the fragment ion index.
- The mass is the singly charged fragment mass.
- A larger value will use less memory and generate the fragment ion
  index faster as less fragment ions will be added to the index.
- Specifying too large a value may remove fragment ions
  from consideration that would otherwise aid in peptides passing
  the fragment ion index filter.
- If this value is not specified, the default value is 200.0.
- Valid values are a positive decimal number.

Example:
```
fragindex_min_fragmentmass = 200.0
```
