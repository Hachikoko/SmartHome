#include "server.h"

extern sqlite3* db_smart_home;
extern char* sqlite_err_msg ;
extern int shmemfd;
extern struct ShareMemeryData *pshareMemeryData;
extern struct pointer_for_M0_data FM0Data;
extern int semid;
extern int msgid;

void get_M0_data(void*p){
	struct pointer_for_M0_data *pdata = ((struct pointer_for_M0_data*)p);
	struct ShareMemeryData *pgetM0StructData = pdata->pgetM0StructData;
	pthread_mutex_t*pmutex = pdata->pmutex;
	int ret = 0;
	while(1){
		ret = P(semid,SEM_R);
		if(-1 == ret){
			fprintf(stderr,"get_M0_data p fail");
			break;
		}
		ret = pthread_mutex_lock(pmutex);
		if( 0 != ret){
			fprintf(stderr,"pthread_mutex_lock fail");
			break;
		}
		pgetM0StructData->temperature = pshareMemeryData->temperature;
		pgetM0StructData->humidity = pshareMemeryData->humidity;
		pgetM0StructData->illumination = pshareMemeryData->illumination;
		pgetM0StructData->led1 = pshareMemeryData->led1;
		pgetM0StructData->led2 = pshareMemeryData->led2;
		pgetM0StructData->fan = pshareMemeryData->fan;
		pgetM0StructData->door = pshareMemeryData->door;
		pgetM0StructData->flag = pshareMemeryData->flag;
#ifdef DEBUG_GET_FROM_MO
		printf("---------- get_M0_data(void*p)----------\n");
		printf("gtemperature:%d\n",pgetM0StructData->temperature);
		printf("ghumidity:%d\n",pgetM0StructData->humidity);
		printf("gillumination:%d\n",pgetM0StructData->illumination);
		printf("gflag:%d\n",pgetM0StructData->flag);
#endif
		ret = pthread_mutex_unlock(pmutex);	
		if( 0 != ret){
			fprintf(stderr,"pthread_mutex_unlock fail");
			break;
		}
		ret = V(semid,SEM_W);
		if(-1 == ret){
			fprintf(stderr,"get_M0_data V fail");
			break;
		}
	}
	return;
}


void *tackle_msg(void*puart_fd){
	int ret = 0;
	int i = 0;
	int uart_fd = *((int*)puart_fd);
	struct msg_element msg_ele;
	while(1){
		ret = msgrcv(msgid, &msg_ele, MSGSZ, 1,0);
		if(-1 == ret){
			perror("msgrcv fail");
			return ((void*)(-1));
		}
#ifdef DEBUG_MSG_TO_M0
		printf("void *tackle_msg(void*puart_fd)\n");
		printf("uart_fd:%d\n",uart_fd);
		printf("msg[0]:%x\n",msg_ele.msg[0]);
		printf("msg[1]:%x\n",msg_ele.msg[1]);
		printf("msg[2]:%x\n",msg_ele.msg[2]);
		printf("msg[3]:%x\n",msg_ele.msg[3]);
#endif
		ret = uart_send(uart_fd,msg_ele.msg,4);
		if(-1 == ret){
			perror("puart_fd fail");
			return ((void*)(-1));
		}
	}

	return NULL;
}

void* read_M0(void *p){
	int ret = 0;
	int uart_fd = *((int*)p);
	char data[1024] = {0};
	json_object*json_obj_data = NULL;
	FILE *uart_stream = fdopen(uart_fd,"w+");
	while(1){
		bzero(data,1024);
		ret = P(semid,SEM_W);
		if(-1 == ret){
			fprintf(stderr,"read_M0 P fail");
			break;
		}
       if(NULL == fgets(data, 1024,uart_stream)){
	   		perror("fgets read_M0 fail");
			break;
	   }
	  	ret = fflush(uart_stream);
		if(EOF == ret){
			perror("fflush fail");
			break;
		}
#if 0
		ret = uart_recv(uart_fd, data,94,NULL);
		if(-1 == ret){
			fprintf(stderr,"read_M0,uart_recv fail");
			break;
		}
#endif
#ifdef DEBUG_GET_FROM_MO
		printf("uart_recv data:%s\n",data);
#endif
		json_obj_data = json_tokener_parse(data);
		pshareMemeryData->temperature = get_int_json_member(json_obj_data,"temperature");
		pshareMemeryData->humidity = get_int_json_member(json_obj_data,"humidity");
		pshareMemeryData->illumination = get_int_json_member(json_obj_data,"illumination");
		pshareMemeryData->led1 = get_int_json_member(json_obj_data,"led1");
		pshareMemeryData->led2 = get_int_json_member(json_obj_data,"led2");
		pshareMemeryData->fan = get_int_json_member(json_obj_data,"fan");
		pshareMemeryData->door = get_int_json_member(json_obj_data,"door");
		pshareMemeryData->flag = get_int_json_member(json_obj_data,"flag");
#ifdef DEBUG_GET_FROM_MO
		printf("---------read_M0(void *p)-----------\n");
		printf("stemperature:%d\n",pshareMemeryData->temperature);
		printf("shumidity:%d\n",pshareMemeryData->humidity);
		printf("sillumination:%d\n",pshareMemeryData->illumination);
		printf("sflag:%d\n",pshareMemeryData->flag);
#endif
		ret = V(semid,SEM_R);
		if(-1 == ret){
			fprintf(stderr,"read_M0 V fail");
			break;
		}

	}
	return NULL;
}

