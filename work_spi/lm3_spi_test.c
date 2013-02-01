#include <stdio.h>
#include <string.h>
#include <fcntl.h> 
#include <unistd.h>
#include <linux/input.h>
#include <linux/types.h>
#include <unistd.h>
#include <sys/time.h>
#include "lm3_api.h"
#define  CMD_ACK                    0x00      //¿¿¿¿¿¿¿¿¿¿¿¿¿
#define  CMD_NAK                    0x01      //¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿//¿¿¿¿¿¿¿¿¿¿¿¿¿
#define  CMD_KEY                    0x02      //¿¿¿¿¿
#define  CMD_TIME                   0x03      //¿¿¿
#define  CMD_POWER                  0x04      //¿¿¿¿¿
#define  CMD_KEY_BACK_LIGHT         0X05      //¿¿¿¿¿¿¿
#define  CMD_PLAN_POWER_OFF         0X06      //¿¿¿¿¿¿¿¿¿
#define  CMD_MAGNETIC_PICK_UP       0x07      //¿¿¿¿¿(0¿¿¿¿     1¿¿¿¿)
#define  CMD_POWER_OFF_NOW          0x08      //¿¿cpu¿¿¿¿

#define  CMD_430_STAT_LIGHT1        0x09      //通过uart向430发送打开关闭状态指示灯命令
#define CMD_OUT_12V					0x0e
#define  CMD_430_STAT_LIGHT2        0x0a
#define  CMD_430_STAT_LIGHT3        0x0b
#define  CMD_ENABLE_KEY             0x0d
#define  CMD_RESTART			    0x0f
#define LM3_MSG_MAGIC   0x5a5a0f0f

#define  POWER_OFF_PLAN 		  	2	
#define  ACK_BACK					1
#define  NAK_ERROR 					-1
#define   JP_V2

#ifdef JP_V2
//adc0 						
#define    R12    15     //(K)
#define    R30    1.62
//adc1
#define    R25    22
#define    R31    5
//adc2
#define    R27    20
#define    R32    10
//adc3
#define    R28    10
#define    R33    10
//adc4
#define    R29    10
#define    R40    10
//adc5
#define    R13    10
#endif

#ifdef JP_V3
//adc0 						
#define    R12    20     //(K)
#define    R30    1
//adc1
#define    R25    20
#define    R31    5
//adc2
#define    R27    20
#define    R32    10
//adc3
#define    R28    10
#define    R33    10
//adc4
#define    R29    10
#define    R40    10
//adc5
#define    R13    10

#endif
int poll_flag;
int kfd;
int  v[6]={0};
int read_v[6] = {0};
int i = 0;
struct timeval timeout = {0,100};
fd_set fds;
unsigned char tx[20] ={0x00};
unsigned char rd[30] ={0};


m3msg_t tx_buff,rx_buff;
static int check_send(m3msg_t *data)
{
                unsigned char check1=0;
                int n = 0;
                while(n < data->datalen) {
                        check1 +=data->data[n];
                        n++;
                }
                check1 += data->cmd;
                check1 += data->datalen;
                check1 += data->sig;
                data->check =((~check1)+1)&0xff;
                return 1;
}


