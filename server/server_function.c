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
#ifdef DEBUG2
	pgetM0StructData->temperature = 12;
	pgetM0StructData->humidity = 13;
	pgetM0StructData->illumination = 14;
	pgetM0StructData->triaxial[0] = 15;
	pgetM0StructData->triaxial[1] = 16;
	pgetM0StructData->triaxial[2]  = 17;
	pgetM0StructData->led1 = 0;
	pgetM0StructData->led2 = 0;
	pgetM0StructData->fan = 0;
	pgetM0StructData->door = 0;
	pgetM0StructData->flag = 1;
#endif 
#ifdef DEBUG2
	printf("get_M0_data:temperature %d\n",pgetM0StructData->temperature);
#endif 
	while(1){
		ret = P(semid,SEM_R);
#ifdef DEBUG2
	printf("ret = P(semid,SEM_R);");
#endif
		if(-1 == ret){
			fprintf(stderr,"get_M0_data p fail");
			break;
		}
		ret = pthread_mutex_lock(pmutex);
		if( 0 != ret){
			fprintf(stderr,"pthread_mutex_lock fail");
			break;
		}
		memcpy(pgetM0StructData,pshareMemeryData,sizeof(struct ShareMemeryData));
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
	struct msg_element msg_ele;
	while(1){
		ret = msgrcv(msgid, &msg_ele, MSGSZ, 1,0);
		if(-1 == ret){
			perror("msgrcv fail");
			return ((void*)(-1));
		}

		for(i = 3; i >= 0;i--){
#ifdef DEBUG2
			fprintf(stderr,"msg[%d] = %x\n",i,(int)(((char*)&msg_ele.msg)+i));
#endif
			ret = uart_send(*((int*)puart_fd), (((char*)&msg_ele.msg)+i),1);
			if(-1 == ret){
				perror("puart_fd fail");
				return ((void*)(-1));
			}
		}

		
	}

	return NULL;
}

void* read_M0(void *p){
	int ret = 0;
	int uart_fd = *((int*)p);
	while(1){
		ret = P(semid,SEM_W);
		if(-1 == ret){
			fprintf(stderr,"read_M0 P fail");
			break;
		}
		ret = uart_recv(uart_fd, (char*)pshareMemeryData,sizeof(struct ShareMemeryData),NULL);
		if(-1 == ret){
			fprintf(stderr,"read_M0,uart_recv fail");
			break;
		}
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
	printf("print_table\n");
	int i;
	for(i=0; i < argc; i++){
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

int get_max_user_ID(void *pmax_token, int argc, char **argv, char **azColName){
	printf("get_max_user_ID\n");
	int tmp = atoi(argv[0]? argv[0]:"0");
	if(tmp > *((int*)pmax_token)){
		*((int *) pmax_token) = tmp;
	}
	return 0;
}
int is_user_name_exist(void *pbool_user_name, int argc, char **argv, char **azColName){
	printf("is_user_name_exist\n");
//	if(argc > 0){
//	*((int *)pbool_user_name) = 1;
//	}else{
//	*((int *)pbool_user_name) = 0;
//	}

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
#ifdef DEBUG
	printf("0xff = %x\n",(int)msg[0]);
	printf("fun_num = %d\n",(int)msg[1]);
	printf("json_msg = %s\n",srcmsg);
	printf("json_len = %d\n",*((short*)&(msg[2])));
#endif
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



void do_something(void* p){
	struct clientData* pclientdata = (struct clientData*)p;
#ifdef DEBUG
	printf("--------------2---------------\n");
	printf("rch[0]:%x\n",pclientdata->data[0]);
	printf("rch[1]:%x\n",pclientdata->data[1]);
	printf("rch[2,3]:%d\n",*((short*)(&pclientdata->data[2])));
	printf("rchdata:%s\n",&pclientdata->data[4]);
#endif
	unsigned char json_data[MAXBUFF];
	int ret = 0;
	json_object *json_obj_app = json_object_new_object();
	char*json_obj_app_str;
	char userToken[10] = {0};
	char password[25] = {0};
	float triaxial[3];
	int dev_stata = 0x00;
	int dev_num = 0x00;
	struct ShareMemeryData retShareMemery;


	short json_len = *((short*)(&pclientdata->data[2]));
	strncpy(json_data,(pclientdata->data + 4),strlen(&pclientdata->data[4]) + 1);
	memset(pclientdata->data + 4,0,1020);
#ifdef DEBUG
	printf("--------------3-------------\n");
	printf("rch[2,3]:%d\n",json_len);
	printf("rchdata:%s\n",json_data);
#endif

	if(0xaa != pclientdata->data[0]){
		printf("wrong data type\n");
		return;
	}
	switch(pclientdata->data[1]){
		case EVENT_REGISTER:
			ret = register_user(json_data,json_len);
			if(-2 == ret ){
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"user name exist");
			}else if(0 == ret){
				json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"register success");
			}
			send_msg_to_app(EVENT_REGISTER,json_obj_app_str,pclientdata->client_fd);
			break;
		case EVENT_LOGIN:
			ret = login_usr(json_data,json_len,userToken);
			if( 0 == ret ){
				json_object_object_add(json_obj_app,"userToken",json_object_new_string(userToken));
				json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"login success");
			}else if (-1 == ret){
				json_obj_app_str = make_json_str(json_obj_app,STATE_USER_PASSWORD_WRONG,"login fail,wrong name or password");
			}
			send_msg_to_app(EVENT_LOGIN,json_obj_app_str,pclientdata->client_fd);
			break;
		case EVENT_FORGET_PASSWORD:
			ret = get_password(json_data,json_len,password);
			if(0 == ret){
				json_object_object_add(json_obj_app,"password",json_object_new_string(password));
				json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"get success");
			}else{
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"get fail");
			}
			send_msg_to_app(EVENT_FORGET_PASSWORD,json_obj_app_str,pclientdata->client_fd);
			break;
		case EVENT_MODIFY_PASSWORD:
			ret = modify_password(json_data,json_len);
			if(0 == ret){
				json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"modify success");
			}else{
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"modify fail");
			}	
			send_msg_to_app(EVENT_MODIFY_PASSWORD,json_obj_app_str,pclientdata->client_fd);
			break;
		case EVENT_GET_TEMP:
