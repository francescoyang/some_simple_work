#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <curses.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

#include "devinitmod_api.h"
#include "jpnetsocket_api.h"
#include "jpuart_api.h"
#include "gps_api.h"
#include "lm3_api.h"

#include "person.h"
#include "bslProtocolCom.h"

#include "face_recognition.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char *jpsys_base;
extern char *jpsys_config_base;
extern char *jpsys_codec_base;
extern char *jpsys_data_base;
extern char *jpsys_module_base;
extern char *jpsys_content_base;
extern char *jpsys_tslib_base;


int power_signal = 1;
int back_light_signal = -1;
int person_id = 0;
int person_jpg_len[10] = {0};

static unsigned int training_seq_no = 0;
static int send_op_packet(argparam_t *ap, int cmd, int seqsk, int leng,  char *data);
static int recv_op_packet(argparam_t *ap, oppacketdata_t *rcvdate);

typedef struct scnetdatainfo{
	char name[16];
	char sex;
	char idtype;
	char idlengsign;
	char status;
	
	char current_curse;//1--kemu1   2---kemu2  3--kemu3
	char tech_level;//student--0   coach--1-5
	short train_cartype;//C1

	char icnumb[6];
	char rever[2];
	char idnumb[20];
	
	TCFDBCELL fgfeature;
	char facefeature[256];
		
	union privt{ 
		struct _stu{
			char plan_start_time[6];
			char plan_end_time[6];
			char pic[30*1024];
		}stu;
		struct coach{
			char zh_cartype[8];
			char coachid[20];
			char pic[30*1024];
		}coach;
	}priv;
}scnetdatainfo_t;


//pthread sync modify sync_val
void pthread_set_sync_val(argparam_t *argparam, enum pthread_nb pnb, enum cmdnumber cnb, int val)
{
	argparam_t *ap = argparam;
	assert(ap);
	
	pthread_mutex_lock(&(ap->sync_mutex));
	ap->sync_val[pnb][cnb] = val;
	pthread_mutex_unlock(&(ap->sync_mutex));

	return ;
}

//pthread read sync_val
int pthread_read_sync_val(argparam_t *argparam, enum pthread_nb pnb, enum cmdnumber cnb)
{
	argparam_t *ap = argparam;
	assert(ap);
	int val;

	pthread_mutex_lock(&(ap->sync_mutex));
	val = ap->sync_val[pnb][cnb];
	pthread_mutex_unlock(&(ap->sync_mutex));
	return val; 
}


const char *analysis_gpsdata(const char *str,char *dest )
{	
	//char buf[300];
	char *start, *end, *p;
	char buf[7];
	int num = 0;
	memset(buf, 0, 6);

	start = strchr(str, '$');
	end = strrchr(str, '$');
	if (!(start  && end)) {
		return NULL;
	}
	p = end;
	memcpy(dest, start, end - start);
	dest[end - start ] = '\0';

	memcpy(buf, p, 6);
	buf[6] = '\0';
	end = p;
//	printf("xxxxxxxxxxxxxxxxxxxxxxxx=%s\n",p);
	while(start = (strchr(end, ','))) {
		num++;
		end = start + 1;
	}
	if(strcmp(buf,"$GPRMC") == 0) {
		if(num < 12)
			return dest;
		strcat(dest,p);
		
	} else if(strcmp(buf,"$GPGSV") == 0) {
		if(num < 19)
			return dest;
		strcat(dest,p);

	} else if(strcmp(buf,"$GPGSA") == 0) {
		if(num < 17)
			return dest;
		strcat(dest,p);

	} else if(strcmp(buf,"$GPGLL") == 0) {
		if(num < 8)
			return dest;
		strcat(dest,p);

	} else if(strcmp(buf,"$GPGGA") == 0) {
		if(num < 14)
			return dest;
		strcat(dest,p);

	} else if(strcmp(buf,"$GPVTG") == 0) {
		if(num < 9)
			return dest;
		strcat(dest,p);

	} else {
		return dest;
	}
	
}




int photoresponse(int fd)
{
	short ph_date = 120;
	read(fd, &ph_date, 2);
			struct timespec rem = { 0, 500000000};
			nanosleep(&rem, &rem);
	printf("ph_date is %d\n",ph_date);
	//usleep(1000*500);
	return ph_date;
}
int set_power_off_args(int args)
{
	power_signal = args;
}

