#include<stdio.h>
main()
{
	FILE * fp;
	int fd;
	fp=fopen("/etc/passwd","r");
	fd=fileno(fp);
	printf("fd=%d\n",fd);
	fclose(fp);
}