#ifdef DEBUG2
			printf("EVENT_GET_TEMP\n");
#endif 
			ret = get_temperature(json_data,json_len);
#ifdef DEBUG2
			printf("return get_temperature");
#endif
			if(-1 != ret){
				json_object_object_add(json_obj_app,"temperature",json_object_new_int(ret));
				json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"get_temperature success");
			}else{
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"get_temperature fail");
			}
			send_msg_to_app(EVENT_GET_TEMP,json_obj_app_str,pclientdata->client_fd);
			break;
		case EVENT_GET_HUMIDITY:
			ret = get_humidity(json_data,json_len);
			if(-1 != ret){
				json_object_object_add(json_obj_app,"humidity",json_object_new_int(ret));
				json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"get humidity success");
			}else{
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"get humidity fail");
			}
			send_msg_to_app(EVENT_GET_HUMIDITY,json_obj_app_str,pclientdata->client_fd);

			break;
		case EVENT_GET_ILLUMINATION:
			ret = get_illumination(json_data,json_len);
			if(-1 != ret){
				json_object_object_add(json_obj_app,"light",json_object_new_int(ret));
				json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"get illumination success");
			}else{
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"get illumination fail");
			}
			send_msg_to_app(EVENT_GET_ILLUMINATION,json_obj_app_str,pclientdata->client_fd);

			break;
		case EVENT_GET_AIXS:
			ret = get_AIXS(json_data,triaxial);
			if(-1 != ret){
				json_object_object_add(json_obj_app,"triaxialX",json_object_new_double(triaxial[0]));
				json_object_object_add(json_obj_app,"triaxialY",json_object_new_double(triaxial[1]));
				json_object_object_add(json_obj_app,"triaxialZ",json_object_new_double(triaxial[2]));
				json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"get triaxial success");
			}else{
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"get triaxial fail");
			}
			send_msg_to_app(EVENT_GET_AIXS,json_obj_app_str,pclientdata->client_fd);
			break;
		case EVENT_LED:
			ret = set_led(json_data,&dev_stata,&dev_num);
			if(0 == ret){
				int ret1 = 0;
				ret1 = check_M0_data(FM0Data,&dev_stata,&dev_num,EVENT_LED);
				if(0 == ret1){
					json_object_object_add(json_obj_app,"deviceState",json_object_new_int(dev_stata));
					json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"set led success");
				}else{
					json_object_object_add(json_obj_app,"deviceState",json_object_new_int(-1));
					json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"time out");
				}
				send_msg_to_app(EVENT_LED,json_obj_app_str,pclientdata->client_fd);
			}else{
				json_object_object_add(json_obj_app,"deviceState",json_object_new_int(-1));
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"server fail");
				fprintf(stderr,"send msg to queue fail");
				send_msg_to_app(EVENT_LED,json_obj_app_str,pclientdata->client_fd);
			}
			break;
		case EVENT_FAN:
			ret = set_fan(json_data,&dev_stata,&dev_num);
			if(0 == ret){
				int ret1 = 0;
				ret1 = check_M0_data(FM0Data,&dev_stata,&dev_num,EVENT_FAN);
				if(0 == ret1){
					json_object_object_add(json_obj_app,"deviceState",json_object_new_int(dev_stata));
					json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"set fan success");
				}else{
					json_object_object_add(json_obj_app,"deviceState",json_object_new_int(-1));
					json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"set fan fail");
				}
				send_msg_to_app(EVENT_FAN,json_obj_app_str,pclientdata->client_fd);
			}else{
				json_object_object_add(json_obj_app,"deviceState",json_object_new_int(1));
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"server fail");
				fprintf(stderr,"send msg to queue fail");
				send_msg_to_app(EVENT_FAN,json_obj_app_str,pclientdata->client_fd);
			}

			break;
		case EVENT_DOOR:
			ret = set_door(json_data,&dev_stata,&dev_num);
			if(0 == ret){
				int ret1 = 0;
				ret1 = check_M0_data(FM0Data,&dev_stata,&dev_num,EVENT_DOOR);
				if(0 == ret1){
					json_object_object_add(json_obj_app,"deviceState",json_object_new_int(dev_stata));
					json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"set door success");
				}else{
					json_object_object_add(json_obj_app,"deviceState",json_object_new_int(-1));
					json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"set door fail");
				}
				send_msg_to_app(EVENT_DOOR,json_obj_app_str,pclientdata->client_fd);
			}else{
				json_object_object_add(json_obj_app,"deviceState",json_object_new_int(1));
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"server fail");
				fprintf(stderr,"send msg to queue fail");
				send_msg_to_app(EVENT_DOOR,json_obj_app_str,pclientdata->client_fd);
			}

			break;
		case EVENT_GET_VIDEO:
			ret = get_video(json_data,json_len);

			break;
		case EVENT_GET_DEVICE_STATE:
			retShareMemery = get_device_state(json_data,json_len);
			if(0 == retShareMemery.flag){
				json_object_object_add(json_obj_app,"temperature",json_object_new_int(retShareMemery.temperature));
				json_object_object_add(json_obj_app,"humidity",json_object_new_int(retShareMemery.humidity));
				json_object_object_add(json_obj_app,"illumination",json_object_new_int(retShareMemery.illumination));
				json_object_object_add(json_obj_app,"triaxialX",json_object_new_double(retShareMemery.triaxial[0]));
				json_object_object_add(json_obj_app,"triaxialY",json_object_new_double(retShareMemery.triaxial[1]));
				json_object_object_add(json_obj_app,"triaxialZ",json_object_new_double(retShareMemery.triaxial[2]));
				json_object_object_add(json_obj_app,"led1",json_object_new_int(retShareMemery.led1));
				json_object_object_add(json_obj_app,"led2",json_object_new_int(retShareMemery.led2));
				json_object_object_add(json_obj_app,"fan",json_object_new_int(retShareMemery.fan));
				json_object_object_add(json_obj_app,"door",json_object_new_int(retShareMemery.door));
				json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"get stata success");
			}else{
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"get stata fail");
			}
			send_msg_to_app(EVENT_GET_DEVICE_STATE,json_obj_app_str,pclientdata->client_fd);

			break;
		default:
			printf("wrong function number");
			
	}

	json_object_put(json_obj_app);
	free(pclientdata);
}

