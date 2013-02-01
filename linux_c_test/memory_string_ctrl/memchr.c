#include <stdio.h>
#include <string.h>
main()
{
char *s="0123456789012345678901234567890";
char *p;
p=memchr(s,'5',10);
printf("%s\n",p);
}
