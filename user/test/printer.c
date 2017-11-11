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

struct sched_param {
	int sched_priority;
};

void while_print(int sep, int x)
{
	int i;
	float j;

	while (1) {
		for (i = 0; i < sep; ++i)
			j = 2.333333 * 2.333333;
		if (x > 0)
			printf("%d finishes! ------------\n", x);
		else
			printf("%d finishes! +++ user!!!!\n", -x);
	}
}

int main(int argc, char const *argv[])
{
	if (argc != 4) {
		printf("error: 3 arguments (sep, num_root, num_user)\n");
		return 0;
	}
	int sep = atoi(argv[1]);
	int rnum = atoi(argv[2]);
	int unum = atoi(argv[3]);
	int i;
	int mask = 1;

	for (i = 1; i <= rnum; ++i) {
		pid_t pid = fork();

		if (pid == 0) {
			pid_t child = getpid();

			sched_setaffinity(child, sizeof(mask), &mask);
			while_print(sep, i);
			return 0;
		}
		mask = mask << 1;
		if (mask == (1 << 8))
			mask = 1;
	}
	setuid(10000);
	mask = 1;
	for (i = 1; i <= unum; ++i) {
		pid_t pid = fork();

		if (pid == 0) {
			pid_t child = getpid();

			sched_setaffinity(child, sizeof(mask), &mask);
			while_print(sep, -i);
			return 0;
		}
		mask = mask << 1;
		if (mask == (1 << 8))
			mask = 1;
	}
	for (i = 0; i < rnum + unum; ++i)
		wait(NULL);
	return 0;
}
