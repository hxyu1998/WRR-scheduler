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
	int i = 0;
	param.sched_priority = 0;


	syscall(__NR_sched_setscheduler,p,policy,&param);

	for(i = 0; i < 10 ; i ++){
		printf("%d\n", i);
	}

	return 0;
}
