#include<stdio.h>
#include<string.h>
#define nmemb 3


main()
{
	FILE * stream ;
	int i;
	char *buf[] = {0};

	stream=fopen("./fwrite_file","w");
	
	memcpy(buf,"1234",4);	

	fwrite(buf,4,1,stream);
	fclose(stream);
}
