#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include <sys/syscall.h>
#include <linux/perf_event.h>

#include <sys/time.h>

#include <signal.h>

#define AMD_MSR_PWR_UNIT 0xC0010299
#define AMD_MSR_CORE_ENERGY 0xC001029A
#define AMD_MSR_PACKAGE_ENERGY 0xC001029B

#define AMD_TIME_UNIT_MASK 0xF0000
#define AMD_ENERGY_UNIT_MASK 0x1F00
#define AMD_POWER_UNIT_MASK 0xF
#define STRING_BUFFER 1024

#define MAX_CPUS	1024
#define MAX_PACKAGES	16

static int total_cores=0,total_packages=0;
static int package_map[MAX_PACKAGES];

const int DELAY_UNIT = 1000000; //microsecond 
const unsigned int NUM_NODES = 2;


void exitfunc(int sig)
{
    _exit(0);
}


double secondsSince(struct timeval *startTime) {
  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);
  return ((currentTime.tv_sec*1e6 + currentTime.tv_usec) - (startTime->tv_sec*1e6 + startTime->tv_usec)) / 1e6;
}

static int detect_packages(void) {

	char filename[BUFSIZ];
	FILE *fff;
	int package;
	int i;

	for(i=0;i<MAX_PACKAGES;i++) package_map[i]=-1;

	printf("\t");
	for(i=0;i<MAX_CPUS;i++) {
		sprintf(filename,"/sys/devices/system/cpu/cpu%d/topology/physical_package_id",i);
		fff=fopen(filename,"r");
		if (fff==NULL) break;
		fscanf(fff,"%d",&package);
		printf("%d (%d)",i,package);
		if (i%8==7) printf("\n\t"); else printf(", ");
		fclose(fff);

		if (package_map[package]==-1) {
			total_packages++;
			package_map[package]=i;
		}

	}

	printf("\n");

	total_cores=i;

	printf("\tDetected %d cores in %d packages\n\n",
		total_cores,total_packages);

	return 0;
}
//only one package inside.
//The following link explains what is cores, die and package.   
//https://superuser.com/questions/324284/what-is-meant-by-the-terms-cpu-core-die-and-package


static int open_msr(int core) {

	char msr_filename[BUFSIZ];
	int fd;

	sprintf(msr_filename, "/dev/cpu/%d/msr", core);
	fd = open(msr_filename, O_RDONLY);
	if ( fd < 0 ) {
		if ( errno == ENXIO ) {
			fprintf(stderr, "rdmsr: No CPU %d\n", core);
			exit(2);
		} else if ( errno == EIO ) {
			fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n",
					core);
			exit(3);
		} else {
			perror("rdmsr:open");
			fprintf(stderr,"Trying to open %s\n",msr_filename);
			exit(127);
		}
	}

	return fd;
}

static long long read_msr(int fd, unsigned int which) {

	uint64_t data;
	//
	if (pread(fd, &data, sizeof data, which) != sizeof data ) {
		perror("rdmsr:pread");
		exit(127);
	}
	//ssize_t pread(int fd, void *buf, size_t count, off_t offset);
	//pread() reads up to count bytes from file descriptor fd at offset offset 
	//(from the start of the file) into the buffer starting at buf. The file offset is not changed.
	//On success, the number of bytes read or written is returned (zero indicates that nothing was written, in the case of pwrite(), 
	//or end of file, in the case of pread()), or -1 on error, in which case errno is set to indicate the error.
	return (long long)data;
}


static int * read_stat() {

	FILE *filestream;
	char file_text[100];
	int *cpu_usage = (int*)malloc(sizeof(int) * 2);
    char delim[] = " ";
    
	filestream = fopen("/proc/stat", "r");
	if ( filestream < 0 ) {
		fprintf(stderr, "fail to open /proc/stat\n");
			exit(127);
	}

	fgets(file_text, 100, filestream);

    char *ptr = strtok(file_text, delim);
    int count = 0;
    
    while(ptr != NULL)
	{
        if (count >= 1) {
            cpu_usage[1] += atoi(ptr);
        }
        if (count >=1 && count <=3) {
            cpu_usage[0] += atoi(ptr);
        }
		
		ptr = strtok(NULL, delim);
        count++;
	}

    fclose(filestream);
	return cpu_usage;

}


//The map between cpu number to core_id on AMD Ryzen chip 
/*
cpuid     0 1 2 3 4 5 6 7 8 .. 15 16 .. 23 24 .. 30 31
core_id   0 1 2 3 4 5 6 7 0 .. 7  0  .. 7  0  .. 7  0     
*/

/*
check https://superuser.com/questions/324284/what-is-meant-by-the-terms-cpu-core-die-and-package
get more info about cpu core die and package 
*/


