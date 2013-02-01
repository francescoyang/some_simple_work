#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<string.h>
#include<netdb.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#define SERVPORT 3333
#define MAXDATASIZE 100 

typedef struct dateinfo{
		int leng;
		char  name[6];
		char sex[3];
		int age;
		char hobby[10];
}dateinfo_t;

int main(int argc,char *argv[])
{
	char *leng ;
	int sockfd,sendbytes;
	char buf[MAXDATASIZE];
	struct hostent *host;
	struct sockaddr_in serv_addr;
	struct dateinfo date;

	date.leng = sizeof(date);
	printf("date.leng is %d\n",date.leng);
	memcpy(date.name,"acanoe",strlen("acanoe"));
	memcpy(date.sex,"man",strlen("man"));
	date.age = 22;
	memcpy(date.hobby,"Linux c",strlen("Linux c"));
	
	memcpy(buf,&date,sizeof(dateinfo_t));
if(argc<2) {
		fprintf(stderr,"Please enter the server's hostmame!\n");
		exit(1);
	}

   /*	first 地址解析函数*/
	if((host=gethostbyname(argv[1])) == NULL){ perror("socket");
		exit(1);
	}

   /*	sceond 	设置socket*/
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))== -1) {
		perror("socket");
		exit(1);
	}

   /* 	thrid	设置sockaddr_in结构体中相关参数*/
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(SERVPORT);
	serv_addr.sin_addr=*((struct in_addr *)host->h_addr);
	bzero(&(serv_addr.sin_zero),8);

   /*	connect()	调用connetc函数主动发起对服务器端的链接*/
	if(connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr))== -1){
		printf("connect is error\n");
		perror("connect");
		exit(1);
	}

   /*发送消息给服务器端*/
	if((sendbytes=send(sockfd,&buf,sizeof(date),0))== -1) {
		perror("send");
		exit(1);
	}
	close(sockfd);
}
