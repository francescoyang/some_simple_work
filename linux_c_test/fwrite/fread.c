#include<stdio.h>
#define nmemb 3
struct test
{
	char name[20];
	int size;
}s[nmemb];

main()
{
	FILE * stream;
	int i;
	char *buf;
	stream = fopen( "./fread_file","r");
//	for( i = 0; i < 3; i++){
		fread(buf,5,1,stream);
		printf("printf %s\n",buf);
//	}
	fclose(stream);
}

