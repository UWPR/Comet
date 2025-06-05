### Comet releases 2025.02

Documentation for parameters for release 2025.02 [can be found 
here](/Comet/parameters/parameters_202502/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2025.02 rev. 0 (2025.02.0), release date 2025/06/04

- Per request, implemented the "[min_precursor_charge](https://uwpr.github.io/Comet/parameters/parameters_202502/min_precursor_charge.html)" parameter.
This, along with the existing "[max_precursor_charge](https://uwpr.github.io/Comet/parameters/parameters_202502/max_precursor_charge.html)"
parameter, allows for searching a restricted precursor charge state range.  Feature requested by the UW Bruce lab.
- Changed allocations of temporary memory during threaded preprocessing… by @mhoopmann in https://github.com/UWPR/Comet/pull/78
- Update MSTookit in https://github.com/UWPR/Comet/pull/80
- Deprecated win32 support in Visual Studio.
