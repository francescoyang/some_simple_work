#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

main()
{
	int fd;
	void *start;
	struct stat sb;
	fd=open("/etc/passwd",O_RDONLY); /*��/etc/passwd*/
	fstat(fd,&sb); /*ȡ���ļ���С*/
	start=mmap(NULL,sb.st_size,PROT_READ,MAP_PRIVATE,fd,0);
	if(start == MAP_FAILED) /*�ж��Ƿ�ӳ��ɹ�*/
			return;
	printf("%s",start);
	munmap(start,sb.st_size); /*���ӳ��*/
	close(fd);
}