int check_M0_data(struct pointer_for_M0_data FM0Data,int*pdev_stata,int*pdev_num,int event){
	int i = 10;
	struct ShareMemeryData *pM0Data = FM0Data.pgetM0StructData;
#ifdef DEBUG2
	fprintf(stderr,"in check_M0_data\n");
#endif
	while(i--){
		pthread_mutex_lock(FM0Data.pmutex);
		switch(event){
		case EVENT_LED:
#ifdef DEBUG2
			fprintf(stderr,"in check_M0_data:EVENT_LED\n");
#endif
			if((pM0Data->flag == 1)&&((*(&pM0Data->led1+*pdev_num)) == *pdev_stata)){
				pthread_mutex_unlock(FM0Data.pmutex);
				return 0;
			}
			break;
		case EVENT_FAN:
			if((pM0Data->flag == 1)&&((*(&pM0Data->fan)) == *pdev_stata)){
				pthread_mutex_unlock(FM0Data.pmutex);
				return 0;
			}
			break;
		case EVENT_DOOR:
			if((pM0Data->flag == 1)&&((*(&pM0Data->door)) == *pdev_stata)){
				pthread_mutex_unlock(FM0Data.pmutex);
				return 0;
			}
			break;
		default:
			pthread_mutex_unlock(FM0Data.pmutex);
			return -1;
		}
		pthread_mutex_unlock(FM0Data.pmutex);
		sleep(1);
	}
	return -1;
}

