### Comet parameter: retentiontime_tol

- This parameter specifies the +/- retention time tolerance, in seconds,
to apply for MS1 alignment.
- This parameter is only relevant for the real-time search interface and
is typically specified in the C# calling program.
- A value of "0" will not apply any tolerance which means each query MS1
scan will be analyzed against every reference run MS1 scan.

Example:
```
retentiontime_tol = 300    (for +/- 5 minutes)
```