/*int set_key_back_light(int args)
{
	back_light_signal = args;
}*/
/*int read_key_back_light()
{
	if (back_light_signal == 1 )
	{
		back_light_signal = -1;
		return 1;
	}
	else if (back_light_signal == 0)
		return 0;
	else return -1;
}*/
int read_power_off_args()
{
	if (power_signal == 0)
	{	
		power_signal = 0;
		return 1;
	}
	else return 0;
}
void* gps_pthread(void *argparam)
{
	argparam_t *ap = argparam;
	assert(ap);
	char buf[300];
	int i = 0;
	struct timeval timeout;
	uartdev_t *dev = &ap->gpsmod.dev;
	GPSRMC gpsbuf;
	//GPSRMC gpsbuf;
	memset(&gpsbuf,0,sizeof(GPSRMC));
	memset(buf,0,300);
	
	pthread_mutex_init(&(ap->gpsmod.pmutex), NULL);
	pthread_cond_init(&(ap->gpsmod.pcond), NULL);
	ap->gpsmod.gpsp = (void*)jpuart_init(dev->name, dev->baudrate);
	if (!ap->gpsmod.gpsp) {
		printf("GPS mode init error!\n");
		goto gpserror;
	}
	//init gsp pthread envirenment val
	ap->gpsmod.cmd = CMDINVALID;	
	ap->gpsmod.sendleng = 0;
	ap->gpsmod.recvleng = 0;
	ap->gpsmod.sendp = malloc(GPS_SENDBUF_SIZE);
	ap->gpsmod.recvp = malloc(GPS_RECVBUF_SIZE);
	memset(ap->gpsmod.sendp, 0, GPS_SENDBUF_SIZE);
	memset(ap->gpsmod.recvp, 0, GPS_RECVBUF_SIZE);
	//tell main pthread sync 
	
	ap->gpsmod.recvleng = 300;
//	analysis_gpsdata(buf,ap->gpsmod.recvp);
//	printf("[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[\n");
	
//	printf("first output gps data : \n");//%s\n",ap->gpsmod.recvp);
//	getgpsinfo(ap, GPRMC, ap->gpsmod.recvp);
	
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	jpuart_recv_timeout(ap->gpsmod.gpsp, buf, ap->gpsmod.recvleng, &timeout);
	analysis_gpsdata(buf,ap->gpsmod.recvp);
//
//device send open msg to server
//
#if 0
	char sdate = 0;
	oppacketdata_t rcvdate;

	send_op_packet(ap, CMD_DEVOPEN_INFO, 0, 1,  sdate);
	recv_op_packet(ap, &rcvdate);

	//regist_deviceid_gpstime(ap);
#endif

#if 0
	
	int aaa = 0;
	if (ap->wifimod.enable) {
		while (1) {
			sleep(1);
			printf("aaaaaaaa=0x%x\n", aaa++);	
				download_datefrom_server(ap);
	
		}
	}
#endif
//wifi
	int wifitimeout=0;
	if (ap->wifimod.enable) {
		while (DEVINITOK_SYNCVAL!=pthread_read_sync_val(ap, WIFI_PTHREADNO, CMDDEVINIT)) {
			if (wifitimeout++ > DEVINIT_SYNC_TIMEOUT) {
				printf("wifidownload timeout!\n");
				break;
			}

			struct timespec rem = { 0, 100000};
			nanosleep(&rem, &rem);
			//usleep(100);
			continue;	
		}
		if (wifitimeout < DEVINIT_SYNC_TIMEOUT) {	
			printf("download data start!\n");
			download_datefrom_server(ap);
			printf("download data ok!\n");
		}
	}
	pthread_set_sync_val(ap, GPS_PTHREADNO, CMDDEVINIT, DEVINITOK_SYNCVAL);

	while ( 1 ) {
			ap->gpsmod.recvleng = 300;
			timeout.tv_sec = 2;
			timeout.tv_usec = 0;
			jpuart_recv_timeout(ap->gpsmod.gpsp, buf, ap->gpsmod.recvleng, &timeout);
			//printf("recv_timeout output gps data : %s\n",buf);
			//printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
#if 1
			if (!analysis_gpsdata(buf,ap->gpsmod.recvp)) {
				continue;
			}
			
//	puts(buf);
			getgpsinfo(ap,GPRMC,&gpsbuf);
/*	printf("chechout GPSRMC:%s\n%c\n%f\n%c\n%f\n%c\n%f\n%f\n%s\n%f\n%c\n%s\n", 
						gpsbuf.utctime, 
						gpsbuf.status, 
						gpsbuf.Latitude, 
						gpsbuf.Latitudesign, 
						gpsbuf.longitude, 
						gpsbuf.longitudesign, 
						gpsbuf.speed, 
						gpsbuf.direction, 
						gpsbuf.utcdate,
						gpsbuf.declination,
						gpsbuf.decdirection,
						gpsbuf.mode);

*/
			if(gpsbuf.status == 'A'){
				char timecmd[32];
				char buf_dd[2];
				char buf_m[2];
				char buf_yy[2];
				char buf_hh[2];
				char buf_mm[2];
				char buf_ss[2];
				memset(timecmd,0,sizeof(timecmd));
				sscanf(gpsbuf.utcdate,"%2s%2s%2s",buf_dd,buf_m,buf_yy);
				sscanf(gpsbuf.utctime,"%2s%2s%2s",buf_hh,buf_mm,buf_ss);
				int tmp_yy = atoi(buf_yy) + 1970;
				sprintf(timecmd,"date %s%s%s%s%d.%s",buf_m,buf_dd,buf_hh,buf_mm,tmp_yy,buf_ss);
				printf("timecmd = %s\n",timecmd);
				system(timecmd);
				system("hwclock -w");
			}
		
			if(ap->p_syslog.action){
				ap->p_syslog.action = (ap->p_syslog.action - 3)*2 + ap->p_syslog.identify;
				char logdata[12];
				memset(logdata,0,sizeof(logdata));
				oppacketdata_t logrecvdata;

				logdata[0] = 1;//current status of the car
				logdata[1] = training_seq_no++;
				memcpy(logdata+2,ap->p_syslog.iccard,sizeof(ap->p_syslog.iccard));
				if(ap->p_syslog.action == 3 || ap->p_syslog.action == 5){
					ap->p_syslog.trtime = htonl(ap->p_syslog.trtime);
					memcpy(logdata+8,&ap->p_syslog.trtime,sizeof(ap->p_syslog.trtime));
					printf("syslog: training time = 0x%X\n",ap->p_syslog.trtime);
				}

				printf("syslog: action = %d , now start to send.\n",ap->p_syslog.action);

				send_op_packet(ap,ap->p_syslog.action,0,sizeof(logdata),logdata);
				recv_op_packet(ap,&logrecvdata);
				if(!logrecvdata.leng){
					printf("=============syslog:receive nothing after sent============\n");
				}
				//write to file
				ap->p_syslog.iccard[5] = 0;
				fprintf(ap->p_syslog.fp,"%d %s %d %s %s\n",ap->p_syslog.action,ap->p_syslog.time,ap->p_syslog.identify,ap->p_syslog.name,ap->p_syslog.iccard);
				fflush(ap->p_syslog.fp);
				memset(&ap->p_syslog,0,sizeof(ap->p_syslog));
			}
/*			else{
				char statdata = 1;//current status of the car
				oppacketdata_t statrecvdata;

				send_op_packet(ap,0,0,1,&statdata);
				recv_op_packet(ap,&statrecvdata);
				if(!statrecvdata.leng){
					printf("=============statuslog:receive nothing after sent============\n");
				}
			}
*/		
#endif
			struct timespec rem = { 0, 100000000};
			nanosleep(&rem, &rem);
//	usleep(100000);
	}


gpserror:
	ap->gpsmod.enable = 0;
	pthread_set_sync_val(ap, GPS_PTHREADNO, CMDDEVINIT, INVALID_SYNCVAL);
	ap->gpsmod.cmd = CMDINVALID;	
	ap->gpsmod.sendleng = 0;
	ap->gpsmod.recvleng = 0;
	free(ap->gpsmod.sendp);
	free(ap->gpsmod.recvp);
	ap->gpsmod.sendp = NULL;
	ap->gpsmod.sendp = NULL;
	pthread_cond_destroy(&ap->gpsmod.pcond);
	pthread_mutex_destroy(&ap->gpsmod.pmutex);
	if (ap->gpsmod.gpsp) {
		jpuart_close(ap->gpsmod.gpsp);
		ap->gpsmod.gpsp = NULL;
	}
	pthread_exit(NULL);
}

	
void *wifi_pthread(void *argparam)
{
	argparam_t *ap = argparam;
	assert(ap);
	netdev_t *dev = &ap->wifimod.dev;
	struct timeval timeout;

	pthread_mutex_init(&(ap->wifimod.pmutex), NULL);
	pthread_cond_init(&(ap->wifimod.pcond), NULL);
	ap->wifimod.wifip = jpnetsocket_client_init(dev->sip, dev->sport, dev->sprotocol);
	if (!ap->wifimod.wifip) {
		printf("wifi error!\n");
		goto wifierror;
	}
	//init wifi pthread envirenment val	
	ap->wifimod.cmd = CMDINVALID;	
	ap->wifimod.sendleng = 0;
	ap->wifimod.recvleng = 0;
	ap->wifimod.sendp = malloc(WIFI_SENDBUF_SIZE);
	ap->wifimod.recvp = malloc(WIFI_RECVBUF_SIZE);
	memset(ap->wifimod.sendp, 0, WIFI_SENDBUF_SIZE);
	memset(ap->wifimod.recvp, 0, WIFI_RECVBUF_SIZE);
	
	pthread_set_sync_val(ap, WIFI_PTHREADNO, CMDDEVINIT, DEVINITOK_SYNCVAL);
	while ( 1 ) {
		printf("wifi pthread lock!\n");
		pthread_mutex_lock(&(ap->wifimod.pmutex));
		printf("wifi pthread set sendok!\n");
		pthread_set_sync_val(ap, WIFI_PTHREADNO, CMDSEND, SENDOK_SYNCVAL);
		printf("wifi pthread set recvok!\n");
		pthread_set_sync_val(ap, WIFI_PTHREADNO, CMDRECV, RECVOK_SYNCVAL);
		printf("wifi pthread waiting!\n");
		pthread_cond_wait(&(ap->wifimod.pcond), &(ap->wifimod.pmutex));
		pthread_mutex_unlock(&(ap->wifimod.pmutex));
		printf("wifi wakeup!\n");
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		switch(ap->wifimod.cmd) {
		case CMDSEND:
			jpnetsocket_send(ap->wifimod.wifip, ap->wifimod.sendp, 
								ap->wifimod.sendleng);
			break;
		case CMDRECV:
			if (jpnetsocket_recv_timeout(ap->wifimod.wifip, ap->wifimod.recvp, 
								ap->wifimod.recvleng, &timeout) < 0) {
								printf("recv error\n");
								 ap->wifimod.recvleng = -1;
								}
			printf ("recvdata: 0x%x, 0x%x\n", *((int *)(ap->wifimod.recvp)), *((int *)(ap->wifimod.recvp)+1));
			break;
		default:
			break;
		}
		continue;
		break;
	}

	jpnetsocket_close(ap->wifimod.wifip);
wifierror:
	ap->wifimod.enable= 0;
	pthread_set_sync_val(ap, WIFI_PTHREADNO, CMDDEVINIT, INVALID_SYNCVAL);
	ap->wifimod.cmd = CMDINVALID;	
	ap->wifimod.sendleng = 0;
	ap->wifimod.recvleng = 0;
	pthread_cond_destroy(&ap->wifimod.pcond);
	pthread_mutex_destroy(&ap->wifimod.pmutex);
	free(ap->wifimod.sendp);
	free(ap->wifimod.recvp);
	ap->wifimod.sendp = NULL;
	ap->wifimod.recvp = NULL;
	if (ap->wifimod.wifip) {
		jpnetsocket_close(ap->wifimod.wifip);
		ap->wifimod.wifip = NULL;
	}
	pthread_exit(NULL);
}
/*
void *server_pthread(void *argparam)
{
	argparam_t *ap = argparam;
	assert(ap);
	netdev_t *dev = &ap->servermod.dev;
	struct timeval timeout;

	ap->servermod.wifip = jpnetsocket_server_init(dev->sport, 5, dev->sprotocol);
	if (!ap->servermod.wifip) {
		printf("server error!\n");
		goto servererror;
	}
	//init wifi pthread envirenment val	
	ap->servermod.sendleng = 0;
	ap->servermod.recvleng = 1;
	ap->servermod.sendp = malloc(WIFI_SENDBUF_SIZE);
	ap->servermod.recvp = malloc(WIFI_RECVBUF_SIZE);
	memset(ap->servermod.sendp, 0, WIFI_SENDBUF_SIZE);
	memset(ap->servermod.recvp, 0, WIFI_RECVBUF_SIZE);

	while(1){
			jpnetsocket_accept(ap->servermod.wifip);
			while(1){
				if(jpnetsocket_recv(ap->servermod.wifip, ap->servermod.recvp, ap->servermod.recvleng) == -1) return -1;
				if(*(char *)ap->servermod.recvp != 0x7e) continue;
				if(jpnetsocket_recv(ap->servermod.wifip, ap->servermod.recvp+1, ap->servermod.recvleng) == -1) return -1;
				if(*(char *)(ap->servermod.recvp+1) != 0x7e) continue;
				if(jpnetsocket_recv(ap->servermod.wifip, ap->servermod.recvp+2, ap->servermod.recvleng) == -1) return -1;
				if(*(char *)(ap->servermod.recvp+2) != 0x7e) continue;
				if(jpnetsocket_recv(ap->servermod.wifip, ap->servermod.recvp+3, ap->servermod.recvleng) == -1) return -1;
				if(*(char *)(ap->servermod.recvp+3) != 0x7e) continue;
				break;
			}
			ap->servermod.recvleng = 16;
			if(jpnetsocket_recv(ap->servermod.wifip, ap->servermod.recvp, ap->servermod.recvleng) == -1) return -1;
			//
			if(jpnetsocket_recv(ap->servermod.wifip, ap->servermod.recvp, ap->servermod.recvleng) == -1) return -1;
			jpnetsocket_send(ap->servermod.wifip, ap->servermod.sendp, 
								ap->servermod.sendleng);
	}

	jpnetsocket_close(ap->servermod.serverp);
servererror:
	ap->servermod.enable= 0;
	ap->servermod.cmd = CMDINVALID;	
	ap->servermod.sendleng = 0;
	ap->servermod.recvleng = 0;
	free(ap->servermod.sendp);
	free(ap->servermod.recvp);
	ap->servermod.sendp = NULL;
	ap->servermod.recvp = NULL;
	if (ap->servermod.wifip) {
		jpnetsocket_close(ap->servermod.wifip);
		ap->servermod.wifip = NULL;
	}
	pthread_exit(NULL);
}
*/
#if 0 
argparam_t *alarmap;

void  alarm_func(int sign)
{
	jpnetsocket_shutdown((alarmap->facemod.facep), FREE_CLIENT);
	jpnetsocket_accept(alarmap->facemod.facep);
	alarm(120);
}


static int recv_exec_heartbeat(argparam_t *ap)
{
	int ret;
	struct timeval timeout;
	timeout.tv_sec = FACE_HEARTBEAT_TIME_SEC;
	timeout.tv_usec = FACE_HEARTBEAT_TIME_USEC;
	
	ap->facemod.recvleng =  sizeof(BSL_MSG_HEAD);
	ret = jpnetsocket_recv_timeout(ap->facemod.facep, ap->facemod.recvp, ap->facemod.recvleng, &timeout);
	if (ret > 0) {
		if(((BSL_MSG_HEAD *)ap->facemod.recvp)->msgType == BSL_MSGTYPE_HEADBEAT_REQ){
			memcpy(ap->facemod.sendp, ap->facemod.recvp, sizeof(BSL_MSG_HEAD));
			((BSL_MSG_HEAD *)(ap->facemod.sendp))->msgType = BSL_MSGTYPE_HEADBEAT_RES;
			ap->facemod.sendleng = sizeof(BSL_MSG_HEAD);
			jpnetsocket_send(ap->facemod.facep, ap->facemod.sendp, ap->facemod.sendleng);
			alarm(120);
		} else if(((BSL_MSG_HEAD *)ap->facemod.recvp)->msgType == BSL_MSGTYPE_ALARM_REPORT){
			//printf("alarm report \n");
		} else if(((BSL_MSG_HEAD *)ap->facemod.recvp)->msgType == BSL_MSGTYPE_EVENT_REPORT){
			//printf("event report \n");
		} else {
			ap->facemod.cmd = CMDRECV;
			pthread_set_sync_val(ap, FACE_PTHREADNO, CMDRECV, RECVOK_SYNCVAL);
		}
	} else if(ret==0) {
		//printf("timeout\n");
	} else {
		//printf("recv error and accept new!\n");
		alarm_func(SIGALRM);
	}
	return 0;
}



