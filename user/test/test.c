#include <linux/unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <sys/types.h>
#include "stdio.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define __NR_sched_setscheduler 119
struct sched_param {
	int sched_priority;
};

int main(int argc, char const *argv[])
{

	pid_t p = getpid();
	int policy = 6;
	struct sched_param param;
	param.sched_priority = 101;


	syscall(__NR_sched_setscheduler,p,policy,&param);

	while(1);
	
	return 0;
}