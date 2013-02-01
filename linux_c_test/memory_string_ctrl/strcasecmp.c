#include <stdio.h>
#include <string.h>
main()
{
	char *a="aBcDeF";
	char *b="AbCdEf";
	if(!strcasecmp(a,b))
		printf("%s=%s\n",a,b);
}
