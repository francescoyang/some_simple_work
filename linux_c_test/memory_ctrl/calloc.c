#include <stdio.h>
#include <string.h>
#include <stdlib.h>
struct test
{
	int a;
	char b[20];
}
main()
{
	int a;
	a = 1024;
	struct test *ptr=calloc(sizeof(struct test),10);
	ptr->a = a;
	memcpy(ptr->b,"acanoe is no 1",strlen("acanoe is no 1"));
	free(ptr);
	printf("calloc return %s\n",ptr->b);
	
}
