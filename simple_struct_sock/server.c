#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>

#define SERVPORT 3333
#define BACKLOG 10
#define MAX_CONNECTED_NO 10
#define MAXDATASIZE 100 

typedef struct __blogsinfo{
		int leng;
		char  name[6];
		char sex[3];
		int age;
		char hobby[10];
}blogsinfo_t;
typedef struct __personinfo {
		int leng;
		char  name[6];
		char sex[3];
		int age;
		char hobby[10];
}personinfo_t;
personinfo_t personinfo;

int main()
{
	struct sockaddr_in server_sockaddr,client_sockaddr;
	socklen_t sin_size = 1;
	int recvbytes;
	int sockfd,client_fd;
	char buf[MAXDATASIZE];
	int leng;

   /*建立socket链接*/
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))== -1) {
		perror("socket");
		exit(1);
	}
	printf("socket success!,sockfd=%d\n",sockfd);

   /*设置socket_in结构体中相关参数*/
	server_sockaddr.sin_family=AF_INET;
	server_sockaddr.sin_port=htons(SERVPORT);
	server_sockaddr.sin_addr.s_addr=INADDR_ANY;
	bzero(&(server_sockaddr.sin_zero),8);

   /*绑定函数bind*/
	if(bind(sockfd,(struct sockaddr *)&server_sockaddr,sizeof(struct sockaddr)) == -1) {
		perror("bind");
		exit(1);
	}
	printf("bind success!\n");

   /*调用listen函数*/
	if(listen(sockfd,BACKLOG)== -1) {
		printf("listen  is error\n");
		perror("listen");
		exit(1);
	}
	printf("listening....\n");

   /*调用accept函数，等待客户端的链接*/
	memset(&client_sockaddr,0,sizeof(struct sockaddr_in));
	if((client_fd=accept(sockfd,(struct sockaddr *)&client_sockaddr,&sin_size)) == -1) {
		printf("accept  is error\n");
		perror("accept");
		exit(1);
	}

   /*调用recv函数接收客户端的请求*/
//	while(1)
//	{
		if((recvbytes=recv(client_fd,buf,1,0))== -1) {
			printf("recn is error\n");
			perror("recv");
			exit(1);
		}
		leng = (int )buf[0];
		printf("leng is %d\n",leng);

		if((recvbytes=recv(client_fd,buf+1,leng + 1,0))== -1) {
			printf("recn is error\n");
			perror("recv");
			exit(1);
		}
		blogsinfo_t *blogsinfo=(blogsinfo_t *)buf;
		
		personinfo.leng = 

		memcpy(personinfo.name, blogsinfo->name, 6);
		printf("personinfo_name is %s\n",personinfo.name);
	
		memcpy(personinfo.sex, blogsinfo->sex, 3);
		printf("personinfo_sex is %s\n",personinfo.sex);
	
		personinfo.age = blogsinfo->age;
		printf("personinfo_age is %d\n",personinfo.age);
	
		memcpy(personinfo.hobby, blogsinfo->hobby, 10);
		printf("personinfo_hobby is %s\n",personinfo.hobby);
//	}
	close(sockfd);
}

int sc_info_writefile(char *filename, char *data, int leng)
{
	int wolen = 0, walen = 0;
	FILE *fd = fopen(filename, "a+");
	if (!fd) {
		printf("can not open file:%s\n", filename);
		return -1;
	}
	while (walen < leng) {
		wolen = fwrite(data, 1, leng-walen,fd);
		if (wolen<0) {
			printf("write error!\n");
			fclose(fd);
			return -1;
		}
		walen+= wolen;
	}
	fclose(fd);

	return 0;
}
