#include <stdio.h>
#include <string.h>
main()
{
	char a[30]="string(1)";
	char b[]="string(2)";
	printf("before strcat() : %s\n",a);
	printf("after strcat()  : %s\n",strcat(a,b));
}