int set_fan(char*json_data, int *pdev_stata,int *pdev_num){
	char user_token[10] = {0};
	int ret = 0;
	char user_name[25] = {0};
	int pbool_user_name = 0;
	char sql_user_info[1024] = {0};
	int deviceNumber = 0;
	int deviceCode = 0;
	struct msg_element msg_ele;
	int msg = 0;
	int temp = 0;

	json_object*json_obj_data = NULL;
	json_obj_data = json_tokener_parse(json_data);
	get_string_json_member(json_obj_data,"userName",user_name);
	get_string_json_member(json_obj_data,"userToken",user_token);
	deviceNumber = get_int_json_member(json_obj_data,"deviceNumber");
	deviceCode = get_int_json_member(json_obj_data,"deviceCode");
	*pdev_stata = deviceCode;
	*pdev_num = deviceNumber;

	msg_ele.type = 1;
	msg_ele.msg = make_msg(DEV_FAN,deviceNumber,deviceCode);
#ifdef DEBUG1
	unsigned char*p = (unsigned char*)&msg_ele.msg;
	printf("-----------msg_ele--------\n");
	printf("type = %x\n",p[3]);
	printf("dev = %x\n",p[2]);
	printf("deviceNumber = %x\n",p[1]);
	printf("deviceCode = %x\n",p[0]);
#endif

	//查看用户名是否存在
	sprintf(sql_user_info,"select * FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\" AND USER_TOKEN=\"%s\"",user_name,user_token);
	ret = sqlite3_exec(db_smart_home, sql_user_info, is_user_name_exist, &pbool_user_name, &sqlite_err_msg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_data);
		return -1;
	}

	if(pbool_user_name){
		ret = msgsnd(msgid, &msg_ele, MSGSZ, 0);
		if(-1 == ret){
			fprintf(stderr,"msgsnd fail\n");
			return ret;
		}
		json_object_put(json_obj_data);
		return ret;
	}
	return -2;
}
int set_door(char*json_data,int*pdev_stata,int*pdev_num){
	char user_token[10] = {0};
	int ret = 0;
	char user_name[25] = {0};
	int pbool_user_name = 0;
	char sql_user_info[1024] = {0};
	int deviceNumber = 0;
	int deviceCode = 0;
	struct msg_element msg_ele;
	int msg = 0;
	int temp = 0;

	json_object*json_obj_data = NULL;
	json_obj_data = json_tokener_parse(json_data);
	get_string_json_member(json_obj_data,"userName",user_name);
	get_string_json_member(json_obj_data,"userToken",user_token);
	deviceNumber = get_int_json_member(json_obj_data,"deviceNumber");
	deviceCode = get_int_json_member(json_obj_data,"deviceCode");
	*pdev_stata = deviceCode;
	*pdev_num = deviceNumber;

	msg_ele.type = 1;
	msg_ele.msg = make_msg(DEV_DOOR,deviceNumber,deviceCode);
#ifdef DEBUG1
	unsigned char*p = (unsigned char*)&msg_ele.msg;
	printf("-----------msg_ele--------\n");
	printf("type = %x\n",p[3]);
	printf("dev = %x\n",p[2]);
	printf("deviceNumber = %x\n",p[1]);
	printf("deviceCode = %x\n",p[0]);
#endif

	//查看用户名是否存在
	sprintf(sql_user_info,"select * FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\" AND USER_TOKEN=\"%s\"",user_name,user_token);
	ret = sqlite3_exec(db_smart_home, sql_user_info, is_user_name_exist, &pbool_user_name, &sqlite_err_msg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_data);
		return -1;
	}

	if(pbool_user_name){
		ret = msgsnd(msgid, &msg_ele, MSGSZ, 0);
		if(-1 == ret){
			fprintf(stderr,"msgsnd fail\n");
			return ret;
		}
		json_object_put(json_obj_data);
		return ret;
	}
	return -2;

	return 0;
}
int get_video(char*json_data,int json_len){

	return 0;
}
struct ShareMemeryData get_device_state(char*json_data,int json_len){
	char user_token[10] = {0};
	int ret = 0;
	char user_name[25] = {0};
	int pbool_user_name = 0;
	char sql_user_info[1024] = {0};
	struct ShareMemeryData *pM0Data = FM0Data.pgetM0StructData;
	struct ShareMemeryData temp;


