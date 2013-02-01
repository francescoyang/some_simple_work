#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include "lm3_api.h"


static int power_signal = 1;
static int i;
static int fd;

typedef struct __lm3cmd{
	char *argv;
	char *cmdp;
	char *reserv;
}lm3cmd_t;

enum lm3cmdval{
	OPEN_KEY,
	CONCTL_L1,
	CONCTL_L2,
	CONCTL_L3,
	CONCTL_BACKLIGHT,
	POWEROFF_PLAN,
	READ_VOLTAGE,
	POWEROFF_NOW,
	OUT_12,
	CMD_RESTART,
	LM3CMDMAX,
};

const lm3cmd_t lm3cmd_key[LM3CMDMAX] = {
{NULL, "-k", NULL},
{NULL,"-l1",NULL},
{NULL, "-l2", NULL},
{NULL, "-l3", NULL},
{NULL, "-b", NULL},
{NULL, "-p", NULL},
{NULL, "-g", NULL},
{NULL, "-c", NULL},
{NULL, "-o", NULL},
{NULL, "-re", NULL}
};



int read_voltage(void)
{
	return  lm3_main(fd,2,(char **)&lm3cmd_key[READ_VOLTAGE],1);
}

int out_12(int op)
{
	lm3_main(fd,2,(char **)&lm3cmd_key[OUT_12],op);
}
int operation_lm1(int op)
{
	lm3_main(fd,2,(char **)&lm3cmd_key[CONCTL_L1],op);
}
int operation_lm2(int op)
{
	struct timespec rem = { 0, 5000000};
	for(i=0;i < 4;i++) {
		if((lm3_main(fd,2,(char **)&lm3cmd_key[CONCTL_L2],op)) == 1) {
			return 0;
		}
		nanosleep(&rem, &rem);
	}
	return -1;
}

int operation_lm3(int op)
{
	struct timespec rem = { 0, 5000000};
	for(i=0;i < 4;i++) {
		if((lm3_main(fd,2,(char **)&lm3cmd_key[CONCTL_L3],op)) == 1) {
			return 0;
		}
		nanosleep(&rem, &rem);
	}
	return -1;
}

int operation_backlight(int op)
{
	struct timespec rem = { 0, 5000000};
	for(i=0;i < 4;i++) {
		if((lm3_main(fd,2,(char **)&lm3cmd_key[CONCTL_BACKLIGHT],op)) == 1) {
			return 0;
		}
		nanosleep(&rem, &rem);
	}
	return -1;
}

int restart_system(void)
{
	struct timespec rem = { 0, 500000000};
	for(i=0;i < 4;i++) {
		if((lm3_main(fd,2,(char **)&lm3cmd_key[CMD_RESTART],1)) == 1) {
			return 0;
		}
		nanosleep(&rem, &rem);
	}
	return -1;
}


int open_key(void)
{
	struct timespec rem = { 0, 500000000};
	for(i=0;i < 4;i++) {
		if((lm3_main(fd,2,(char **)&lm3cmd_key[OPEN_KEY],1)) == 1) {
			return 0;
		}
		nanosleep(&rem, &rem);
	}
	return -1;
}

int  test_poweroff_status(void)
{
//	printf("power_signal is %d\n",power_signal);
	if(!power_signal) {
		for(i=0;i<4;i++) {
			if((lm3_main(fd,2,(char **)&lm3cmd_key[POWEROFF_PLAN],0)) == 1)
					break;		
			struct timespec rem = { 0, 5000000};
			nanosleep(&rem, &rem);
		};
		printf("************* I will be turn off the light 2****************\n");
		operation_lm2(0);
		operation_lm3(0);
		operation_backlight(0);
		printf("****  you are into the poweroff function *********\n");
		return 1;
	} else {
		return 0 ;	
	}
	return 0;
}

void poweroff_now(void)															// power off	
{
	while(1){
		printf("*************** the system will be halt ******************\n");
		lm3_main(fd,2,(char **)&lm3cmd_key[POWEROFF_NOW],0);
		struct timespec rem = { 1, 0};
		nanosleep(&rem, &rem);
	}
}

int spi_modules_init(char  *devname)
{
	assert(devname);
	fd = open(devname,O_RDWR,0777);	
	if(fd < 0) {
		printf("open /dev/lm3_spi is error");
		return -1;
	}
	
	return 0;
}