int lm3_main(int fd,int argc, char *argv[], int sign)
{
	int m3_cmd_back = 0; 
	int ro=0;
	if(argc>1)
	{
	strncpy(tx_buff.data,(char *)tx,20);
		if(!strcmp(argv[1],"-r"))
		{
			ro=1;
		}
		else if(!strcmp(argv[1],"-g"))
		{
			tx_buff.cmd = CMD_POWER;//RCMD_KEY;
			tx_buff.datalen = 0;
		}
		else if(!strcmp(argv[1],"-p"))
		{
			tx_buff.cmd = CMD_PLAN_POWER_OFF;//RCMD_KEY;
			printf("cmd = CMD_PLAN_POWER_OFF\n");
			tx_buff.datalen = 0;
		}
		else if(!strcmp(argv[1],"-c"))
		{
			printf("cmd = CMD_POWER_OFF_NOW\n");
			tx_buff.cmd = CMD_POWER_OFF_NOW;//RCMD_KEY;
			tx_buff.datalen = 0;
		}
		else if(!strcmp(argv[1],"-l1"))
		{
			printf("cmd = CMD_430_STAT_LIGHT1\n");
			tx_buff.cmd = CMD_430_STAT_LIGHT1;
			if(sign)
			tx_buff.data[0] =0x01;
			else
			printf("ture off the l1\n");
			tx_buff.data[0] =0x00;
			tx_buff.datalen = 1;
		}
		else if(!strcmp(argv[1],"-o"))
		{
			printf("cmd = CMD_OUT_12V\n");
			tx_buff.cmd = CMD_OUT_12V;
			if(sign)
			tx_buff.data[0] =0x01;
			else
			tx_buff.data[0] =0x00;
			tx_buff.datalen = 1;
		}
		else if(!strcmp(argv[1],"-l2"))
		{
			printf("cmd = CMD_430_STAT_LIGHT2\n");
			tx_buff.cmd = CMD_430_STAT_LIGHT2;
			if(sign)
			tx_buff.data[0] =0x01;
			else
			tx_buff.data[0] =0x00;
			tx_buff.datalen = 1;
		}
		else if(!strcmp(argv[1],"-l3"))
		{
			printf("cmd = CMD_430_STAT_LIGHT3\n");
			tx_buff.cmd = CMD_430_STAT_LIGHT3;
			if(sign)
			tx_buff.data[0] =0x01;
			else
			tx_buff.data[0] =0x00;
			tx_buff.datalen = 1;
		}
		else if(!strcmp(argv[1],"-b"))
		{
			//printf("cmd = CMD_BACk_LIGHT\n");
			tx_buff.cmd = CMD_KEY_BACK_LIGHT;
			if(sign)
			tx_buff.data[0] =0x01;
			else
			tx_buff.data[0] =0x00;
			tx_buff.datalen = 1;
		}else if(!strcmp(argv[1],"-k"))
		{
			printf("cmd = CMD_ENABLE_KEY\n");
			
			tx_buff.cmd = CMD_ENABLE_KEY;
			tx_buff.datalen = 0;
		}else if(!strcmp(argv[1],"-re"))
		{
			printf("cmd = CMD_RESTART\n");
			
			tx_buff.cmd = CMD_RESTART;
			tx_buff.datalen = 0;
		}

	}
	tx_buff.magic =  LM3_MSG_MAGIC;
	tx_buff.sig = 0;
	tx_buff.check = check_send(&tx_buff);
//	printf("start spi test!\n");
	write(fd,&tx_buff,sizeof(struct lm3msg));
	FD_ZERO(&fds);
	FD_SET(fd,&fds);
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000*500;
	switch(select(fd+1,&fds,NULL,NULL,&timeout))
	{
		case -1:
		case 0:
			break;
		default:
			read(fd,&rx_buff,sizeof(struct lm3msg));
			switch(rx_buff.cmd){
			case CMD_POWER:
			for(i=0;i<6;i++)
			{
					v[i] = (rx_buff.data[(i*2)]*100)+rx_buff.data[(i*2)+1];
				switch(i)
				{
					case 0:
					//	read_v[0] = (v[0]*16.2) / 1.62;   //input power 10 - 60
						read_v[0] = (v[0]*(R12 + R30)) / R30;   //input power 10 - 60
//						printf( "  %d  ", readv);
						break;
					case 1:
					//	read_v[1] = (v[1]*27) / 5;        //12v
						read_v[1] = (v[1]*(R25 + R31)) / R31;        //12v
						break;
					case 2:
					//	read_v[2] = (v[2]*30) / 10;       //5v
						read_v[2] = (v[2]*(R27 + R32)) / R32;       //5v
						break;
					case 3:
						read_v[3] = (v[3]*(R33 + R28)) / R33;       //3.3v
						break;
					case 4:
						read_v[4] = (v[4]*(R29 + R40)/R29)/3.83;       //output 0.7I
						
						break;
					case 5:
						read_v[5] = v[5];                 //1.2v
						break;

				}
			}
			m3_cmd_back =  read_v[4];
			break;
		case CMD_PLAN_POWER_OFF:
			printf("*************** get the CMD_PLAN_POWER_OFF **************\n");
			m3_cmd_back  = POWER_OFF_PLAN;	 // 2 
			break;	
		case CMD_ACK:
			printf("*************** get the CMD_ACK *************************\n");
			m3_cmd_back =  ACK_BACK;		//  1 
			break;
		case CMD_NAK:
			printf("*************** get the CMD_NAK *************************\n");
			m3_cmd_back = NAK_ERROR;		// -1 
			break;
		default:
			printf("**************** default get this way *******************\n");
			break;
		}
	}
	return m3_cmd_back;
}