	json_object*json_obj_data = NULL;
	json_obj_data = json_tokener_parse(json_data);
	get_string_json_member(json_obj_data,"userName",user_name);
	get_string_json_member(json_obj_data,"userToken",user_token);

	//查看用户名是否存在
	sprintf(sql_user_info,"select * FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\" AND USER_TOKEN=\"%s\"",user_name,user_token);
	ret = sqlite3_exec(db_smart_home, sql_user_info, is_user_name_exist, &pbool_user_name, &sqlite_err_msg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_data);
		temp.flag = -1;
		return temp;
	}

	if(pbool_user_name){
		pthread_mutex_lock(FM0Data.pmutex);
		temp.temperature = pM0Data->temperature;
		temp.humidity = pM0Data->humidity;
		temp.illumination = pM0Data->humidity;
		temp.triaxial[0] = pM0Data->triaxial[0];
		temp.triaxial[1] = pM0Data->triaxial[1];
		temp.triaxial[2] = pM0Data->triaxial[2];
		temp.led1 = pM0Data->led1;
		temp.led2 = pM0Data->led2;
		temp.fan = pM0Data->fan;
		temp.door = pM0Data->door;
		temp.flag = 0;
		pthread_mutex_unlock(FM0Data.pmutex);
		json_object_put(json_obj_data);
		return temp; 
	}
	json_object_put(json_obj_data);
	temp.flag = -1;
	return temp;
}

int make_msg(int dev_type,int deviceNumber,int deviceCode){
	int msg = 0;
	int temp = 0;
	temp = FSTM0;
	msg = (temp << 24);
	temp = dev_type;
	msg |= (temp << 16);
	temp = deviceNumber;
	msg |= (temp << 8);
	temp = deviceCode;
	msg |= temp;

	return msg;
}

int set_led(char*json_data,int*pdev_stata,int*pdev_num){
	char user_token[10] = {0};
	int ret = 0;
	char user_name[25] = {0};
	int pbool_user_name = 0;
	char sql_user_info[1024] = {0};
	int deviceNumber = 0;
	int deviceCode = 0;
	struct msg_element msg_ele;
	int msg = 0;
	int temp = 0;

	json_object*json_obj_data = NULL;
	json_obj_data = json_tokener_parse(json_data);
	get_string_json_member(json_obj_data,"userName",user_name);
	get_string_json_member(json_obj_data,"userToken",user_token);
	deviceNumber = get_int_json_member(json_obj_data,"deviceNumber");
	deviceCode = get_int_json_member(json_obj_data,"deviceCode");
	*pdev_stata = deviceCode;
	*pdev_num = deviceNumber;

	msg_ele.type = 1;
	msg_ele.msg = make_msg(DEV_LED,deviceNumber,deviceCode);
#ifdef DEBUG1
	unsigned char*p = (unsigned char*)&msg_ele.msg;
	printf("-----------msg_ele--------\n");
	printf("type = %x\n",p[3]);
	printf("dev = %x\n",p[2]);
	printf("deviceNumber = %x\n",p[1]);
	printf("deviceCode = %x\n",p[0]);
	printf("user_name = %s\n",user_name);
	printf("user_token = %s\n",user_token);
#endif

	//查看用户名是否存在
	sprintf(sql_user_info,"select * FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\" AND USER_TOKEN=\"%s\"",user_name,user_token);
	ret = sqlite3_exec(db_smart_home, sql_user_info, is_user_name_exist, &pbool_user_name, &sqlite_err_msg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_data);
		return -1;
	}
