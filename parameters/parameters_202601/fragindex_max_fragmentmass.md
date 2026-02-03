### Comet parameter: fragindex_max_fragmentmass

- This parameter defines the maximum fragment ion mass to include
  in the fragment ion index.
- The mass is the singly charged fragment mass.
- A smaller value will use less memory and generate the fragment ion
  index faster as less fragment ions will be added to the index.
- Specifying too small a value may remove fragment ions
  from consideration that would otherwise aid in peptides passing the
  fragment ion index filter.
- If this value is not specified, the default value is 2000.0.
- Valid values are a positive decimal number.

Example:
```
fragindex_max_fragmentmass = 1500.0
fragindex_max_fragmentmass = 2000.0
```
