#include <stdio.h>
#include <time.h>
main()
{
time_t timep;
time (&timep);
printf("the system is %s",asctime(gmtime(&timep)));
}
