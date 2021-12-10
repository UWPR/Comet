### Comet parameter: equal_I_and_L

- This parameter controls whether the Comet treats isoleucine (I) and leucine (L)
as the same/equivalent with respect to a peptide identification.
- For low-energy fragmentation, there's no way to distinguish between an I and L
in a spectrum.  Because of this, it doesn't make sense to assign a spectrum to peptide
DIGSTK but not DLGSTK.  With "equal_I_and_L = 1", Comet will treat these peptides as
the same identification and map proteins from either peptide to the output protein list.
- A user can change this behavior and treat a I residue as being different than an L
residue by setting "equal_I_and_L = 0".
- Valid values are 0, 1:
  - 0 treats I and L as different
  - 1 treats I and L as the same
- The default value is "1" if this parameter is missing.

Example:
```
equal_I_and_L = 0
```