void *face_pthread(void *argparam)
{
	argparam_t *ap = argparam;
	assert(ap);

	netdev_t *dev = &ap->facemod.dev;	
	pthread_mutex_init(&ap->facemod.pmutex, NULL);
	pthread_cond_init(&ap->facemod.pcond, NULL);
	ap->facemod.facep = jpnetsocket_server_init(dev->sport, dev->slistens, dev->sprotocol);
	printf("face init sport:%d\n", dev->sport);
	if (!ap->facemod.facep) {
		printf("jpnetsocket_server_init face  pthread error!\n");
		goto faceerror;
	}
//init wifi pthread envirenment val	
	ap->facemod.cmd = CMDINVALID;	
	ap->facemod.sendleng = 0;
	ap->facemod.recvleng = 0;
	ap->facemod.sendp = malloc(FACE_SENDBUF_SIZE);
	ap->facemod.recvp = malloc(FACE_RECVBUF_SIZE);
	memset(ap->facemod.sendp, 0, FACE_SENDBUF_SIZE);
	memset(ap->facemod.recvp, 0, FACE_RECVBUF_SIZE);
	printf("face accept!\n");	
	jpnetsocket_accept(ap->facemod.facep);
	printf("face accept retujrn !\n");	
	alarmap = ap;
	signal(SIGALRM, alarm_func);
	alarm(120);

	pthread_set_sync_val(ap, FACE_PTHREADNO, CMDDEVINIT, DEVINITOK_SYNCVAL);
	
	while ( 1 ) {
		pthread_mutex_lock(&ap->facemod.pmutex);
		pthread_set_sync_val(ap, FACE_PTHREADNO, CMDSEND, SENDOK_SYNCVAL);
		pthread_set_sync_val(ap, FACE_PTHREADNO, CMDRECV, RECVOK_SYNCVAL);
		pthread_cond_wait(&ap->facemod.pcond, &ap->facemod.pmutex);
		pthread_mutex_unlock(&ap->facemod.pmutex);
		switch(ap->facemod.cmd) {
		case CMDSEND:
			if (pthread_read_sync_val(ap, FACE_PTHREADNO, CMDSEND) == SENDOK_SYNCVAL) {
				break;
			}
			
			jpnetsocket_send(ap->facemod.facep, ap->facemod.sendp, ap->facemod.sendleng);
			break;
		case CMDRECV:
			if (pthread_read_sync_val(ap, FACE_PTHREADNO, CMDRECV) == RECVOK_SYNCVAL) {
				break;
			}
			jpnetsocket_recv(ap->facemod.facep, ap->facemod.recvp, ap->facemod.recvleng);
			break;
		default:
			//recv heatbeet
			recv_exec_heartbeat(ap);
			break;
		}
		continue;
		break;
	}
//cleanup:
faceerror:
	ap->facemod.enable = 0;
	jpnetsocket_close(ap->facemod.facep);
	ap->facemod.facep = NULL;
	pthread_set_sync_val(ap, FACE_PTHREADNO, CMDDEVINIT, INVALID_SYNCVAL);
	ap->facemod.cmd = CMDINVALID;	
	ap->facemod.sendleng = 0;
	ap->facemod.recvleng = 0;
	pthread_cond_destroy(&ap->facemod.pcond);
	pthread_mutex_destroy(&ap->facemod.pmutex);
	free(ap->facemod.sendp);
	free(ap->facemod.recvp);
	ap->facemod.sendp = NULL;
	ap->facemod.recvp = NULL;
	pthread_exit(NULL);
}

#else

#define FACE_THREAD_DEBUG (1)
#define EXTENDED_DATA_LEN (25 * 1024)
#define RECEIVING_TIMEOUT_LEVEL (6)

void *face_pthread(void *argparam)
{
	char msg[BSL_MSG_HEAD_SIZE + EXTENDED_DATA_LEN];
	struct timeval timeout;
	BSL_MSG_HEAD *pmsgHead = (BSL_MSG_HEAD *) msg;
	char *pmsgBody = msg + BSL_MSG_HEAD_SIZE;
	argparam_t *ap = argparam;
	char timeout_cnt, flag_quit = 0, flag_init = 1;

	assert(ap);

	netdev_t *dev = &ap->facemod.dev;	
	pthread_mutex_init(&ap->facemod.pmutex, NULL);
	pthread_mutex_init(&ap->facemod.send_mutex, NULL);
	pthread_cond_init(&ap->facemod.pcond, NULL);
	ap->facemod.facep = jpnetsocket_server_init(dev->sport, dev->slistens, dev->sprotocol);
	if (!ap->facemod.facep) {
		fprintf(stderr, "jpnetsocket_server_init face  pthread error !\n");
		goto faceerror;
	}

	ap->facemod.cmd = CMDINVALID;	
	ap->facemod.sendleng = 0;
	ap->facemod.recvleng = 0;
	ap->facemod.sendp = malloc(FACE_SENDBUF_SIZE);
	ap->facemod.recvp = malloc(FACE_RECVBUF_SIZE);
	memset(ap->facemod.sendp, 0, FACE_SENDBUF_SIZE);
	memset(ap->facemod.recvp, 0, FACE_RECVBUF_SIZE);

accept:
	jpnetsocket_accept(ap->facemod.facep);
	fprintf(stdout, "Face device detected !\n");

	timeout_cnt = 0;
        ap->face_res = -1;

	if (flag_init == 1) {
		pthread_set_sync_val(ap, FACE_PTHREADNO, CMDDEVINIT, DEVINITOK_SYNCVAL);
		flag_init = 0;
	}

	while ( 1 ) {

		int ret;
		unsigned int mark, data_len;
		unsigned short msgType, ack;

		/* Is timeout ? */
		if (timeout_cnt >= RECEIVING_TIMEOUT_LEVEL) {
			fprintf(stderr, "Face device lost !\n");
			flag_quit = 0;
			break;
		}

		/* Receiving header */
		timeout.tv_sec = FACE_HEARTBEAT_TIME_SEC;
		timeout.tv_usec = FACE_HEARTBEAT_TIME_USEC;
		ret = jpnetsocket_recv_timeout(ap->facemod.facep, (char *)pmsgHead, BSL_MSG_HEAD_SIZE, &timeout);
		if (ret == 0) {
			fprintf(stderr, "Face device receiving timeout !\n");
                        ap->face_res = 0;
			timeout_cnt++;
			continue;
		} else if (ret < 0) {
			fprintf(stderr, "Face device receiving error !\n");
                        ap->face_res = 0;
			/* Consider error as timeout event */
			timeout_cnt++;
			continue;
		} else {
			timeout_cnt = 0;
		}

		if (face_head_parse(msg, &mark, &msgType, &ack, &data_len) < 0) {
			fprintf(stderr, "Invalid header\n");
			continue;
		}

#ifdef FACE_THREAD_DEBUG
		fprintf(stdout, "Msg header:\nmark = 0x%08x\nmsgType = 0x%04x\nack = 0x%04x\nlen=%d\n", mark, msgType, ack, data_len);
#endif

		/* Heart beat responds */
		if (msgType == BSL_MSGTYPE_HEADBEAT_REQ) {
#ifdef FACE_THREAD_DEBUG
			fprintf(stdout, "Face device heart Beats !\n");
#endif
			pmsgHead->msgType = BSL_MSGTYPE_HEADBEAT_RES;
			pthread_mutex_lock(&(ap->facemod.send_mutex));
			jpnetsocket_send(ap->facemod.facep, (char *)pmsgHead, (unsigned int)BSL_MSG_HEAD_SIZE);
			pthread_mutex_unlock(&(ap->facemod.send_mutex));
			continue;
		}

		/* Receiving real data */
		timeout.tv_sec = 60;
		timeout.tv_usec = 0;
		ret = jpnetsocket_recv_timeout(ap->facemod.facep, pmsgBody, data_len, &timeout);
		if (ret > 0) {

#ifdef FACE_THREAD_DEBUG
			fprintf(stdout, "Face device data received : %d\n", ret);
#endif

			int i;

                        // ap->face_res = -1;

			if (msgType == BSL_MSGTYPE_EVENT_REPORT) {
				int event_type;
				event_type = face_event_parse(msg);

			        // pthread_mutex_lock(&(ap->facemod.pmutex));
                                if (event_type == 0) {
                                    ap->face_res = 2;
                                } else if (event_type == 1) {
                                    ap->face_res = 3;
                                } else if (event_type == 2) {
                                    ap->face_res = 4;
                                } else if (event_type == 3) {
                                    ap->face_res = 5;
                                }
			        //pthread_mutex_unlock(&(ap->facemod.pmutex));
#ifdef FACE_THREAD_DEBUG
				fprintf(stdout, "Face event reported from device: %d\n", event_type);
#endif
			} else if (msgType == BSL_MSGTYPE_ALARM_REPORT) {
				BSL_ALARM_REPORT alarm;
				face_alarm_parse(msg, &alarm);
#ifdef FACE_THREAD_DEBUG
				fprintf(stdout, "Face alarm reported form device: type = %d, level = %d, jpg = %d, channel = %d\n", 
					alarm.alarmType, alarm.alarmLevel, alarm.jpgLen, alarm.channel);
#endif
			} 
			else {
				if (msgType == BSL_MSGTYPE_DVS_STATE_RES) {
					BSL_DVS_INFO state;
					face_info_respond(msg, &state);
#ifdef FACE_THREAD_DEBUG
					fprintf(stdout, "Face info reported form device: vid0 = %d, vid1 = %d, recState = %d, smartState = %d\n",
						state.videoInput0, state.videoInput1,
						state.recordState, state.smartState );
#endif
				}

				if (msgType == BSL_MSGTYPE_IDENTITY_TEMPLATE_CAPTURE_RES) {
					char id[32];
					int jpg_len;

                                        jpg_len = 0;
					face_capture_respond(msg, id, &jpg_len);
#ifdef FACE_THREAD_DEBUG
					fprintf(stdout, "Face capture reported form device:\n");
					for (i = 0;i < 32;i++) {
						fprintf(stdout, "id[%d] = 0x%02x ", i, id[i]);
						if (!((i + 1) % 8))
							fprintf(stdout, "\n");
					}
					fprintf(stdout, "jpg = %d\n", jpg_len);
#endif

			                //pthread_mutex_lock(&(ap->facemod.pmutex));
                                        if (jpg_len > 0) {
                                            ap->face_res = 1;
                                            person_jpg_len[person_id] = jpg_len;
                                            if (person_id <= 9)
                                               person_id++;
                                            else
                                               person_id = 0;
                                        } else {
                                            ap->face_res = 0;
                                        }
			               // pthread_mutex_unlock(&(ap->facemod.pmutex));
				}

				if (msgType == BSL_MSGTYPE_IDENTITY_RECOGNITE_RES) {
					char id[32];
					int result, jpg_len, i;

                                        jpg_len = 0;
					face_recognition_respond(msg, id, &result, &jpg_len);
#ifdef FACE_THREAD_DEBUG
					fprintf(stdout, "Face recognition result reported form device:\n");
					fprintf(stdout, "Result = %d\n", result);
					for (i = 0;i < 32;i++) {
						fprintf(stdout, "id[%d] = 0x%02x ", i, id[i]);
						if (!((i + 1) % 8))
							fprintf(stdout, "\n");
					}
					fprintf(stdout, "jpg = %d\n", jpg_len);
#endif

			                //pthread_mutex_lock(&(ap->facemod.pmutex));
                                        if (jpg_len > 0) {
                                            for (i = 0;i < 10;i++) {
                                                if (jpg_len == person_jpg_len[i]) {
					           fprintf(stdout, "jpg = %d, person_jpg_len = %d\n", jpg_len, person_jpg_len[i]);
                                                   ap->face_res = 1;
                                                   break;
                                                }
                                            }
                                            if (i < 10)
                                                ap->face_res = 1;
                                            else 
                                                ap->face_res = 0;
                                        } else {
                                            ap->face_res = 0;
                                        }
			                //pthread_mutex_unlock(&(ap->facemod.pmutex));
				}

				// memcpy(ap->facemod.recvp, msg, sizeof(msg));
				// pthread_set_sync_val(ap, FACE_PTHREADNO, CMDRECV, RECVOK_SYNCVAL);
			}
		}
	}

	if (!flag_quit) {
		goto accept;
	}

/* cleanup */
faceerror:
	ap->facemod.enable = 0;
	jpnetsocket_close(ap->facemod.facep);
	ap->facemod.facep = NULL;
	pthread_set_sync_val(ap, FACE_PTHREADNO, CMDDEVINIT, INVALID_SYNCVAL);
	ap->facemod.cmd = CMDINVALID;	
	ap->facemod.sendleng = 0;
	ap->facemod.recvleng = 0;
	pthread_cond_destroy(&ap->facemod.pcond);
	pthread_mutex_destroy(&ap->facemod.pmutex);
	free(ap->facemod.sendp);
	free(ap->facemod.recvp);
	ap->facemod.sendp = NULL;
	ap->facemod.recvp = NULL;
	pthread_exit(NULL);
}

