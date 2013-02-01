#include<stdio.h>
main()
{
	FILE * read_fp;
	FILE * write_fp;
	char s[80];
	int i;
	read_fp = fopen("./file","r");
	write_fp = fopen("./write_file","w");
	for(i = 0; i < 2; i++)
	fputs(fgets(s,80,read_fp),write_fp);
}
