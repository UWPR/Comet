### Notes 2019.11.04:  threading analysis

Quick analysis on Comet's threading performance

- The analysis was on a human target-decoy search of a Thermo QE file run on
two linux systems.
- Comet version 2019.01 rev. 1 using normal high-res search parameters.
- Searches using 1, 2, 4, 8, 12, 18, 24, 32, 48, or 64 search threads on a
64-core machine (AMD Opteron 6278 CPUs).
- Searches using 1, 2, 4, 8, 12, or 18 search threads on a 12-core (24
hyperthreads) machine (Intel Xeon X5690 CPU).  Only 11 cores were
available to the search process due to a quirk in job scheduler resource
allocation.
- In both instances, there's definitely a plateau in decreasing search times
around 12 to 18 search threads.

![image](/Comet/notes/20191104_threading.png)