#endif

void *face_pthread_udp(void *argparam)
{
	printf("face udp pthread creat...\n");

	pthread_exit(NULL);

}

int init_iccardmod(char *devname);
unsigned char THM_MFindCard(unsigned char * b_uid);

void *jpcard_pthread(void *argparam)
{
	argparam_t *ap = argparam;
	assert(ap);
	uartdev_t *dev = &ap->iccardmod.dev;
	struct timeval timeout;

	pthread_mutex_init(&(ap->iccardmod.pmutex), NULL);
	pthread_cond_init(&(ap->iccardmod.pcond), NULL);
//	ap->iccardmod.iccardp = (void *)jpuart_init(dev->name, dev->baudrate);
	//if (!ap->iccardmod.iccardp) {
	//	printf("idcard pthread jpuart_init error!\n");
	//	goto idcarderror;
//	}
//init idcard pthread envirenment val	
	ap->iccardmod.cmd = CMDINVALID;	
	ap->iccardmod.sendleng = 0;
	ap->iccardmod.recvleng = 0;
	ap->iccardmod.sendp = malloc(ICCARD_SENDBUF_SIZE);
	ap->iccardmod.recvp = malloc(ICCARD_RECVBUF_SIZE);
	memset(ap->iccardmod.sendp, 0, ICCARD_SENDBUF_SIZE);
	memset(ap->iccardmod.recvp, 0, ICCARD_RECVBUF_SIZE);
	
	if (init_iccardmod(dev->name)) {
		printf("icmod init error:");	
	}
	pthread_set_sync_val(ap, ICCARD_PTHREADNO, CMDDEVINIT, DEVINITOK_SYNCVAL);
	
	while ( 1 ) {
		pthread_mutex_lock(&(ap->iccardmod.pmutex));
		pthread_set_sync_val(ap, ICCARD_PTHREADNO, CMDRECV, RECVOK_SYNCVAL);
		pthread_set_sync_val(ap, ICCARD_PTHREADNO, CMDSEND, SENDOK_SYNCVAL);
		pthread_cond_wait(&(ap->iccardmod.pcond), &(ap->iccardmod.pmutex));
		pthread_mutex_unlock(&(ap->iccardmod.pmutex));
	
		int times = 0;
		while (THM_MFindCard(ap->iccardmod.recvp)) {
			struct timespec rem = { 0, 100000};
			nanosleep(&rem, &rem);
			//usleep(100);	
			if (times++ > 50000) {
				break;
			}
		}
	}
iccarderror:
	jpuart_close(ap->iccardmod.iccardp);
	ap->iccardmod.iccardp = NULL;
	ap->iccardmod.enable = 0;
	pthread_set_sync_val(ap, IDCARD_PTHREADNO, CMDRECV, INVALID_SYNCVAL);
	ap->iccardmod.cmd = CMDINVALID;	
	ap->iccardmod.sendleng = 0;
	ap->iccardmod.recvleng = 0;
	pthread_cond_destroy(&(ap->iccardmod.pcond));
	pthread_mutex_destroy(&(ap->iccardmod.pmutex));
	free(ap->iccardmod.sendp);
	free(ap->iccardmod.recvp);
	ap->iccardmod.sendp = NULL;
	ap->iccardmod.recvp = NULL;
	pthread_exit(NULL);
}


void *idcard_pthread(void *argparam)
{
	argparam_t *ap = argparam;
	assert(ap);
	uartdev_t *dev = &ap->idcardmod.dev;
	struct timeval timeout;

	pthread_mutex_init(&(ap->idcardmod.pmutex), NULL);
	pthread_cond_init(&(ap->idcardmod.pcond), NULL);
//	ap->idcardmod.idcardp = (void *)jpuart_init(dev->name, dev->baudrate);
	//if (!ap->idcardmod.idcardp) {
	//	printf("idcard pthread jpuart_init error!\n");
	//	goto idcarderror;
//	}
//init idcard pthread envirenment val	
	ap->idcardmod.cmd = CMDINVALID;	
	ap->idcardmod.sendleng = 0;
	ap->idcardmod.recvleng = 0;
	ap->idcardmod.sendp = malloc(IDCARD_SENDBUF_SIZE);
	ap->idcardmod.recvp = malloc(IDCARD_RECVBUF_SIZE);
	memset(ap->idcardmod.sendp, 0, IDCARD_SENDBUF_SIZE);
	memset(ap->idcardmod.recvp, 0, IDCARD_RECVBUF_SIZE);
	
	if (init_iccardmod(dev->name)) {
		printf("icmod init error:");	
	}
	pthread_set_sync_val(ap, IDCARD_PTHREADNO, CMDDEVINIT, DEVINITOK_SYNCVAL);
	
	while ( 1 ) {
		pthread_mutex_lock(&(ap->idcardmod.pmutex));
		pthread_set_sync_val(ap, IDCARD_PTHREADNO, CMDRECV, RECVOK_SYNCVAL);
		pthread_set_sync_val(ap, IDCARD_PTHREADNO, CMDSEND, SENDOK_SYNCVAL);
		pthread_cond_wait(&(ap->idcardmod.pcond), &(ap->idcardmod.pmutex));
		pthread_mutex_unlock(&(ap->idcardmod.pmutex));

		memset(ap->idcardmod.recvp,0,sizeof(ap->idcardmod.recvp));
		int times = 0;
		while (THM_MFindCard(ap->idcardmod.recvp)) {
			struct timespec rem = { 0, 100000};
			nanosleep(&rem, &rem);
			//usleep(100);	
			if (times++ > 20) {
				break;
			}
		}
		continue;
		printf("========================horrible!=======================\n");
	/*
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		switch(ap->idcardmod.cmd) {
		case CMDSEND:
			jpuart_send(ap->idcardmod.idcardp, ap->idcardmod.sendp, ap->idcardmod.sendleng);
			break;
		case CMDRECV:
			jpuart_recv_timeout(ap->idcardmod.idcardp, ap->idcardmod.recvp, ap->idcardmod.recvleng, &timeout);
			break;
		default:
			break;
		}
		continue;
		break;
		*/
	}

idcarderror:
	jpuart_close(ap->idcardmod.idcardp);
	ap->idcardmod.idcardp = NULL;
	ap->idcardmod.enable = 0;
	pthread_set_sync_val(ap, IDCARD_PTHREADNO, CMDRECV, INVALID_SYNCVAL);
	ap->idcardmod.cmd = CMDINVALID;	
	ap->idcardmod.sendleng = 0;
	ap->idcardmod.recvleng = 0;
	pthread_cond_destroy(&(ap->idcardmod.pcond));
	pthread_mutex_destroy(&(ap->idcardmod.pmutex));
	free(ap->idcardmod.sendp);
	free(ap->idcardmod.recvp);
	ap->idcardmod.sendp = NULL;
	ap->idcardmod.recvp = NULL;
	pthread_exit(NULL);
}



void *wlan3g_pthread(void *argparam)
{
	argparam_t *ap = argparam;
	assert(ap);
	netdev_t *dev = &ap->wlan3gmod.dev;

	pthread_mutex_init(&ap->wlan3gmod.pmutex, NULL);
	pthread_cond_init(&ap->wlan3gmod.pcond, NULL);
	ap->wlan3gmod.wlan3gp = jpnetsocket_client_init(dev->sip, dev->sport, dev->sprotocol);
	if (!ap->wlan3gmod.wlan3gp) {
		printf("wlan3g jpnetsocket client init error!\n");
		goto wlan3gerror;
	}
//init idcard pthread envirenment val	
	ap->wlan3gmod.cmd = CMDINVALID;	
	ap->wlan3gmod.sendleng = 0;
	ap->wlan3gmod.recvleng = 0;
	ap->wlan3gmod.sendp = malloc(WLAN3G_SENDBUF_SIZE);
	ap->wlan3gmod.recvp = malloc(WLAN3G_RECVBUF_SIZE);
	memset(ap->wlan3gmod.sendp, 0, WLAN3G_SENDBUF_SIZE);
	memset(ap->wlan3gmod.recvp, 0, WLAN3G_RECVBUF_SIZE);
	
	pthread_set_sync_val(ap, WLAN3G_PTHREADNO, CMDDEVINIT, DEVINITOK_SYNCVAL);
	//printf("pthread_cond_signal in WLAN3G\n");
	while ( 1 ) {
		pthread_mutex_lock(&ap->wlan3gmod.pmutex);
		pthread_set_sync_val(ap, WLAN3G_PTHREADNO, CMDSEND, SENDOK_SYNCVAL);
		pthread_set_sync_val(ap, WLAN3G_PTHREADNO, CMDRECV, RECVOK_SYNCVAL);
		pthread_cond_wait(&ap->wlan3gmod.pcond, &(ap->wlan3gmod.pmutex));
		pthread_mutex_unlock(&ap->wlan3gmod.pmutex);

		switch(ap->wlan3gmod.cmd) {
		case CMDSEND:
			jpnetsocket_send(ap->wlan3gmod.wlan3gp, ap->wlan3gmod.sendp, ap->wlan3gmod.sendleng);
			break;
		case CMDRECV:
			jpnetsocket_recv(ap->wlan3gmod.wlan3gp, ap->wlan3gmod.recvp, ap->wlan3gmod.recvleng);
			break;
		default:
			break;
		}
		continue;
		break;
	}

wlan3gerror:
	jpnetsocket_close(ap->wlan3gmod.wlan3gp);
	ap->wlan3gmod.wlan3gp = NULL;
	ap->wlan3gmod.enable = 0;
	ap->wlan3gmod.cmd = CMDINVALID;	
	ap->wlan3gmod.sendleng = 0;
	ap->wlan3gmod.recvleng = 0;
	pthread_cond_destroy(&ap->wlan3gmod.pcond);
	pthread_mutex_destroy(&ap->wlan3gmod.pmutex);
	free(ap->wlan3gmod.sendp);
	free(ap->wlan3gmod.recvp);
	ap->wlan3gmod.sendp = NULL;
	ap->wlan3gmod.recvp = NULL;
	pthread_exit(NULL);
}


