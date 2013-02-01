#include<stdio.h>
main()
{
	FILE *fp;
	int c;
	fp=fopen("./file","r");
	while((c=fgetc(fp))!=EOF)
		printf("%c",c);
	fclose(fp);
}
