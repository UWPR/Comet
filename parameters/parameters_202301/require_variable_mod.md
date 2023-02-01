### Comet parameter: require_variable_mod

- This parameter takes in one input value.
- The input value is an integer that controls whether the analyzed peptides
must contain at least one variable modification i.e. force all reported peptides
to have a variable modifiation.
  - 0 = consider both modified and unmodified peptides (default)
  - 1 = analyze only peptides that contain a variable modification

Example:
```
require_variable_mod = 0
require_variable_mod = 1
```