void *finger_pthread(void *argparam)
{
	argparam_t *ap = argparam;
	assert(ap);
	uartdev_t *dev = &ap->fingermod.dev;
	struct timeval timeout;

	pthread_mutex_init(&(ap->fingermod.pmutex), NULL);
	pthread_cond_init(&(ap->fingermod.pcond), NULL);
	ap->fingermod.fingerp = (void *)jpuart_init(dev->name, dev->baudrate);
	if (!ap->fingermod.fingerp) {
		printf("jpuart_init fingermod error!\n");
		pthread_cond_destroy(&ap->fingermod.pcond);
		pthread_mutex_destroy(&ap->fingermod.pmutex);
		pthread_exit(NULL);
	}
//init idcard pthread envirenment val	
	ap->fingermod.cmd = CMDINVALID;	
	ap->fingermod.sendleng = 0;
	ap->fingermod.recvleng = 0;
	ap->fingermod.sendp = malloc(FINGER_SENDBUF_SIZE);
	if (!ap->fingermod.sendp) {
		printf("malloc error!\n");
		jpuart_close(ap->fingermod.fingerp);
		pthread_cond_destroy(&(ap->fingermod.pcond));
		pthread_mutex_destroy(&(ap->fingermod.pmutex));
		pthread_exit(NULL);
	}
	ap->fingermod.recvp = malloc(FINGER_RECVBUF_SIZE);
	if (!ap->fingermod.recvp) {
		printf("fingermod malloc error!\n");	
		pthread_cond_destroy(&(ap->fingermod.pcond));
		pthread_mutex_destroy(&(ap->fingermod.pmutex));
		jpuart_close(ap->fingermod.fingerp);
		free(ap->fingermod.sendp);
		ap->fingermod.sendp = NULL;
		pthread_exit(NULL);
	}
	memset(ap->fingermod.sendp, 0, FINGER_SENDBUF_SIZE);
	memset(ap->fingermod.recvp, 0, FINGER_RECVBUF_SIZE);
	
	pthread_set_sync_val(ap, FINGER_PTHREADNO, CMDDEVINIT, DEVINITOK_SYNCVAL);
	
	while ( 1 ) {
		pthread_mutex_lock(&(ap->fingermod.pmutex));
		pthread_set_sync_val(ap, FINGER_PTHREADNO, CMDSEND, SENDOK_SYNCVAL);
		pthread_set_sync_val(ap, FINGER_PTHREADNO, CMDRECV, RECVOK_SYNCVAL);
		pthread_cond_wait(&(ap->fingermod.pcond), &(ap->fingermod.pmutex));
		pthread_mutex_unlock(&ap->fingermod.pmutex);
		
		
		printf("finger wakeup\n");	
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		switch (ap->fingermod.cmd) {
		case CMDSEND:
			jpuart_send(ap->fingermod.fingerp, ap->fingermod.sendp, ap->fingermod.sendleng);
			break;
		case CMDRECV:
			jpuart_recv_timeout(ap->fingermod.fingerp, ap->fingermod.recvp, ap->fingermod.recvleng, &timeout);
			break;
		default:
			break;
		}
		continue;
	}
	
	jpuart_close(ap->fingermod.fingerp);
	pthread_set_sync_val(ap, FINGER_PTHREADNO, CMDRECV, INVALID_SYNCVAL);
	ap->fingermod.cmd = CMDINVALID;	
	ap->fingermod.sendleng = 0;
	ap->fingermod.recvleng = 0;
	pthread_cond_destroy(&(ap->fingermod.pcond));
	pthread_mutex_destroy(&(ap->fingermod.pmutex));
	jpuart_close(ap->fingermod.fingerp);
	free(ap->fingermod.sendp);
	free(ap->fingermod.recvp);
	ap->fingermod.sendp = NULL;
	ap->fingermod.recvp = NULL;
	pthread_exit(NULL);
}

void *voice_pthread(void *argparam)
{

#if 0
#endif
	pthread_exit(NULL);
}

void *video0_pthread(void *argparam)
{
#if 0
#endif
	pthread_exit(NULL);
}


void *video1_pthread(void *argparam)
{
#if 0
#endif
	pthread_exit(NULL);
}




void *video2_pthread(void *argparam)
{
#if 0
#endif
	pthread_exit(NULL);
}


void *video3_pthread(void *argparam)
{
#if 0
#endif
	pthread_exit(NULL);
}


static unsigned  char checksum(char *data, int len)
{
	unsigned short sum = 0;
	int i;

	for (i = 0; i < len; i++) {
		sum ^= data[i];
	}
	sum = ~sum;

	return ((sum + 1) & 0xff);
}

void* manage_pthread(void *argparam)
{
	int photoresponse_back ;
	int fd, fdph;
	int i;
	int light2_status = 0;
	int lm3_return;
	struct timeval oldtimeof;
	struct timeval newtimeof;
	char *lm3cmd_key[]={NULL,"-k",NULL};
	char *lm3cmd_l2[]={NULL,"-l2",NULL};
//	char *lm3cmd_l3[]={NULL,"-l3",NULL};
	char *lm3cmd_back[] ={NULL,"-b",NULL};
	char *lm3cmd_poweroff_plan[]={NULL,"-p",NULL};
	char *lm3cmd_power[]={NULL,"-g",NULL};
	char *lm3cmd_poweroff_now[]={NULL,"-c",NULL};
	argparam_t *ap = argparam;
	assert(ap);
	
	// open the m3_file
    fd = open("/dev/lm3_spi",O_RDWR,0777);	
	if(fd < 0) {
		printf("open /dev/lm3_spi is error \n");
		goto manageerror;
	}
	fdph = open("/dev/phss", O_RDWR);
	if (fdph < 0) {
		printf("open /dev/phss is error \n");
		goto manageerror;
	}
	
	// send key signal to m3
	for(i=0;i < 4;i++)
	{
		if((lm3_main(fd,2,&lm3cmd_key[1],1)) == 1)
		break;
			struct timespec rem = { 1, 0};
			nanosleep(&rem, &rem);
	}
	// open the light 2
	for(i=0;i < 4;i++)
	{
		if((lm3_main(fd,2,&lm3cmd_l2[1],1)) == 1)
		break;
			struct timespec rem = { 1, 0};
			nanosleep(&rem, &rem);
	}

	gettimeofday(&oldtimeof, NULL);
	while (1) 
	{

		gettimeofday(&newtimeof, NULL);

		if ((newtimeof.tv_sec *1000+newtimeof.tv_usec/1000)-(oldtimeof.tv_sec*1000+oldtimeof.tv_usec/1000)>10*60*1000) {
		oldtimeof.tv_sec= newtimeof.tv_sec;
		oldtimeof.tv_usec= newtimeof.tv_usec;

		if(light2_status == 1) {
				// turn off the back light
				for(i=0;i < 4;i++) 
				{
					lm3_main(fd,2,&lm3cmd_back[1],0);
					struct timespec rem = { 0, 5000};
					nanosleep(&rem, &rem);
				}
				
		}
		photoresponse_back = photoresponse(fdph);
		printf("the system get the photoresponse argc \n");
		if(light2_status == 1) {
				// open the back light
				for(i=0;i < 4;i++)
				{
						lm3_main(fd,2,&lm3cmd_back[1],1);
						struct timespec rem = { 0, 5*1000};
						nanosleep(&rem, &rem);
				}
		}

		if(photoresponse_back < 18){
			if (light2_status == 0)
			{
				printf("you back light will be open\n");
				for(i=0;i < 4;i++)
				{
					if((lm3_main(fd,2,&lm3cmd_back[1],1)) == 1)
						break;
						struct timespec rem = { 0, 5*1000};
						nanosleep(&rem, &rem);
				}
				light2_status = 1;
			}
		}
		else if ((photoresponse_back >= 18))
		{	
			if(light2_status == 1)
			{
			printf(" your back light is open and the envimoute is I will turn off it\n");
				for(i=0;i < 4;i++)
				{
					if((lm3_main(fd,2,&lm3cmd_back[1],0)) == 1)
					break;
					struct timespec rem = { 0, 5*1000};
					nanosleep(&rem, &rem);
				}
				light2_status = 0;
			}
		}
		}
		// power off function
		if(read_power_off_args())
		{
			printf("************* I will be turn off the light 2****************\n");
			for(i=0;i < 4;i++)
			{
				if((lm3_main(fd,2,&lm3cmd_l2[1],0)) == 1)
				break;
				struct timespec rem = { 1, 0};
				nanosleep(&rem, &rem);
				//sleep(1);
			}
			printf("****  you are into the poweroff function *********\n");
/*			pthread_cancel(ap->gpsmod.tid);
			pthread_cancel(ap->wifimod.tid);
			pthread_cancel(ap->facemod.tid);
			pthread_cancel(ap->fingermod.tid);
			pthread_cancel(ap->idcardmod.tid);
			pthread_cancel(ap->voicemod.tid);
			pthread_cancel(ap->iccardmod.tid);
			pthread_cancel(ap->wlan3gmod.tid);
			pthread_cancel(ap->videomod[0].tid);
			pthread_cancel(ap->videomod[1].tid);
			pthread_cancel(ap->videomod[2].tid);
			pthread_cancel(ap->videomod[3].tid);a*/
			lm3_main(fd,2,&lm3cmd_poweroff_plan[1],0);
			for(i=0;i < 4;i++)
			{
				printf("*************** the system will be halt ******************\n");
				printf("*************** I will be send the cmd poweroff now ******\n");
				lm3_main(fd,2,&lm3cmd_poweroff_now[1],0);
				struct timespec rem = { 1, 0};
				nanosleep(&rem, &rem);
				//sleep(1);
			}
		}
	}
manageerror:
	jpuart_close(ap->iccardmod.iccardp);
	close(fd);
	close(fdph);
	pthread_exit(NULL);
}
static int send_op_packet(argparam_t *ap, int cmd, int seqsk, int leng,  char *data)
{
	int *seq = NULL;
	struct oppacketdata sdate;

	if (ap->gpsmod.enable && ap->wifimod.enable) {
		memset(&sdate, 0, sizeof(struct oppacketdata));

		sdate.opstart = OPSTARTDATA;
		sdate.opcmd = cmd;
		sdate.opdevid = ap->jpdev.devid;
		seq = (int *)(&(sdate.sks.sign));
		*seq = seqsk;

		sdate.leng = leng + OPSGPSLENG;
		memcpy(sdate.data, data, leng);
		memcpy(sdate.data+leng, ap->gpsmod.recvp, OPSGPSLENG);

		sdate.sks.chk = checksum((char *)&(sdate.opcmd), OPSGPSLENG + OPPACKETHEADLENG + leng-4);
		
		sdate.opstart = htonl(sdate.opstart);
		sdate.opcmd = htonl(sdate.opcmd);
		sdate.opdevid = htonl(sdate.opdevid);
//		*seq = htonl(*seq);
		sdate.leng = htonl(sdate.leng);

		memset(ap->wifimod.sendp, 0, leng + OPSGPSLENG + OPPACKETHEADLENG);
		memcpy(ap->wifimod.sendp, &sdate, leng + OPSGPSLENG + OPPACKETHEADLENG);
		
		sync_send_subpthread_signal(ap, WIFI_PTHREADNO, 
									&(ap->wifimod.pcond), 
									&(ap->wifimod.pmutex), 
									leng + OPSGPSLENG + OPPACKETHEADLENG);
	}
	return 0;	
}

