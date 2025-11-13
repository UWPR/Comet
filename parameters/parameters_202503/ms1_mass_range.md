### Comet parameter: ms1_mass_range

- Defines the mass range of MS1 peaks to analyze for MS1 retention time alignment.
- This parameter is only relevant for the real-time search interface and
is typically specified in the C# calling program.
- This parameter has two decimal values.
- The first value is the lower mass cutoff and the second value is
the high mass cutoff.
- Valid values are two decimal numbers where the first number must
be less or equal to the second number.

Example:
```
ms1_mass_range = 0.0 2000.0
```
