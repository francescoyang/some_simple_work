#include <stdio.h>
#include <string.h>
#include <unistd.h>
void main(int argc,char *argv[])
{
		char passwd[100];
		char *key;
		char slat[2];
		key= getpass("Input First Password:");
		slat[0]=key[0];
		slat[1]=key[1];
		strcpy(passwd,(char *)crypt(key,slat));
//		key=getpass("Input Second Password:");
//		slat[0]=passwd[0];
//		slat[1]=passwd[1];
		printf("After crypt(),1st passwd :%s\n",passwd);
//		printf("After crypt(),2nd passwd:%s\n",crypt(key,slat));
		key = argv[1];
		slat[0]=key[0];
		slat[1]=key[1];

		if( strcmp(passwd,(char *)crypt(key,slat)) == 0){
			printf("login in acanoe system#\n");	
		} else {
			printf("passwd is error\n");
		}
}