static int recv_op_packet(argparam_t *ap, oppacketdata_t *rcvdate)
{
	int seq;	
	if (ap->gpsmod.enable && ap->wifimod.enable) {
		while (1) {
			memset(ap->wifimod.recvp, 0, 4);
			sync_recv_subpthread_signal(ap, WIFI_PTHREADNO, &ap->wifimod.pcond, &ap->wifimod.pmutex, 4);
			if (ap->wifimod.recvleng < 0) {
				printf("recv data error\n");
				return -1;
			}
			if (OPSTARTDATA == (*((int *)(ap->wifimod.recvp)))) {
				rcvdate->opstart = OPSTARTDATA;
				break;
			}
		}

		sync_recv_subpthread_signal(ap, WIFI_PTHREADNO, &ap->wifimod.pcond, &ap->wifimod.pmutex, 16);
		rcvdate->opcmd = ntohl(*(int *)ap->wifimod.recvp);	
		rcvdate->opdevid = ntohl(*((int *)ap->wifimod.recvp+1));	
		seq = ntohl(*((int *)ap->wifimod.recvp+2));	
		
		rcvdate->sks.sign = ((seq >> 24) & 0xff);	
		rcvdate->sks.chk = ((seq >> 16) & 0xff);	
		//rcvdate->sks.chk = (char )0;
//		printf("checksum: 0x%x\n", checksum((char *)&(rcvdate->opcmd), rcvdate->leng+300+19));
		rcvdate->sks.sq = (seq & 0xffff);	
		rcvdate->leng = ntohl(*((int *)(ap->wifimod.recvp)+3));

		if (rcvdate->leng >= WIFI_RECVBUF_SIZE) {
			rcvdate->leng = WIFI_RECVBUF_SIZE - 1;
		}
		memset(ap->wifimod.recvp, 0, rcvdate->leng);
		sync_recv_subpthread_signal(ap, WIFI_PTHREADNO, 
									&ap->wifimod.pcond, &ap->wifimod.pmutex, rcvdate->leng);
		memcpy(rcvdate->data, ap->wifimod.recvp, rcvdate->leng);	
	}
	return 0;
}

int regist_deviceid_gpstime(argparam_t *ap)
{
	char sdate[10];
	oppacketdata_t rcvdate;
	memset(sdate, 0, 10);
	sdate[0] = CAR_STATUS_STOP;
	send_op_packet(ap, CMD_ONLY_GPS, 0, 1,  sdate);
	if (recv_op_packet(ap, &rcvdate) < 0) {
		return -1;	
	}
	/*
	printf("leng = 0x%x\n", rcvdate.leng);	
	printf("socketrecvdata:0x%x, 0x%x, 0x%x, 0x%x\n", rcvdate.opstart, rcvdate.opcmd, rcvdate.opdevid, rcvdate.leng);

	for (i = 0; i < rcvdate.leng; i++) {
		if (!(i % 10)) {
			printf("\n");
		}
		printf("0x%x,  ", *(((char *)&rcvdate)+i));
		
	}
	*/
	printf("\nrecvdata end\n");
	return 0;
}

