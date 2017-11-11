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

int main(int argc, char const *argv[])
{
	pid_t pid;
	int i = 0;
	for(; i < 6; i ++){
		pid = fork();
		if (pid == 0)
		{
			pid_t new = getpid();
			unsigned long mask = 1;
			sched_setaffinity(new, sizeof(mask), &mask);
			int j = 0;
			for ( ; j < 100000; ++j)
			{
				if( j == 5000){
					mask = 15;
					sched_setaffinity(new,sizeof(mask),&mask);
				}
			}
			return 0;
		}
	}

	int ret = 0;
	while(1){
		wait();
		ret += 1;
		if(ret == 5)
			break;
	}

	return 0;
}