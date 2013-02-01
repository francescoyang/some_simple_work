#include <stdio.h>
#include <string.h>
#include<stdio.h>
#include<stdlib.h>
#define NMEMB 5
#define SIZE 10
int compar(const void *a,const void *b)
{
	return (strcmp((char *)a,(char *)b));
}
main()
{
	char data[50][SIZE]={"linux","freebsd","solaris","sunos","windows"};
	char key[80],*base ,*offset;
	int i, nmemb=NMEMB,size=SIZE;
	while(1){
		printf(">");
		fgets(key,sizeof(key),stdin);
		key[strlen(key)-1]='\0';
		if(!strcmp(key,"exit"))break;
		if(!strcmp(key,"list")){
			for(i=0;i<nmemb;i++)
				printf("%s\n",data[i]);
			continue;
		}
		base = data[0];
		qsort(base,nmemb,size,compar);
		offset = (char *) bsearch(key,base,nmemb,size,compar);
		if( offset == NULL){
			printf("%s not found!\n",key);
			strcpy(data[nmemb++],key);
			printf("Add %s to data array\n",key);
		}else{
			printf("found: %s \n",offset);
		}
	}
}
