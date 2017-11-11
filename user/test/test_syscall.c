#include <linux/unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <sys/types.h>
#include "stdio.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define __NR_sched_get_wrr_info 244
#define __NR_sched_set_wrr_weight 245
#define MAX_CPUS 8
struct wrr_info {
	
	int num_cpus;
	int nr_running[MAX_CPUS];
	int total_weight[MAX_CPUS];
};

int main(int argc, char const *argv[])
{

	struct wrr_info info;
	int cpu_num = syscall(__NR_sched_get_wrr_info, &info);
	int i;
	if (cpu_num < 0 ){
		perror("cpu_num<0");
	}
	printf("total cpu_num = %d\n", info.num_cpus);
	printf("# tasks  and total_weight on wrr_rq on each cpu:\n");
	for (i =0; i<info.num_cpus;++i){
		printf("# tasks: %d, total_weight: %d\n",info.nr_running[i], info.total_weight[i] );
	}


	return 0;
}
