# This program requires the py3nvml library
# pip install py3nvml

# Usage: python gpuProfiling.py [sampling interval] [filename] [sampling time] 
# e.g. python gpuProfiling.py 1 test 10  (sampling time is 1/sec, filename is test, sample for 10 seconds)

import py3nvml.nvidia_smi as nvml
import datetime
import time
import sys
import signal

def getGPUInfo():
    nvml.nvmlInit()
    num_gpus = nvml.nvmlDeviceGetCount()
    time_interval = 1 / float(sys.argv[1])
    filename = 'GPU_' + sys.argv[2] + '.csv'

    # 0 means buffer size is 0, auto flush buffer to the file 
    file1 = open(filename,"w", 1)

    file1.write('Number of GPUs {}\n'.format(num_gpus))
    file1.write('Time, device, gpu_util(%), gpu_mem(%), memory_used/total(Mib), power_current/limit(W), temperature(C), Frequency(MHz)\n')
    while True:
        for i in range(num_gpus):
            handle = nvml.nvmlDeviceGetHandleByIndex(i)
            util = nvml.nvmlDeviceGetUtilizationRates(handle)
            power = nvml.nvmlDeviceGetPowerUsage(handle)
            power *= 0.001
            power_limit = nvml.nvmlDeviceGetPowerManagementLimit(handle)
            power_limit *= 0.001
            memory_info = nvml.nvmlDeviceGetMemoryInfo(handle)
            temp = nvml.nvmlDeviceGetTemperature(handle, nvml.NVML_TEMPERATURE_GPU)
            clock = nvml.nvmlDeviceGetClockInfo(handle, 1)
            file1.write('{}, {}, {}, {}, {}/{}, {}/{}, {}, {}\n'.format(datetime.datetime.now().time(), i, util.gpu, util.memory, memory_info.used >> 20, memory_info.total >> 20, power, power_limit, temp, clock))

        time.sleep(time_interval)
    file1.close()


class TimeoutException(Exception):
    pass

def timeout_handler(signum, frame):
    raise TimeoutException


def main():

    if len(sys.argv) < 4:
        print('Usage: {} [Sampling interval (HZ)] [output filename] [measuring time (s)]'.format(sys.argv[0]))
        return 0

    TIMEOUT = int(sys.argv[3])

    signal.signal(signal.SIGALRM, timeout_handler)
    signal.alarm(TIMEOUT)

    try:
        print('GPU profiling started, time now: {}'.format(datetime.datetime.now()))
        getGPUInfo()
    except TimeoutException:
        print('GPU profiling terminated, duration: {}s, time now: {}'.format(TIMEOUT, datetime.datetime.now()))


if __name__ == '__main__':
    main()