int sc_info_writefile(char *filename, char *data, int leng)
{
	int wolen = 0, walen = 0;
	FILE *fd = fopen(filename, "w+");
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

int parse_scpacket(oppacketdata_t *rcvdate)
{
	person_t person;
	char picname[256];
	char personconfig[256];
	char timestr[6];
	int ret = 0;
	scnetdatainfo_t *scnetinfo = (scnetdatainfo_t *)rcvdate->data; 
	memset(&person, 0, sizeof(person_t));
	memset(timestr, 0, 6);
	memset(picname, 0, 256);
	memset(personconfig, 0, 256);

	memcpy(person.basic.name, scnetinfo->name, 16);
	person.basic.gender =scnetinfo->sex;
	memcpy(person.basic.idnum, (char *)scnetinfo->idnumb, 20);
		
	memcpy((char *)&(person.iccard.cardid), scnetinfo->icnumb, 6);
	printf("cardid: 0x%x, %x, %x, %x\n", 
					scnetinfo->icnumb[0], 
					scnetinfo->icnumb[1], 
					scnetinfo->icnumb[2], 
					scnetinfo->icnumb[3]);

//		scnetinfo->idtype;
//		scnetinfo->idlengsign;
//		scnetinfo->status;
	memcpy(person.basic.pic, scnetinfo->idnumb, 18);
	memcpy(person.basic.pic+strlen(person.basic.pic), ".png", 4);
	person.basic.pic[strlen(person.basic.pic)] = '\0';
	printf("pcstudent pic:%s\n", person.basic.pic);
	printf("scnetinfo_level = %d\n", scnetinfo->tech_level);	

	//scnetinfo->current_curse;//1--kemu1   2---kemu2  3--kemu3
	if (!scnetinfo->tech_level)  {//student--0   coach--1-5
		memcpy(timestr, scnetinfo->priv.stu.plan_start_time, 2);
		person.train[0].starttime.tm_sec = 	atoi(timestr);
		memcpy(timestr, scnetinfo->priv.stu.plan_start_time+2, 2);
		person.train[0].starttime.tm_min = 	atoi(timestr);
		memcpy(timestr, scnetinfo->priv.stu.plan_start_time+4, 2);
		person.train[0].starttime.tm_hour = atoi(timestr);	
		memcpy(timestr, scnetinfo->priv.stu.plan_end_time, 2);
		person.train[0].endtime.tm_sec = atoi(timestr);
		memcpy(timestr, scnetinfo->priv.stu.plan_end_time+2, 2);
		person.train[0].endtime.tm_min = 	atoi(timestr);
		memcpy(timestr, scnetinfo->priv.stu.plan_end_time+4, 2);
		person.train[0].endtime.tm_hour = 	atoi(timestr);
		memcpy(person.fingerfeature.byFprMut, scnetinfo->fgfeature.byFprMut, sizeof(TCFDBCELL));
		
		snprintf(picname,256,"%s/%s", jpsys_content_base, person.basic.pic);
		snprintf(personconfig, 256, "%s/%s", jpsys_content_base, "psconfig");
		sc_info_writefile(picname, scnetinfo->priv.stu.pic, rcvdate->leng-64-sizeof(TCFDBCELL)-256);
		sc_info_writefile(personconfig, (char *)&person, sizeof(person_t));
	} else {
//		scnetinfo->train_cartype;//C1

		memcpy(person.fingerfeature.byFprMut, scnetinfo->fgfeature.byFprMut, sizeof(TCFDBCELL));
		//scnetinfo->facefeature[256];
		//scnetinfo->pic[
		//
		//0];
		
		snprintf(picname,256,"%s/%s", jpsys_content_base, person.basic.pic);
		snprintf(personconfig, 256, "%s/%s", jpsys_content_base, "pcconfig");
		sc_info_writefile(picname, scnetinfo->priv.coach.pic, rcvdate->leng - 80 - sizeof(TCFDBCELL) - 256);
		sc_info_writefile(personconfig, (char *) &person, sizeof(person_t));
	}
	ret = ((rcvdate->sks.sign >> 5) & 0x3);
	
	switch(ret) {
	case 0:return 1;
	case 1:return 1;//start
	case 2:return 0;//midoue
	case 3:return 0; //end
	default:break;	
	}
	return ret;
}

int download_datefrom_server(argparam_t *ap)
{
	char sdate[10];
	oppacketdata_t rcvdate;
	memset(sdate, 0, 10);
	sdate[0] = CAR_STATUS_STOP;
	send_op_packet(ap, CMD_UPDATE_SC_INFO, 0, 1,  sdate);
	do {
		if (recv_op_packet(ap, &rcvdate)< 0){
			return -1;	
		}
		if (!rcvdate.leng) {
			printf("recv leng = 0\n");
			break;
		}
		/*
		printf("leng = 0x%x\n", rcvdate.leng);	
		printf("socketrecvdata:0x%x, 0x%x, 0x%x, 0x%x\n", rcvdate.opstart, rcvdate.opcmd, rcvdate.opdevid, rcvdate.leng);

		for (i = 0; i < rcvdate.leng+OPPACKETHEADLENG; i++) {
			if (!(i % 10)) {
				printf("\n");
			}
			printf("0x%x,  ", *(((char *)&rcvdate)+i));
		}
		printf("\nrecvdata end\n");
	*/
	} while (parse_scpacket(&rcvdate));
	
	return 0;
}


int sync_manage_strategy(argparam_t *argparam)
{
	assert(argparam);
	return JPSUCCESS;
}

int check_organizetion_operate_status(argparam_t *argparam)
{
	assert(argparam);

	return JPSUCCESS;

}

int check_car_operate_status(argparam_t *argparam)
{
	assert(argparam);
	return JPSUCCESS;

}
int init_finger_status(argparam_t *argparam)
{
	return JPSUCCESS;
}

int init_car_position_status(argparam_t *argparam)
{
	assert(argparam);
//GPS position
	//printf("init car position status ok\n");
	return JPSUCCESS;
}
int init_iccard_mod(argparam_t *argparam)
{
	assert(argparam);
	return JPSUCCESS;
}
int init_GPS_mod(argparam_t *argparam)
{
	assert(argparam);
	//jp_u8_t *buffer;

	//printf("init GPS mod ok\n");
	return JPSUCCESS;
}
	


int jpdevice_init(argparam_t *argparam, char *config)
{
	argparam_t *ap = argparam;
	assert(ap);
	int psva[MAX_PTHREAD];//pthread sync val array
	int psvatimes[MAX_PTHREAD] = {0};//pthread sync val array
	
//	system("echo -e \"\033[9;0]\" > /dev/tty1");

	pthread_mutex_init(&ap->sync_mutex, NULL);
	
	//creat gps pthread
	if (ap->gpsmod.enable) {
		pthread_cond_init(&ap->gpsmod.pcond, &(ap->gpsmod.pattr));
		if (!pthread_attr_init(&ap->gpsmod.tattr)) {
			if (!pthread_attr_setdetachstate(&ap->gpsmod.tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->gpsmod.tid, &ap->gpsmod.tattr, gps_pthread, (void *)ap);
				pthread_attr_destroy(&ap->gpsmod.tattr);
			}	
		}
	}

	//creat wifi pthread
	if (ap->wifimod.enable) {
		pthread_cond_init(&ap->wifimod.pcond, &(ap->wifimod.pattr));
		if (!pthread_attr_init(&ap->wifimod.tattr)) {
			if (!pthread_attr_setdetachstate(&ap->wifimod.tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->wifimod.tid, &ap->wifimod.tattr, wifi_pthread, (void *)ap);
				pthread_attr_destroy(&ap->wifimod.tattr);
			}	
		}
	}
/*
	if (ap->wifimod.enable) {
		pthread_cond_init(&ap->servermod.pcond, &(ap->servermod.pattr));
		if (!pthread_attr_init(&ap->servermod.tattr)) {
			if (!pthread_attr_setdetachstate(&ap->servermod.tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->servermod.tid, &ap->servermod.tattr, server_pthread, (void *)ap);
				pthread_attr_destroy(&ap->servermod.tattr);
			}	
		}
	}
*/
	//creat face pthread
	if (ap->facemod.enable) {
		pthread_cond_init(&ap->facemod.pcond, &(ap->facemod.pattr));
		if (!pthread_attr_init(&ap->facemod.tattr)) {
			if (!pthread_attr_setdetachstate(&ap->facemod.tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->facemod.tid, &ap->facemod.tattr, face_pthread, (void *)ap);
				pthread_attr_destroy(&ap->facemod.tattr);
			}	
		}
	}

	//creat face udp pthread
	if (ap->faceudp.enable) {
		pthread_cond_init(&ap->faceudp.pcond, &(ap->faceudp.pattr));
		if (!pthread_attr_init(&ap->faceudp.tattr)) {
			if (!pthread_attr_setdetachstate(&ap->faceudp.tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->faceudp.tid, &ap->faceudp.tattr, face_pthread_udp, (void *)ap);
				pthread_attr_destroy(&ap->faceudp.tattr);
			}	
		}
	}
	/*
	//creat jpcard pthread
	if (ap->jpcardmod.enable) {
		pthread_cond_init(&ap->jpcardmod.pcond, &(ap->jpcardmod.pattr));
		if (!pthread_attr_init(&ap->jpcardmod.tattr)) {
			if (!pthread_attr_setdetachstate(&ap->jpcardmod.tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->jpcardmod.tid, &ap->jpcardmod.tattr, jpcard_pthread, (void *)ap);
				pthread_attr_destroy(&ap->jpcardmod.tattr);
			}	
		}
	}*/
	//creat idcard pthread
	if (ap->idcardmod.enable) {
		pthread_cond_init(&ap->idcardmod.pcond, &(ap->idcardmod.pattr));
		if (!pthread_attr_init(&ap->idcardmod.tattr)) {
			if (!pthread_attr_setdetachstate(&ap->idcardmod.tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->idcardmod.tid, &ap->idcardmod.tattr, idcard_pthread, (void *)ap);
				pthread_attr_destroy(&ap->idcardmod.tattr);
			}	
		}
	}
	//creat finger pthread
	if (ap->fingermod.enable) {
		pthread_cond_init(&ap->fingermod.pcond, &(ap->fingermod.pattr));
		if (!pthread_attr_init(&ap->fingermod.tattr)) {
			if (!pthread_attr_setdetachstate(&ap->fingermod.tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->fingermod.tid, &ap->fingermod.tattr, finger_pthread, (void *)ap);
				pthread_attr_destroy(&ap->fingermod.tattr);
			}	
		}
	}

	//creat wlan3g pthread
	if (ap->wlan3gmod.enable) {
		pthread_cond_init(&ap->wlan3gmod.pcond, &(ap->wlan3gmod.pattr));
		if (!pthread_attr_init(&ap->wlan3gmod.tattr)) {
			if (!pthread_attr_setdetachstate(&ap->wlan3gmod.tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->wlan3gmod.tid, &ap->wlan3gmod.tattr, wlan3g_pthread, (void *)ap);
				pthread_attr_destroy(&ap->wlan3gmod.tattr);
			}	
		}
	}

	//creat voice pthread
	if (ap->voicemod.enable) {
		pthread_cond_init(&ap->voicemod.pcond, &(ap->voicemod.pattr));
		if (!pthread_attr_init(&ap->voicemod.tattr)) {
			if (!pthread_attr_setdetachstate(&ap->voicemod.tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->voicemod.tid, &ap->voicemod.tattr, voice_pthread, (void *)ap);
				pthread_attr_destroy(&ap->voicemod.tattr);
			}	
		}
	}

	//creat video channal 0 pthread
	if (ap->videomod[0].enable) {
		pthread_cond_init(&ap->videomod[0].pcond, &(ap->videomod[0].pattr));
		if (!pthread_attr_init(&ap->videomod[0].tattr)) {
			if (!pthread_attr_setdetachstate(&ap->videomod[0].tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->videomod[0].tid, &ap->videomod[0].tattr, video0_pthread, (void *)ap);
				pthread_attr_destroy(&ap->videomod[0].tattr);
			}	
		}
	}
	//creat voice channal 1 pthread
	if (ap->videomod[1].enable) {
		pthread_cond_init(&ap->videomod[1].pcond, &(ap->videomod[1].pattr));
		if (!pthread_attr_init(&ap->videomod[1].tattr)) {
			if (!pthread_attr_setdetachstate(&ap->videomod[1].tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->videomod[1].tid, &ap->videomod[1].tattr, video1_pthread, (void *)ap);
				pthread_attr_destroy(&ap->videomod[1].tattr);
			}	
		}
	}
	//creat voice channal 2 pthread
	if (ap->videomod[2].enable) {
		pthread_cond_init(&ap->videomod[2].pcond, &(ap->videomod[2].pattr));
		if (!pthread_attr_init(&ap->videomod[2].tattr)) {
			if (!pthread_attr_setdetachstate(&ap->videomod[2].tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->videomod[2].tid, &ap->videomod[2].tattr, video2_pthread, (void *)ap);
				pthread_attr_destroy(&ap->videomod[2].tattr);
			}	
		}
	}

	//creat voice channal 3 pthread
	if (ap->videomod[3].enable) {
		pthread_cond_init(&ap->videomod[3].pcond, &(ap->videomod[3].pattr));
		if (!pthread_attr_init(&ap->videomod[3].tattr)) {
			if (!pthread_attr_setdetachstate(&ap->videomod[3].tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->videomod[3].tid, &ap->videomod[3].tattr, video3_pthread, (void *)ap);
				pthread_attr_destroy(&ap->videomod[3].tattr);
			}	
		}
	}


	//create manage pthread
	if (1) {
		pthread_cond_init(&ap->managemod.pcond, &(ap->managemod.pattr));
		if (!pthread_attr_init(&ap->managemod.tattr)) {
			if (!pthread_attr_setdetachstate(&ap->managemod.tattr, PTHREAD_CREATE_DETACHED)) {
				pthread_create(&ap->managemod.tid,&ap->managemod.tattr,manage_pthread,(void *)ap);
				pthread_attr_destroy(&ap->managemod.tattr);
			}
		}
	}

//	while(1) {
	//gps
		if (ap->gpsmod.enable) {
			while (!(psva[GPS_PTHREADNO] = pthread_read_sync_val(ap, GPS_PTHREADNO, CMDDEVINIT))) {
				struct timespec rem = { 0, 10000};
				nanosleep(&rem, &rem);
				//usleep(10000);
				continue;	
			}
		}
//wifi
		if (ap->wifimod.enable) {
			while (!(psva[WIFI_PTHREADNO] = pthread_read_sync_val(ap, WIFI_PTHREADNO, CMDDEVINIT))) {
				if (psvatimes[WIFI_PTHREADNO]++>DEVINIT_SYNC_TIMEOUT) {
					break;
				}
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
			//	usleep(100);
				continue;	
			}
		}
		//face tcp
		if (ap->facemod.enable) {
			while (!(psva[FACE_PTHREADNO] = pthread_read_sync_val(ap, FACE_PTHREADNO, CMDDEVINIT))) {
				if (psvatimes[FACE_PTHREADNO]++>DEVINIT_SYNC_TIMEOUT) {
					break;
				}
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
				//usleep(100);
				continue;	
			}
		}
		//face udp
		if (ap->faceudp.enable) {
			while (!(psva[FACE_UDP_PTHREADNO] = pthread_read_sync_val(ap, FACE_UDP_PTHREADNO, CMDDEVINIT))) {
				if (psvatimes[FACE_UDP_PTHREADNO]++>DEVINIT_SYNC_TIMEOUT) {
					break;
				}
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
				//usleep(100);
				continue;	
			}
		}
	/*	
	 *jpcard
		if (ap->jpcardmod.enable) {
			if (!(psva[JPCARD_PTHREADNO] = pthread_read_sync_val(ap, JPCARD_PTHREADNO, CMDDEVINIT))) {
				continue;	
			}
		}
	*/	
	//idcard
		if (ap->idcardmod.enable) {
			while (!(psva[IDCARD_PTHREADNO] = pthread_read_sync_val(ap, IDCARD_PTHREADNO, CMDDEVINIT))) {
				if (psvatimes[IDCARD_PTHREADNO]++>DEVINIT_SYNC_TIMEOUT) {
					break;
				}
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
				//usleep(100);
				continue;	
			}
		}
		//finger
		if (ap->fingermod.enable) {
			while (!(psva[FINGER_PTHREADNO] = pthread_read_sync_val(ap, FINGER_PTHREADNO, CMDDEVINIT))) {
				if (psvatimes[FINGER_PTHREADNO]++>DEVINIT_SYNC_TIMEOUT) {
					break;
				}
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
				//usleep(100);
				continue;	
			}
		}
		//wlan3g
		if (ap->wlan3gmod.enable) {
			while (!(psva[WLAN3G_PTHREADNO] = pthread_read_sync_val(ap, WLAN3G_PTHREADNO, CMDDEVINIT))) {
				if (psvatimes[WLAN3G_PTHREADNO]++>DEVINIT_SYNC_TIMEOUT) {
					break;
				}
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
				//usleep(100);
				continue;	
			}
		}
		
		//voice
		if (ap->voicemod.enable) {
			while (!(psva[VOICE_PTHREADNO] = pthread_read_sync_val(ap, VOICE_PTHREADNO, CMDDEVINIT))) {
				if (psvatimes[VOICE_PTHREADNO]++>DEVINIT_SYNC_TIMEOUT) {
					break;
				}
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
				continue;	
			}
		}
		
	//video0	
		if (ap->videomod[0].enable) {
			while (!(psva[VIDEO0_PTHREADNO] = pthread_read_sync_val(ap, VIDEO0_PTHREADNO, CMDDEVINIT))) {
				if (psvatimes[VIDEO0_PTHREADNO]++>DEVINIT_SYNC_TIMEOUT) {
					break;
				}
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
				//usleep(100);
				continue;	
			}
		}
		//video1
		if (ap->videomod[1].enable) {
			while (!(psva[VIDEO1_PTHREADNO] = pthread_read_sync_val(ap, VIDEO1_PTHREADNO, CMDDEVINIT))) {
				if (psvatimes[VIDEO1_PTHREADNO]++>DEVINIT_SYNC_TIMEOUT) {
					break;
				}
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
				//usleep(100);
				continue;	
			}	
		}
		//video2
		if (ap->videomod[2].enable) {
			while (!(psva[VIDEO2_PTHREADNO] = pthread_read_sync_val(ap, VIDEO2_PTHREADNO, CMDDEVINIT))) {
				if (psvatimes[VIDEO2_PTHREADNO]++>DEVINIT_SYNC_TIMEOUT) {
					break;
				}
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
				//usleep(100);
				continue;	
			}
		}
		//video3
		if (ap->videomod[3].enable) {
			while (!(psva[VIDEO3_PTHREADNO] = pthread_read_sync_val(ap, VIDEO3_PTHREADNO, CMDDEVINIT))) {
				if (psvatimes[VIDEO3_PTHREADNO]++>DEVINIT_SYNC_TIMEOUT) {
					break;
				}
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
				//usleep(100);
				continue;	
			}	
		}
		//recv all pthread val
//	}
	TESTPRINT("GPS_PTHREADNO", psva[GPS_PTHREADNO]);
	TESTPRINT("WIFI_PTHREADNO", psva[WIFI_PTHREADNO]);
	TESTPRINT("FACE_PTHREADNO", psva[FACE_PTHREADNO]);
	TESTPRINT("FACE_UDP_PTHREADNO", psva[FACE_UDP_PTHREADNO]);
	TESTPRINT("JPCARD_PTHREADNO", psva[ICCARD_PTHREADNO]);
	TESTPRINT("IDCARD_PTHREADNO", psva[IDCARD_PTHREADNO]);
	TESTPRINT("FINGER_PTHREADNO", psva[FINGER_PTHREADNO]);
	TESTPRINT("WLAN3G_PTHREADNO", psva[WLAN3G_PTHREADNO]);
	TESTPRINT("VOICE_PTHREADNO", psva[VOICE_PTHREADNO]);
	TESTPRINT("VIDEO0_PTHREADNO", psva[VIDEO0_PTHREADNO]);
	TESTPRINT("VIDEO1_PTHREADNO", psva[VIDEO1_PTHREADNO]);
	TESTPRINT("VIDEO2_PTHREADNO", psva[VIDEO2_PTHREADNO]);
	TESTPRINT("VIDEO3_PTHREADNO", psva[VIDEO3_PTHREADNO]);
	
	if (0) {
		//getgpsinfo(ap, GPRMC, msg);
	}
//		getgpsinfo(ap, GPRMC, msg);
//	test_wlan3g(ap);
//	check_terminal_data_and_time(ap);	
	/*
	upload_deviceid(ap);
	update_device_program(ap);
	sync_manage_strategy(ap);
	check_organizetion_operate_status(ap);
	check_car_operate_status(ap);
	init_finger_status(ap);
	init_car_position_status(ap);
	init_iccard_mod(ap);
	init_GPS_mod(ap);
	*
	*/


	return 0;
}

int jpdevice_cleanup(argparam_t *argparam)
{
	argparam_t *ap = argparam;
	pthread_cond_destroy(&ap->gpsmod.pcond);
	pthread_cond_destroy(&ap->wifimod.pcond);
	return 0;

}

static void init_subpthread_cmd_recv(argparam_t *ap, enum pthread_nb pnb, int len)
{

	switch(pnb) {
	case GPS_PTHREADNO:
		ap->gpsmod.recvleng = len;
		ap->gpsmod.cmd = CMDRECV;
		break;
	case WIFI_PTHREADNO:
		ap->wifimod.recvleng = len;
		ap->wifimod.cmd = CMDRECV;
		break;
	case VIDEO0_PTHREADNO:
		ap->videomod[0].recvleng = len;
		ap->videomod[0].cmd = CMDRECV;
		break;
	case VIDEO1_PTHREADNO:
		ap->videomod[1].recvleng = len;
		ap->videomod[1].cmd = CMDRECV;
		break;
	case VIDEO2_PTHREADNO:
		ap->videomod[2].recvleng = len;
		ap->videomod[2].cmd = CMDRECV;
		break;
	case VIDEO3_PTHREADNO:
		ap->videomod[3].recvleng = len;
		ap->videomod[3].cmd = CMDRECV;
		break;
	case VOICE_PTHREADNO:
		ap->voicemod.recvleng = len;
		ap->voicemod.cmd = CMDRECV;
		break;
	case ICCARD_PTHREADNO:
		ap->iccardmod.recvleng = len;
		ap->iccardmod.cmd = CMDRECV;
		break;
	case IDCARD_PTHREADNO:
		ap->idcardmod.recvleng = len;
		ap->idcardmod.cmd = CMDRECV;
		break;
	case FINGER_PTHREADNO:
		ap->fingermod.recvleng = len;
		ap->fingermod.cmd = CMDRECV;
		break;
	case WLAN3G_PTHREADNO:
		ap->wlan3gmod.recvleng = len;
		ap->wlan3gmod.cmd = CMDRECV;
		break;
	case FACE_PTHREADNO:
		ap->facemod.recvleng = len;
		ap->facemod.cmd = CMDRECV;
		break;
	case FACE_UDP_PTHREADNO:
		ap->faceudp.recvleng = len;
		ap->faceudp.cmd = CMDRECV;
		break;
	default:	
		break;
	}
	pthread_set_sync_val(ap, pnb, CMDRECV, RECVING_SYNCVAL);

	return ;

}


static void init_subpthread_cmd_send(argparam_t *ap, enum pthread_nb pnb, int len)
{

	switch(pnb) {
	case GPS_PTHREADNO:
		ap->gpsmod.sendleng = len;
		ap->gpsmod.cmd = CMDSEND;
		break;
	case WIFI_PTHREADNO:
		ap->wifimod.sendleng = len;
		ap->wifimod.cmd = CMDSEND;
		break;
	case VIDEO0_PTHREADNO:
		ap->videomod[0].sendleng = len;
		ap->videomod[0].cmd = CMDSEND;
		break;
	case VIDEO1_PTHREADNO:
		ap->videomod[1].sendleng = len;
		ap->videomod[1].cmd = CMDSEND;
		break;
	case VIDEO2_PTHREADNO:
		ap->videomod[2].sendleng = len;
		ap->videomod[2].cmd = CMDSEND;
		break;
	case VIDEO3_PTHREADNO:
		ap->videomod[3].sendleng = len;
		ap->videomod[3].cmd = CMDSEND;
		break;
	case VOICE_PTHREADNO:
		ap->voicemod.sendleng = len;
		ap->voicemod.cmd = CMDSEND;
		break;
	case ICCARD_PTHREADNO:
		ap->iccardmod.sendleng = len;
		ap->iccardmod.cmd = CMDSEND;
		break;
	case IDCARD_PTHREADNO:
		ap->idcardmod.sendleng = len;
		ap->idcardmod.cmd = CMDSEND;
		break;
	case FINGER_PTHREADNO:
		ap->fingermod.sendleng = len;
		ap->fingermod.cmd = CMDSEND;
		break;
	case WLAN3G_PTHREADNO:
		ap->wlan3gmod.sendleng = len;
		ap->wlan3gmod.cmd = CMDSEND;
		break;
	case FACE_PTHREADNO:
		ap->facemod.sendleng = len;
		ap->facemod.cmd = CMDSEND;
		break;
	case FACE_UDP_PTHREADNO:
		ap->faceudp.sendleng = len;
		ap->faceudp.cmd = CMDSEND;
		break;
	default:	
		break;
	}
	pthread_set_sync_val(ap, pnb, CMDSEND, SENDING_SYNCVAL);

	return ;
}



static void wait_subpthread_wait(argparam_t *ap, enum pthread_nb pnb)
{
	assert(ap);
	int sendflags;
	int recvflags;
	while(1) {
		sendflags = pthread_read_sync_val(ap, pnb, CMDSEND);
		recvflags = pthread_read_sync_val(ap, pnb, CMDRECV);
		if ((SENDOK_SYNCVAL == sendflags) && (RECVOK_SYNCVAL == recvflags)) {
			break;	
		}
//		printf("waiting: sendflag: 0x%x, recvflags: 0x%x\n", sendflags, recvflags);
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
		//usleep(100);
		continue;	

	}
	return ;
}


int async_recvok(argparam_t *argparam, enum pthread_nb pnb)
{
	argparam_t *ap = argparam;
	assert(ap);
				struct timespec rem = { 0, 1000};
				nanosleep(&rem, &rem);
	//usleep(1);
	while (RECVING_SYNCVAL == pthread_read_sync_val(ap, pnb, CMDRECV)){
				struct timespec rem = { 0, 100000};
				nanosleep(&rem, &rem);
			//usleep(100);
	}
	pthread_set_sync_val(ap, pnb, CMDRECV, DEVINITOK_SYNCVAL);
	return 0;
}

int async_sendok(argparam_t *argparam, enum pthread_nb pnb)
{
	argparam_t *ap = argparam;
	assert(ap);
				struct timespec rem = { 0, 1000};
				nanosleep(&rem, &rem);
	//usleep(1);
	while (SENDING_SYNCVAL == pthread_read_sync_val(ap, pnb, CMDSEND)){
				struct timespec rem = { 0, 10000};
				nanosleep(&rem, &rem);
			//usleep(10);
	}
	pthread_set_sync_val(ap, pnb, CMDSEND, DEVINITOK_SYNCVAL);
	return 0;
}


int async_send_subpthread_signal(argparam_t *argparam, enum pthread_nb pnb, pthread_cond_t *cond,pthread_mutex_t *mutex, int len)
{
	argparam_t *ap = argparam;
	assert(ap);
	pthread_mutex_lock(mutex);
	wait_subpthread_wait(ap, pnb);
	
	init_subpthread_cmd_send(ap, pnb, len);
	printf("pthread_comd_signal!\n");
	pthread_mutex_unlock(mutex);
	pthread_cond_signal(cond);
				struct timespec rem = { 0, 1000};
				nanosleep(&rem, &rem);
//	usleep(1);
	return 0;
}

int async_recv_subpthread_signal(argparam_t *argparam, enum pthread_nb pnb,pthread_cond_t *cond, pthread_mutex_t *mutex,int len)
{
	argparam_t *ap = argparam;
	assert(ap);
				struct timespec rem = { 0, 1000};
				nanosleep(&rem, &rem);
	//usleep(1);
	pthread_mutex_lock(mutex);
	wait_subpthread_wait(ap, pnb);
	
	init_subpthread_cmd_recv(ap, pnb, len);
	printf("signal recv!\n");
	pthread_mutex_unlock(mutex);
	pthread_cond_signal(cond);
	struct timespec req = { 0, 10000};
	nanosleep(&req, &req);
//	usleep(10);
	return 0;
}


int sync_send_subpthread_signal(argparam_t *argparam, enum pthread_nb pnb, pthread_cond_t *cond, pthread_mutex_t *mutex, int len)
{
	argparam_t *ap = argparam;
	assert(ap);
	
	async_send_subpthread_signal(ap, pnb, cond, mutex, len);
	struct timespec rem = { 0, 1000};
	nanosleep(&rem, &rem);
	//usleep(1);
	wait_subpthread_wait(ap, pnb);
	
	return 0;
}


int sync_recv_subpthread_signal(argparam_t *argparam, enum pthread_nb pnb, pthread_cond_t *cond, pthread_mutex_t *mutex, int len)
{
	argparam_t *ap = argparam;
	assert(ap);
	async_recv_subpthread_signal(ap, pnb, cond, mutex, len);
	struct timespec rem = { 0, 1000};
	nanosleep(&rem, &rem);
	//usleep(1);
	wait_subpthread_wait(ap, pnb);
	return 0;
}


#ifdef __cplusplus
}
#endif




