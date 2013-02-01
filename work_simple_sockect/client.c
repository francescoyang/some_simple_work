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

int main(int argc,char *argv[])
{
	int sockfd,sendbytes;
	char buf[MAXDATASIZE];
	struct hostent *host;
	struct sockaddr_in serv_addr;
	if(argc<2)
	{
		fprintf(stderr,"Please enter the server's hostmame!\n");
		exit(1);
	}
   /*地址解析函数*/
	if((host=gethostbyname(argv[1])) == NULL){
		perror("socket");
		exit(1);
	}
   /*设置socket*/
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))== -1)
	{
		perror("socket");
		exit(1);
	}
   /*设置sockaddr_in结构体中相关参数*/
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(SERVPORT);
	serv_addr.sin_addr=*((struct in_addr *)host->h_addr);
	bzero(&(serv_addr.sin_zero),8);
   /*调用connetc函数主动发起对服务器端的链接*/
	if(connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr))== -1){
		printf("connect is error\n");
		perror("connect");
		exit(1);
	}
   /*发送消息给服务器端*/
	if((sendbytes=send(sockfd,"this is hello world test",5,0))== -1)
	{
		perror("send");
		exit(1);
	}
	close(sockfd);
}
