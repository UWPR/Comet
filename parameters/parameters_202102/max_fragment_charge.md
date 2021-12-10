### Comet parameter: max_fragment_charge

- This parameter sets the maximum fragment charge state that will
be considered in the analysis.
- Typically, the fragment charge state range that is analyzed is
from 1+ to one charge less than the precursor charge state.
- For high precursor charge states (i.e. 6+), the default behavior
would analyze fragment ions with 1+ through 5+ charges on them.  This
parameter is a mechanism to limit the fragment charge range that is
analyzed.
- For example, if max_fragment_charge is set to 3 then the maximum
fragment charge state that will be analyzed is 3+.  However, the default
rule will still limit 1+ and 2+ precursor ions to only have 
1+ fragments considered.  And similarly 3+ precursors will still only
have 1+ and 2+ fragments considered.
- Valid values are any non-zero integer.
- The default value is "3" if this parameter is missing.  A maximum
allowed value of "5" is enforced for this parameter.

Example:
```
max_fragment_charge = 3
```
