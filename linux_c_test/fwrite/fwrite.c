#include<stdio.h>
#include<string.h>
#define nmemb 3


struct test
{
	char name[20];
	int size;
}s[nmemb];

set_s(int x,char *y) 
{
	strcpy(s[x].name,y);
	s[x].size=strlen(y);
}

main()
{
	FILE * stream ;
	int i;
	set_s(0,"Linux!\n");
	set_s(1,"FreeBSD!\n");
	set_s(2,"Windows2000.\n");

	stream=fopen("./fwrite_file","w");
	printf("s is %s\n",s);
	for( i = 0; i < 3; i++){
		fwrite(s[i].name,s[i].size,1,stream);
	}
	fclose(stream);
}
