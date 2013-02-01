#include <stdio.h>
#include <string.h>
main()
{
char s[30];
memset (s,'A',sizeof(s));
s[30]='\0';
printf("%s\n",s);
}
