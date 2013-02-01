#ifndef __LM3_API__H
#define __LM3_API__H

#ifdef __cplusplus
extern "C" {
#endif


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
#define  CMD_430_STAT_LIGHT2        0x0a
#define  CMD_430_STAT_LIGHT3        0x0b
#define  CMD_ENABLE_KEY             0x0d

#define LM3_MSG_MAGIC   0x5a5a0f0f
typedef struct lm3msg
{
	int    magic;
        char   cmd;           //
        char   datalen;
        char   check;         //(¶¨ÒåÎª0ËãÐ£ÑéºÍ)
        char    sig;       //0Îª·¢ËÍ·½£¬1Îª½ÓÊÕ·½
        char   data[20];      //´«ÊäµÄ×î´ó³¤¶ÈÎª30
}m3msg_t;



int lm3_main(int fd, int argc,char *argv[],int sign);



#ifdef __cplusplus
}
#endif

#endif
