#include <stdio.h>
#include <string.h>
main()
{
	char *a ="aBcDeF";
	char *b ="AbCdEf";
	char *c ="aacdef";
	char *d ="aBcDeF";
	printf("memcmp(a,b):%d\n",memcmp((void*)a,(void*) b,6));
	printf("memcmp(a,c):%d\n",memcmp((void*)a,(void*) c,6));
	printf("memcmp(a,d):%d\n",memcmp((void*)a,(void*) d,6));
}