#ifdef DEBUG2
		fprintf(stderr,"set_led:ret1=%d\n",ret);
		fprintf(stderr,"set_led:pbool_user_name=%d\n",pbool_user_name);
#endif 

	if(pbool_user_name){
		ret = msgsnd(msgid, &msg_ele, MSGSZ, 0);
		if(-1 == ret){
			fprintf(stderr,"msgsnd fail\n");
			return ret;
		}
		json_object_put(json_obj_data);
#ifdef DEBUG2
		fprintf(stderr,"set_led:ret2=%d\n",ret);
#endif 
		return ret;
	}
	return -2;
}

int get_temperature(char*json_data,int json_len){
	char user_token[10] = {0};
	int ret = 0;
	char user_name[25] = {0};
	int pbool_user_name = 0;
	char sql_user_info[1024] = {0};
	struct ShareMemeryData *pM0Data = FM0Data.pgetM0StructData;


	json_object*json_obj_data = NULL;
	json_obj_data = json_tokener_parse(json_data);
	get_string_json_member(json_obj_data,"userName",user_name);
	get_string_json_member(json_obj_data,"userToken",user_token);

	//查看用户名是否存在
	sprintf(sql_user_info,"select * FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\" AND USER_TOKEN=\"%s\"",user_name,user_token);
	ret = sqlite3_exec(db_smart_home, sql_user_info, is_user_name_exist, &pbool_user_name, &sqlite_err_msg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_data);
		return -1;
	}
#ifdef DEBUG2
	printf("pbool_user_name = %d\n",pbool_user_name);
#endif

	if(pbool_user_name){
		int temp = 12;
//		return temp;
		pthread_mutex_lock(FM0Data.pmutex);
#ifdef DEBUG2
		fprintf(stderr,"pthread_mutex_lock(FM0Data.pmutex);\n");
#endif
		temp = pM0Data->temperature;
		pthread_mutex_unlock(FM0Data.pmutex);
		json_object_put(json_obj_data);
		return temp; 
	}
	json_object_put(json_obj_data);
	return -1;
}
int get_humidity(char*json_data,int json_len){
	char user_token[10] = {0};
	int ret = 0;
	char user_name[25] = {0};
	int pbool_user_name = 0;
	char sql_user_info[1024] = {0};
	struct ShareMemeryData *pM0Data = FM0Data.pgetM0StructData;

	json_object*json_obj_data = NULL;
	json_obj_data = json_tokener_parse(json_data);
	get_string_json_member(json_obj_data,"userName",user_name);
 	get_string_json_member(json_obj_data,"userToken",user_token);

	//查看用户名是否存在
	sprintf(sql_user_info,"select * FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\" AND USER_TOKEN=\"%s\"",user_name,user_token);
	ret = sqlite3_exec(db_smart_home, sql_user_info, is_user_name_exist, &pbool_user_name, &sqlite_err_msg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_data);
		return -1;
	}

	if(pbool_user_name){
		int humidity = 0;
		pthread_mutex_lock(FM0Data.pmutex);
		humidity = pM0Data->humidity;
		pthread_mutex_unlock(FM0Data.pmutex);
		json_object_put(json_obj_data);
		return humidity; 
	}
	json_object_put(json_obj_data);
	return -1;
}
int get_illumination(char*json_data,int json_len){
	char user_token[10] = {0};
	int ret = 0;
	char user_name[25] = {0};
	int pbool_user_name = 0;
	char sql_user_info[1024] = {0};
	struct ShareMemeryData *pM0Data = FM0Data.pgetM0StructData;

	json_object*json_obj_data = NULL;
	json_obj_data = json_tokener_parse(json_data);
	get_string_json_member(json_obj_data,"userName",user_name);
	get_string_json_member(json_obj_data,"userToken",user_token);

	//查看用户名是否存在
	sprintf(sql_user_info,"select * FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\" AND USER_TOKEN=\"%s\"",user_name,user_token);
	ret = sqlite3_exec(db_smart_home, sql_user_info, is_user_name_exist, &pbool_user_name, &sqlite_err_msg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_data);
		return -1;
	}

	if(pbool_user_name){
		int illumination = 0;
		pthread_mutex_lock(FM0Data.pmutex);
		illumination = pM0Data->illumination;
		pthread_mutex_unlock(FM0Data.pmutex);
		json_object_put(json_obj_data);
		return illumination; 
	}
	json_object_put(json_obj_data);
	return -1;
}
int get_AIXS(char*json_data,float* ptriaxialData){
	char user_token[10] = {0};
	int ret = 0;
	char user_name[25] = {0};
	int pbool_user_name = 0;
	char sql_user_info[1024] = {0};
	struct ShareMemeryData *pM0Data = FM0Data.pgetM0StructData;

	json_object*json_obj_data = NULL;
	json_obj_data = json_tokener_parse(json_data);
	get_string_json_member(json_obj_data,"userName",user_name);
	get_string_json_member(json_obj_data,"userToken",user_token);

	//查看用户名是否存在
	sprintf(sql_user_info,"select * FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\" AND USER_TOKEN=\"%s\"",user_name,user_token);
	ret = sqlite3_exec(db_smart_home, sql_user_info, is_user_name_exist, &pbool_user_name, &sqlite_err_msg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_data);
		return -1;
	}

	if(pbool_user_name){
		pthread_mutex_lock(FM0Data.pmutex);
		ptriaxialData[0] = pM0Data->triaxial[0];
		ptriaxialData[1] = pM0Data->triaxial[1];
		ptriaxialData[2] = pM0Data->triaxial[2];
		pthread_mutex_unlock(FM0Data.pmutex);
		json_object_put(json_obj_data);
		return 0;
	}
	json_object_put(json_obj_data);
	return -1;
}

