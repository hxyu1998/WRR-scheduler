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
			printf("###%d\n", sched_setaffinity(new, sizeof(mask), &mask));
			int j = 0;
			long long limit = 1000000000;
			for ( ; j < limit; ++j)
			{
				// if( j == 500000000){
				if( j == limit/2){
					mask = 15;
					printf("***%d\n", sched_setaffinity(new, sizeof(mask), &mask));
					sched_setaffinity(new,sizeof(mask),&mask);
				}
			}
			return 0;
		}
	}

	int ret = 0;
	while(1){
		wait(NULL);
		ret += 1;
		if(ret == 5)
			break;
	}

	return 0;
}
