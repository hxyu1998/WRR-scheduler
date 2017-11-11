#include <linux/unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <sys/types.h>
#include "stdio.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include <fcntl.h>

#include <sys/stat.h>

#define __NR_get_wrr_info 244
#define __NR_SET_WRR_WEIGHT 245

#define MAX_CPUS 8


struct wrr_info {
	int num_cpus;
	int nr_running[MAX_CPUS];
	int total_weight[MAX_CPUS];
};

void daemon_mode(void)
{
	pid_t pid, sid;

	pid = fork();

	if (pid < 0) {
		printf("Fork failed\n");
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	umask(0);

	int flog =
	    open("./sched_log.txt", O_WRONLY | O_APPEND | O_CREAT);

	sid = setsid();
	if (sid < 0) {
		printf("Set sid failed\n");
		exit(EXIT_FAILURE);
	}

	if (chdir("/") < 0) {
		printf("Change directory failed\n");
		exit(EXIT_FAILURE);
	}

	dup2(flog, 1);
	close(0);
	close(2);
}


int main(int argc, char const *argv[])
{
	fprintf(stdout, "Start to work\n" );
	// daemon_mode();

	struct wrr_info data;
	int i = 0;

	fprintf(stdout, "Daemon process start to work\n" );

	while(1){
		syscall(__NR_get_wrr_info,&data);
		fprintf(stdout, "There are %d cpus\n", data.num_cpus);
		for(i = 0 ; i < data.num_cpus; i ++){
			fprintf(stdout, "%d cpu has %d tasks and total weight is %d\n",i,data.nr_running[i],data.total_weight[i] );
		}
		fprintf(stdout, "**********************************************\n");
		usleep(500000);
	}

	return 0;
}
