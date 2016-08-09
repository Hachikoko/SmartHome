#include "server.h"
extern sqlite3* db_smart_home;
extern char* sqlite_err_msg ;
extern int shmemfd;
extern struct ShareMemeryData *pshareMemeryData;
extern struct pointer_for_M0_data FM0Data;
extern int semid;
extern int msgid;


void do_something(void* p){
	struct clientData* pclientdata = (struct clientData*)p;
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
#ifdef DEBUG_GET_FROM_APP
	printf("type:%x\n",pclientdata->data[0]);
	printf("fun_num:%x\n",pclientdata->data[1]);
	printf("json_data:%s\n",json_data);
#endif
	memset(pclientdata->data + 4,0,1020);
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
			ret = get_temperature(json_data,json_len);
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
#ifdef DEBUG_GET_FROM_MO
				printf("return to app data : %s\n",json_obj_app_str);
#endif
				send_msg_to_app(EVENT_LED,json_obj_app_str,pclientdata->client_fd);
			}else{
				json_object_object_add(json_obj_app,"deviceState",json_object_new_int(-1));
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"server fail");
				fprintf(stderr,"send msg to queue fail");
#ifdef DEBUG_GET_FROM_MO
				printf("return to app data : %s\n",json_obj_app_str);
#endif
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
#ifdef DEBUG_GET_FROM_MO
				printf("return to app data : %s\n",json_obj_app_str);
#endif
				send_msg_to_app(EVENT_FAN,json_obj_app_str,pclientdata->client_fd);
			}else{
				json_object_object_add(json_obj_app,"deviceState",json_object_new_int(1));
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"server fail");
				fprintf(stderr,"send msg to queue fail");
#ifdef DEBUG_GET_FROM_MO
				printf("return to app data : %s\n",json_obj_app_str);
#endif
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
#ifdef DEBUG_GET_FROM_MO
				printf("return to app data : %s\n",json_obj_app_str);
#endif
				send_msg_to_app(EVENT_DOOR,json_obj_app_str,pclientdata->client_fd);
			}else{
				json_object_object_add(json_obj_app,"deviceState",json_object_new_int(1));
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"server fail");
				fprintf(stderr,"send msg to queue fail");
#ifdef DEBUG_GET_FROM_MO
				printf("return to app data : %s\n",json_obj_app_str);
#endif
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
				json_obj_app_str = make_json_str(json_obj_app,STATE_OK,"get stata success");
			}else{
				json_obj_app_str = make_json_str(json_obj_app,STATE_FAILE,"get stata fail");
			}
#ifdef DEBUG_GET_FROM_MO
				printf("return to app data : %s\n",json_obj_app_str);
#endif
			send_msg_to_app(EVENT_GET_DEVICE_STATE,json_obj_app_str,pclientdata->client_fd);

			break;
		default:
			printf("wrong function number");
			
	}

	json_object_put(json_obj_app);
	free(pclientdata);
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

	json_object*json_obj_register_data = NULL;
	json_obj_register_data = json_tokener_parse(data);
	
	get_string_json_member(json_obj_register_data,"userName",user_name);
	get_string_json_member(json_obj_register_data,"password",user_password);
	get_string_json_member(json_obj_register_data,"phoneNumber",user_phone);
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
		fprintf(stderr, "SQL error(获取最大user_ID fail): %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		ID_num = 0;
	}

	//将用户信息保存到数据库
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
	if(pbool_user_name){
		int temp = 12;
		pthread_mutex_lock(FM0Data.pmutex);
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
	make_msg(msg_ele.msg,DEV_LED,(unsigned char)deviceNumber,(unsigned char)deviceCode);

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
	make_msg(msg_ele.msg,DEV_FAN,(unsigned char)deviceNumber,(unsigned char)deviceCode);
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
	make_msg(msg_ele.msg,DEV_DOOR,(unsigned char)deviceNumber,(unsigned char)deviceCode);

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
		temp.illumination = pM0Data->illumination;
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
