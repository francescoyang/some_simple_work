#include <string.h>
#include <stdio.h>

typedef struct scnetdatainfo{
	char name;
	char sex;
}scnetdatainfo_t;

typedef struct __resvdate{
	char name;
	char sex;
	char date[1024*30];
}resvdate_t;

void main(int argc,char *argv[])
{
	resvdate_t recvdate;
	printf("test one\n");
	memset(recvdate.date,0,1024*30);
	printf("strlen argv[1]  %d\n",strlen(argv[1]));
	memcpy(recvdate.date, argv[1], strlen(argv[1]));
	printf("test tow\n");
//	memcpy(recvdate->sex, argv[2], strlen(argv[2]));
	scnetdatainfo_t *scnetinfo = (scnetdatainfo_t *)recvdate.&date;
	printf("the scnetdateinfo_t name is %s\n",scnetinfo.name);
	printf("the scnetdateinfo_t sex  is %s\n",scnetinfo.sex);
}


