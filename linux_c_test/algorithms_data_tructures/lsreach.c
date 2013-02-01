#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define NMEMB 50
#define SIZE 10
int compar (const void *a,const void *b)
{
	return (strcmp((char *) a, (char *) b));
}
main()
{
	int i, nmemb=NMEMB,size=SIZE;
	char data[NMEMB][SIZE]={"Linux","freebsd","solzris","sunos","windows"};
	char key[80],*base,*offset;
	while(1){
		fgets(key,sizeof(key),stdin);
		printf("stdio is %s",key);
		key[strlen(key)-1]='\0';
		base = data[0];
		offset = (char *)lfind(key,base,&nmemb,size,compar);
		if(offset ==NULL){
			printf("%s not found!\n",key);
			offset=(char *) lsearch(key,base,&nmemb,size,compar);
			printf("Add %s to data array\n",offset);
		}else{
			printf("fount \n");
			printf("found : %s \n",offset);

		}
	}
}
