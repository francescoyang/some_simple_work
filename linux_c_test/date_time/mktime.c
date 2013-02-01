#include <stdio.h>
#include <time.h>
main()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	printf("time() : %d \n",timep);
	p=localtime(&timep);
	timep = mktime(p);
	printf("time()->localtime()->mktime():%d\n",timep);
}
