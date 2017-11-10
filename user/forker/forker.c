#include <linux/unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <sys/types.h>
#include "stdio.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <linux/sched.h>


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
	int limit = 1;
	if (argc == 2)
		limit = atoi(argv[1]);

	param.sched_priority = 0;

	syscall(__NR_sched_setscheduler,p,policy,&param);
	
	for (i = 1; i < limit; ++i) {
		int pid = fork();
		if (pid == 0)
			while (1);
	}
	while (1);
	/*
	for(i = 0; i < 100000 ; i ++){
		printf("%d\n", i);
	}
	*/

	return 0;
}
