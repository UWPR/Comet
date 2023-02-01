### Comet parameter: use_NL_ions

- Controls whether or not neutral loss ions (-NH3 and -H2O from b- and y-ions) are considered in the search.
- The water/ammonia neutral loss peak contributions are applied only for 1+ fragments.
- Valid values are 0 and 1.
- To not use neutral loss ions, set the value to 0.
- To use neutral loss ions, set the value to 1.
- The default value is "0" if this parameter is missing.

Example:
```
use_NL_ions = 0
use_NL_ions = 1
```