int main(int argc, char **argv) {
	
	signal(SIGALRM, exitfunc);
    alarm(atoi(argv[3]));

	struct timeval start;
	gettimeofday(&start, NULL);

	if (argc != 4){
		fprintf(stderr, "Usage: %s [sampling rate (HZ)] [output filename] [measuring time]\n", argv[0]);
	}
	unsigned int delay_us = DELAY_UNIT / atoi(argv[1]);
	char filename[512];
	snprintf(filename, 512, "CPU_%s.csv", argv[2]);
	FILE *outputFile = fopen(filename, "w");
	if (outputFile == NULL) {
		fprintf(stderr, "Unable to open output file.\n");
		return 1;
	}
	setbuf(outputFile, NULL);
	if(delay_us <= 0) {
		fprintf(stderr, "[CPU Meter]: Sampling rate must be a nonnegative integer.\n");
		return 1;
	}

	detect_packages();
	fprintf(outputFile, "Detected %d cores in %d packages\n\n",
		total_cores,total_packages);
	unsigned int time_unit, energy_unit, power_unit;
	double time_unit_d, energy_unit_d, power_unit_d;
	
	double *core_energy = (double*)malloc(sizeof(double)*total_cores/2);
	double *core_energy_delta = (double*)malloc(sizeof(double)*total_cores/2);

	double *package = (double*)malloc(sizeof(double)*total_cores/2);
	double *package_delta = (double*)malloc(sizeof(double)*total_cores/2);

	int *cpu_usage = (int*)malloc(sizeof(int) * 2);
	int *cpu_usage_delta = (int*)malloc(sizeof(int) * 2);
	double cpu_percent;
	
	//total_cores=32 malloc new double arrays that size are equals total_cores/2

	int *fd = (int*)malloc(sizeof(int)*total_cores/2);
	

	for (int i = 0; i < total_cores/2; i++) {
		fd[i] = open_msr(i);    
		// Upon successful completion, the function shall open the file and return a non-negative integer representing the lowest numbered unused file descriptor.
	}
	
	int core_energy_units = read_msr(fd[0], AMD_MSR_PWR_UNIT);
	fprintf(outputFile, "Core energy units: %x\n",core_energy_units);
	//%x	Unsigned hexadecimal integer
	//core_energy_unit is the variable that contain time, energy and power infomation 
	
	time_unit = (core_energy_units & AMD_TIME_UNIT_MASK) >> 16;
	energy_unit = (core_energy_units & AMD_ENERGY_UNIT_MASK) >> 8;
	power_unit = (core_energy_units & AMD_POWER_UNIT_MASK);
	fprintf(outputFile, "Time_unit:%d, Energy_unit: %d, Power_unit: %d\n", time_unit, energy_unit, power_unit);
	
	time_unit_d = pow(0.5,(double)(time_unit));
	energy_unit_d = pow(0.5,(double)(energy_unit));
	power_unit_d = pow(0.5,(double)(power_unit));
	fprintf(outputFile, "Time_unit:%g, Energy_unit: %g, Power_unit: %g\n", time_unit_d, energy_unit_d, power_unit_d);
	for(int i = 0; i < total_cores / 2; i++) fprintf(outputFile, "Core%d_Power(W), ",i);
	fprintf(outputFile, "Core_power_sum(W), CPU_power(W), CPU_usage(%%), Time(s)\n");
	while(1){
		int core_energy_raw = 0;
		int package_raw = 0;
		
		// Read per core energy values
		for (int i = 0; i < total_cores/2; i++) {
			core_energy_raw = read_msr(fd[i], AMD_MSR_CORE_ENERGY);
			package_raw = read_msr(fd[i], AMD_MSR_PACKAGE_ENERGY);

			core_energy[i] = core_energy_raw * energy_unit_d;
			package[i] = package_raw * energy_unit_d;
		}

		// Read whole cpu usage 
		cpu_usage = read_stat();

		usleep(delay_us);
		for (int i = 0; i < total_cores/2; i++) {
			core_energy_raw = read_msr(fd[i], AMD_MSR_CORE_ENERGY);
			package_raw = read_msr(fd[i], AMD_MSR_PACKAGE_ENERGY);

			core_energy_delta[i] = core_energy_raw * energy_unit_d;
			package_delta[i] = package_raw * energy_unit_d;
		}

		cpu_usage_delta = read_stat();

		double sum_core = 0;
		double avg_package = 0;
		for(int i = 0; i < total_cores/2; i++) {
			double diff_core = (core_energy_delta[i] - core_energy[i]) / (1.0 / atoi(argv[1]));
			double diff_package = (package_delta[i]- package[i]) / (1.0 / atoi(argv[1])); 
			sum_core += diff_core;
			avg_package += diff_package;
			//fprintf(outputFile, "Core %d, energy used: %gW, Package: %gW\n", i, diff_core, diff_package);
			fprintf(outputFile, "%g, ", diff_core);
		}

		cpu_percent = 100 * (cpu_usage_delta[0] - cpu_usage[0]) / (cpu_usage_delta[1] - cpu_usage[1]); 
		//fprintf(outputFile, "Core sum: %gW, CPU average power: %gW, CPU usage: %f %%, Time: %f\n", sum_core, avg_package/(total_cores/2), cpu_percent, secondsSince(&start));
		fprintf(outputFile, "%g, %g, %f, %f\n", sum_core, avg_package/(total_cores/2), cpu_percent, secondsSince(&start));
	}
	free(core_energy);
	free(core_energy_delta);
	free(package);
	free(package_delta);
	free(fd);
	free(cpu_usage);
	free(cpu_usage_delta);

	return 0;
}
