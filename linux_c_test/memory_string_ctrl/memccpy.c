#include <stdio.h>
#include <string.h>

void main()
{
	char a[]="string[a]";
	char b[]="string(b)";
	memccpy(a,b,'B',sizeof(b));
	printf("memccpy():%s\n",a);
}
