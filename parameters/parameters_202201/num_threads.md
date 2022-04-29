### Comet parameter: num_threads

- This parameter controls the number of processing threads that will be spawned for a search.
Ideally the number of threads is set to the same value as the number of CPU cores available.
- Valid values range for this parameter are numbers ranging from -64 to 64.
- A value of 0 will cause Comet to poll the system and launch the same number of threads
as CPU cores.
- To set an explicit thread count, enter any value between 1 and 64.
- A negative value will spawn the same number of threads as CPU cores less this negative
value.  For example, a parameter value of "-1" will launch 3 threads for a 4-core CPU
or 7 threads for an 8-core CPU.  If a large negative value is entered, equal to or
greater than the number of CPU cores, then 2 search threads will be spawned.
- The default value is "0" if this parameter is missing.
- If the environment variable NSLOTS is defined, the value of that environment variable
will override this parameter setting and will be the number of threads used in the search.
This environment variable is typically set in cluster/grid engine software environments.
- For an indexed database search, this parameter is ignored and only 1 thread is used in
those searches.

Example:
```
num_threads = 0
num_threads = 8
num_threads = -1
```
