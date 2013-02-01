#include <stdio.h>
#include<time.h>
#include<stdlib.h>
main()
{
	int i,j;
	printf("the time(0) is %d\n",time(0));
	srand((int)time(0));
	for(i=0;i<10;i++)
	{
		j=1+(int)(10.0*rand()/(RAND_MAX+1.0));
		printf(" %d ",j);
	}
	printf("\n");
}
