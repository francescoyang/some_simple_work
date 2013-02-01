#include <stdio.h>
//#include <time.h>
#include <sys/time.h>

static struct timeval newtimeof = {-1, -1};
static struct timeval oldtimeof = {-1, -1};

void main(int argc,char *argv[])
{
	int limit;
	limit = atoi(argv[1]);
	while(1)
	{
		gettimeofday(&newtimeof, NULL);
		if ((newtimeof.tv_sec * 1000 + newtimeof.tv_usec/1000)-(oldtimeof.tv_sec * 1000 + oldtimeof.tv_usec/1000) > limit * 1000) {
			printf("\n");
			printf("the time is up\n");
			printf("\n");
			oldtimeof.tv_sec= newtimeof.tv_sec;
			oldtimeof.tv_usec= newtimeof.tv_usec;
		}
		printf("not in time\n");
		sleep(1);
	}
}
