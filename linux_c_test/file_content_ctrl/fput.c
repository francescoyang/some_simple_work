#include <stdio.h>
#include <string.h>
main()
{
	FILE * fp;
	char a[26]="acanoe fput test";
	int i;
	fp= fopen("./fut_file_test","w");
	for(i=0;i<(strlen(a));i++)
		fputc(a[i],fp);
	fclose(fp);
}