int get_password(char*json_data,int json_len,char* password){
	char user_name[25];
	char user_phone[15];
	char sql_user_info[1024] = {0};
	int ret = 0;
	int user_token = 0;
	int pbool_user_name = 0;

	json_object*json_obj_data = NULL;
	json_obj_data = json_tokener_parse(json_data);
	
	get_string_json_member(json_obj_data,"userName",user_name);
	get_string_json_member(json_obj_data,"phoneNumber",user_phone);
	
	//查看用户名是否已经存在
	sprintf(sql_user_info,"select * FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\" AND USER_PHONE=\"%s\"",user_name,user_phone);
	ret = sqlite3_exec(db_smart_home, sql_user_info, is_user_name_exist, &pbool_user_name, &sqlite_err_msg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_data);
		return -1;
	}

	if(pbool_user_name){
		sprintf(sql_user_info,"select PASSWORD FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\" AND USER_PHONE=\"%s\"",user_name,user_phone);
		ret = sqlite3_exec(db_smart_home, sql_user_info, return_user_password, password, &sqlite_err_msg);
		if(ret != SQLITE_OK){
			fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
			sqlite3_free(sqlite_err_msg);
			json_object_put(json_obj_data);
			return -1;
		}
		json_object_put(json_obj_data);
		return 0;
	}

}

