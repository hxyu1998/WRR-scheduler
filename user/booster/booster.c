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
	printf("uid:%d\n", getuid());
	if (argc != 2)
		printf("error: arguments\n");
	int weight = atoi(argv[1]);
	if (syscall(245, weight) < 0)
		printf("error: system call\n");
	return 0;
}
