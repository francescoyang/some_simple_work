#include <stdio.h>
#include <string.h>
#define nmemb 3
/*struct test
{
	char name[20];
	int size;
}s[nmemb];
*/
main()
{
	FILE * stream;
	char s[1024];
	int i;
	stream = fopen("./read_file","r");
	fread(s,sizeof(s),nmemb,stream);
	fclose(stream);
/*	for(i=0;i<nmemb;i++)
		printf("name[%d]=%-20s:size[%d]=%d\n",i,s[i].name,i,s[i].size);*/
//	bufline[strlen(bufline) - 1] = '\0';

	s[strlen(s) - 2] = '\0';
	printf("%s\n",s);
}


