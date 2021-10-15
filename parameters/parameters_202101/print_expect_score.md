### Comet parameter: print_expect_score

- A boolean flag this determines whether or not the expectation score (E-value)
is reported in .out and SQT formats. Note that the E-value is always
reported in pepXML output.
- This parameter is only pertinant for results reported in .out and SQT
formats.
- If expect scores are chosen to be reported (i.e. value set to 1), they will
replace the number reported for the traditional "spscore" i.e. "spscore" will
be replaced by an E-value. Also an expectation value histogram will be output
at the end of each .out file; this histogram is not present for SQT output.
- Valid values are 0 and 1.
- The default value is 1 if this parameter is missing.

Example:
print_expect_score = 0
print_expect_score = 1
