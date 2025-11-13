### Comet parameter: print_expect_score

- A boolean flag this determines whether or not the expectation
value (E-value) score is reported in the SQT output formats.
- This parameter is only pertinant for results reported in SQT formats,
both SQT file and SQT output stream.
- If the E-value scores are chosen to be reported (i.e. paramter value set to 1),
they will replace the number reported for the traditional "spscore", that is
"spscore" will be replaced by an E-value.
- Valid values are 0 and 1.
- The default value is "1" if this parameter is missing.

Example:
```
print_expect_score = 0
print_expect_score = 1
```
