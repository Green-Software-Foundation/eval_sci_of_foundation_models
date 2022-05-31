# The gpu profiling tool can be used as follows

python gpuProfiling.py [sampling interval] [filename] [sampling time] 
  * e.g. python gpuProfiling.py 1 test 10 (sampling time is 1/sec, filename is test, sample for 10 seconds)

# Sudo permission is needed to run the cpu profiling tool (works for AMD Ryzen CPU)

1. run make command to generate the cpuLogToFile executable

2. sudo ./cpuLogToFile [sampling interval] [filename] [sampling time] 
  * e.g. sudo ./cpuLogToFile 1 test 10 (sampling time is 1/sec, filename is test, sample for 10 seconds)

# The following script can display the profiled data in a nice format

bash format.sh csv_file_name.csv