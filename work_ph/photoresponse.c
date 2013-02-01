#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
static int photoresponsefd = 0;
static struct timeval newtimeof = {-1, -1};
static struct timeval oldtimeof = {-1, -1};
static int light2_status = 0;

int photoresponse_modules_init(char *devname)
{
	assert(devname);
	photoresponsefd  = open(devname, O_RDWR);
	if (photoresponsefd <= 0) {
		perror("photoresponse open\n");
		return -1;
	}
	return 0;
}

//light2_status  0---off, 1--on
int photoresponse_readval(void)  // light2 is back ling status
{
	int photoresponse_back  = 120;
	int ret = 0;
	
		struct timespec rem = { 0, 200000000};
		nanosleep(&rem, &rem);
	ret = read(photoresponsefd, &photoresponse_back, 2);
	printf("the photorresponse_back = %d\n",photoresponse_back);
	if (ret < 0) {
		perror("photoresponse read ");
		return -1;
	}
	return photoresponse_back;
}
