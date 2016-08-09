#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>	       /* See NOTES */
#include <sys/socket.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include "./thread-pool/thread-pool.h"
#include <sys/wait.h>
#include <netinet/tcp.h>
#include <string.h>
#include "./jsonARM/include/json/json.h"
#include "./sqlite3ARM/include/sqlite3.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include "UART/uart.h"
#include <sys/sem.h>
#include <sys/msg.h>



#define MAXBUFF 1024
#define EVENT_REGISTER 0x00
#define EVENT_LOGIN 0x01
#define EVENT_FORGET_PASSWORD 0x02
#define EVENT_MODIFY_PASSWORD 0x03
#define EVENT_GET_TEMP 0x04
#define EVENT_GET_HUMIDITY 0x05
#define EVENT_GET_ILLUMINATION 0x06
#define EVENT_GET_AIXS 0x07
#define EVENT_LED 0x08
#define EVENT_FAN 0x09
#define EVENT_DOOR 0x0a
#define EVENT_GET_VIDEO 0x0b
#define EVENT_GET_DEVICE_STATE 0x0c

#define STATE_OK 0x00
#define STATE_FAILE 0x01
#define STATE_USER_NAME_WRONG 0x02
#define STATE_USER_PASSWORD_WRONG 0x03
#define STATE_USER_PHONE_WRONG 0x04
#define STATE_SECURITY_CODE 0x05
#define STATE_USER_TOKEN_WRONG 0x06
#define STATE_PASS_DUE_TOKEN 0x07

#define DEVICE_CODE_ON 0x00
#define DEVICE_CODE_OFF 0x01
#define DEVICE_CODE_STATE 0x02

#define DEVICE_STATE_ON 0x00
#define DEVICE_STATE_OFF 0x01
#define DEVICE_STATE_BROKEN 0x02

#define APTFS 0xaa
#define FSTAP 0xff
#define FSTM0 0xdd
#define DEV_LED 0x04
#define DEV_FAN 0x05
#define DEV_DOOR 0x06
#define DEV_STATA 0x07

#define SEM_W 1
#define SEM_R 0

#define DEBUG_MSG_TO_M0
#define DEBUG_GET_FROM_MO
#define DEBUG_GET_FROM_APP



#define MSGSZ (sizeof(struct msg_element) - sizeof(long))

struct msg_element{
	long type;
	unsigned char msg[4];
};

struct pointer_for_M0_data{
	struct ShareMemeryData*pgetM0StructData;
	pthread_mutex_t*pmutex;
};

struct clientData{
	int client_fd;
	unsigned char data[MAXBUFF];
	int data_len;
	
};

struct ShareMemeryData{
	int temperature;
	int humidity;
	int illumination;
	int triaxial[3];
	int led1;
	int led2;
	int fan;
	int door;
	int flag;
};

union semun {
	int		val;	/* Value for SETVAL */
	struct semid_ds *buf;	/* Buffer for IPC_STAT, IPC_SET */
	unsigned short  *array;	/* Array for GETALL, SETALL */
	struct seminfo  *__buf;	/* Buffer for IPC_INFO
							   (Linux-specific) */
};


void do_something(void*);
void get_M0_data(void*);
void *read_M0(void*);
void *tackle_msg(void*);


int register_user(char*json_data,int json_len);
int login_usr(char*json_data,int json_len,char* puserToken);
int get_password(char*json_data,int json_len,char* passWord);
int modify_password(char*json_data,int json_len);
int get_temperature(char*json_data,int json_len);
int get_humidity(char*json_data,int json_len);
int get_illumination(char*json_data,int json_len);
int get_AIXS(char*json_data,float*ptriaxialData);
int set_led(char*json_data,int*pdev_stata,int*pdev_num);
int set_fan(char*json_data,int*pdev_stata,int*pdev_num);
int set_door(char*json_data,int*pdev_stata,int*pdev_num);
int get_video(char*json_data,int json_len);
struct ShareMemeryData get_device_state(char*json_data,int json_len);

int get_int_json_member(json_object*obj,const char*filed);
char* get_string_json_member(json_object*obj,const char*filed,char*strdata);


int print_table(void *NotUsed, int argc, char **argv, char **azColName);
int get_max_user_ID(void *pmax_token, int argc, char **argv, char **azColName);
int is_user_name_exist(void *pbool_user_name, int argc, char **argv, char **azColName);
int get_user_token(void *puserToken, int argc, char **argv, char **azColName);
int return_user_password(void *puserToken, int argc, char **argv, char **azColName);
unsigned char* make_msg(unsigned char *,unsigned char dev_type,unsigned char deviceNumber,unsigned char deviceCode);
char * make_json_str(json_object*json_obj_app,int stateCode,char*stateMsg);

int V(int semid, int semnum);
int P(int semid, int semnum);





#endif /*SERVER_H*/