int register_user(char * data,int size){
	char user_name[25] = {0};
	char user_password[25] = {0};
	char user_phone[15] = {0};
	char user_token[10] = {0};
	char sql_user_info[1024] = {0};
	int ret = 0;
	int ID_num = 0;
	int pbool_user_name = 0;

#ifdef DEBUG
	printf("----------4---------\n");
	printf("json_data: %s\n",data);
#endif

	json_object*json_obj_register_data = NULL;
	json_obj_register_data = json_tokener_parse(data);
	
	get_string_json_member(json_obj_register_data,"userName",user_name);
	get_string_json_member(json_obj_register_data,"password",user_password);
	get_string_json_member(json_obj_register_data,"phoneNumber",user_phone);
#ifdef DEBUG
	printf("----------5----------\n");
	printf("usr_name: %s\n",user_name);
	printf("usr_password: %s\n",user_password);
	printf("usr_phone: %s\n",user_phone);
#endif

	//查看用户名是否已经存在
	sprintf(sql_user_info,"select * FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\"",user_name);
	ret = sqlite3_exec(db_smart_home, sql_user_info, is_user_name_exist, &pbool_user_name, &sqlite_err_msg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_register_data);
		return -1;
	}

	if(pbool_user_name){
		fprintf(stderr,"NAME EXIST\n");
		json_object_put(json_obj_register_data);
		return -2;
	}
	//获取最大ID
	sprintf(sql_user_info,"select ID FROM USER_INFO_TABLE ORDER BY ID DESC LIMIT 1");
	ret = sqlite3_exec(db_smart_home,sql_user_info,get_max_user_ID,&ID_num,&sqlite_err_msg);
	if( ret != SQLITE_OK ){
		fprintf(stderr, "SQL error(获取最大user_token fail): %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		ID_num = 0;
	}

	//将用户信息保存到数据库
#ifdef DEBUG
	fprintf(stderr,"Max ID %d\n",ID_num);
#endif
	strncpy(user_token,user_name,4);
	strncpy((user_token + 4),user_password,4);
	user_token[8] = 0x00;
	sprintf(sql_user_info,
			"INSERT INTO USER_INFO_TABLE(ID,USER_TOKEN,USER_NAME,USER_PHONE,PASSWORD)  VALUES(%d,\"%s\",\"%s\",\"%s\",\"%s\");"
			,++ID_num,user_token,user_name,user_phone,user_password);
	ret = sqlite3_exec(db_smart_home, sql_user_info
						, NULL, NULL, &sqlite_err_msg);
	if( ret != SQLITE_OK ){
		fprintf(stderr, "SQL error(插入数据): %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_register_data);
		return -3;
	}
	sprintf(sql_user_info,
			"select * from USER_INFO_TABLE ORDER BY USER_TOKEN LIMIT 1"
			,user_token,user_name,user_phone,user_password);
	ret = sqlite3_exec(db_smart_home, sql_user_info
						, print_table, NULL, &sqlite_err_msg);
	if( ret != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_register_data);
		return -4;
	}
	json_object_put(json_obj_register_data);
	return 0;
}

int login_usr(char*json_data,int json_len,char* puserToken){
	char user_name[25];
	char user_password[25];
	int pbool_user_name = 0;
	char sql_user_info[1024];
	int ret = 0;

	json_object*json_obj_login_data = NULL;
	json_obj_login_data = json_tokener_parse(json_data);

	get_string_json_member(json_obj_login_data,"userName",user_name);
	get_string_json_member(json_obj_login_data,"password",user_password);
	
	//查看用户名是否已经存在
	sprintf(sql_user_info,"select * FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\" AND PASSWORD=\"%s\"",user_name,user_password);
	ret = sqlite3_exec(db_smart_home, sql_user_info, is_user_name_exist, &pbool_user_name, &sqlite_err_msg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_login_data);
		return -1;
	}

	if(pbool_user_name){
		sprintf(sql_user_info,"select USER_TOKEN FROM USER_INFO_TABLE WHERE USER_NAME=\"%s\" AND PASSWORD=\"%s\"",user_name,user_password);
		ret = sqlite3_exec(db_smart_home, sql_user_info, get_user_token, puserToken, &sqlite_err_msg);
		if(ret != SQLITE_OK){
			fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
			sqlite3_free(sqlite_err_msg);
			json_object_put(json_obj_login_data);
			return -1;
		}
		json_object_put(json_obj_login_data);
		return 0;
	}
	json_object_put(json_obj_login_data);

	return -1;
}
int modify_password(char*json_data,int json_len){

	char user_token[10];
	char user_old_password[25];
	char user_new_password[25];
	char sql_user_info[1024];
	int ret = 0;

	json_object*json_obj_modify_password_data = NULL;
	json_obj_modify_password_data = json_tokener_parse(json_data);

	get_string_json_member(json_obj_modify_password_data,"userToken",user_token);
	get_string_json_member(json_obj_modify_password_data,"oldPassword",user_old_password);
	get_string_json_member(json_obj_modify_password_data,"newPassword",user_new_password);
	
	
	sprintf(sql_user_info,"UPDATE USER_INFO_TABLE SET PASSWORD=\"%s\" WHERE USER_TOKEN=\"%s\""
			,user_new_password,user_token);
	ret = sqlite3_exec(db_smart_home, sql_user_info, print_table, NULL, &sqlite_err_msg);
	if(ret != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		json_object_put(json_obj_modify_password_data);
		return -1;
	}
	json_object_put(json_obj_modify_password_data);
	return 0;
}