int V(int semid, int semnum){
	struct sembuf sem;
	sem.sem_num = semnum;
	sem.sem_op = 1;
	sem.sem_flg = SEM_UNDO;
	if(-1 == semop(semid,&sem,1)){
		perror("V fail");
		return -1;
	}
}

int P(int semid, int semnum){
	struct sembuf sem;
	sem.sem_num = semnum;
	sem.sem_op = -1;
	sem.sem_flg = SEM_UNDO;
	if(-1 == semop(semid,&sem,1)){
		perror("P fail");
		return -1;
	}
}

int print_table(void *NotUsed, int argc, char **argv, char **azColName){
	int i;
	for(i=0; i < argc; i++){
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

int get_max_user_ID(void *pmax_token, int argc, char **argv, char **azColName){
	int tmp = atoi(argv[0]? argv[0]:"0");
	if(tmp > *((int*)pmax_token)){
		*((int *) pmax_token) = tmp;
	}
	return 0;
}
int is_user_name_exist(void *pbool_user_name, int argc, char **argv, char **azColName){
	*((int *)pbool_user_name) = 1;

	return 0;
}

int get_user_token(void *puserToken, int argc, char **argv, char **azColName){
	strcpy((char*)puserToken,argv[0]);
	return 0;
}


int return_user_password(void *password, int argc, char **argv, char **azColName){
	strcpy(((char*)password),argv[0]);
	return 0;
}

int get_int_json_member(json_object*obj,const char*filed){
	struct json_object *member_object = json_object_object_get(obj,filed);

	return json_object_get_int(member_object);
}

char* get_string_json_member(json_object*obj,const char*filed,char*strdata){
	struct json_object *member_object = json_object_object_get(obj,filed);
	strcpy(strdata,json_object_get_string(member_object));

	return strdata;
}


int send_msg_to_app(unsigned char fun_num,char*srcmsg,int client_fd){
	unsigned char msg[1024] ={0};
	int ret = 0;

	msg[0] = 0xff;
	msg[1] = fun_num;
	short msglen = strlen(srcmsg) + 1;
	*((short*)&(msg[2])) = msglen;
	strncpy((msg + 4),srcmsg,msglen);
	ret = send(client_fd, msg, msglen + 4, 0);
	if(-1 == ret){
		perror("write fail");
		return ret;
	}
	close(client_fd);
	return 0;
}


char * make_json_str(json_object*json_obj_app,int stateCode,char*stateMsg){
	json_object_object_add(json_obj_app,"stateCode",json_object_new_int(stateCode));
	json_object_object_add(json_obj_app,"stateMsg",json_object_new_string(stateMsg));
	return (char*)json_object_to_json_string(json_obj_app);
}
int check_M0_data(struct pointer_for_M0_data FM0Data,int*pdev_stata,int*pdev_num,int event){
	int i = 5;
	struct ShareMemeryData *pM0Data = FM0Data.pgetM0StructData;
	while(i--){
		pthread_mutex_lock(FM0Data.pmutex);
		switch(event){
		case EVENT_LED:
			if((pM0Data->flag == 1)&&((*(&pM0Data->led1+(*pdev_num))) == *pdev_stata)){
				pthread_mutex_unlock(FM0Data.pmutex);
#ifdef DEBUG_GET_FROM_MO
				printf("check_M0_data OK\n");
#endif
				return 0;
			}
			break;
		case EVENT_FAN:
			if((pM0Data->flag == 1)&&((*(&pM0Data->fan)) == *pdev_stata)){
				pthread_mutex_unlock(FM0Data.pmutex);
#ifdef DEBUG_GET_FROM_MO
				printf("check_M0_data OK\n");
#endif
				return 0;
			}
			break;
		case EVENT_DOOR:
			if((pM0Data->flag == 1)&&((*(&pM0Data->door)) == *pdev_stata)){
				pthread_mutex_unlock(FM0Data.pmutex);
#ifdef DEBUG_GET_FROM_MO
				printf("check_M0_data OK\n");
#endif
				return 0;
			}
			break;
		default:
			pthread_mutex_unlock(FM0Data.pmutex);
#ifdef DEBUG_GET_FROM_MO
				printf("check_M0_data--->fail\n");
#endif
			return -1;
		}
		pthread_mutex_unlock(FM0Data.pmutex);
		sleep(1);
	}
	return -1;
}


unsigned char *make_msg(unsigned char *msg,unsigned char dev_type,unsigned char  deviceNumber,unsigned char deviceCode){
	msg[0] = FSTM0;
	msg[1] = dev_type;
	msg[2] = deviceNumber;
	msg[3] = deviceCode;
	return msg;
}






