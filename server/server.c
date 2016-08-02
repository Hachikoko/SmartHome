#include "server.h"

sqlite3* db_smart_home;
char* sqlite_err_msg = NULL;
int shmemfd;
struct ShareMemeryData*pshareMemeryData = NULL;
int semid = 0;
int msgid = 0;
struct pointer_for_M0_data FM0Data;

int main(int argc, const char *argv[])
{
	int listenfd = 0;
	int ret = 0;
	struct sockaddr_in addr_server;
	fd_set readfds;
	fd_set readfds_backup;
	int maxfd;
	thread_pool_t* pthread_pool;
	int max_thread = 20;
	int min_thread = 10;
	int pid = 0;
	int client_fd;
	struct sockaddr_in addr_client;
	socklen_t addr_len = sizeof(addr_client);
	int on = 1;
	int keepidle = 1;
	int keepinterval = 1;
	int keepcnt = 5;
	char* sql_create_user_info_table;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	struct ShareMemeryData M0_Struct_Data;
	

	union semun sem_val;


	if(argc != 3){
		printf("Usage: %s <ip> <port>",argv[0]);
		exit(EXIT_FAILURE);
	}

	pthread_pool = create_thread_pool(20,10);
	shmemfd = shmget(IPC_PRIVATE, sizeof(struct ShareMemeryData),IPC_CREAT | 0666 );
	if(-1 == shmemfd){
		perror("shmget fail");
		return -1;
	}
	
	pshareMemeryData = (struct ShareMemeryData*)shmat(shmemfd, NULL, 0);
	if(-1 == (int)pshareMemeryData){
		perror("shmat fail");
		return -1;
	}

	semid = semget(IPC_PRIVATE, 2, IPC_CREAT| 0666);
	if(-1 == semid){
		perror("semget fail");
		return -1;
	}

	sem_val.val = 1;
    ret = semctl(semid, SEM_W, SETVAL, sem_val);
	if (-1 == ret){
		perror("semctl init SEM_W fail");
		return -1;
	}

	sem_val.val = 0;
	ret = semctl(semid,SEM_R,SETVAL,sem_val);
	if(-1 == ret){
		perror("semctl init SEM_R fail");
		return -1;
	}

	msgid = msgget(IPC_PRIVATE, IPC_CREAT| 0666);
	if(-1 == semid){
		perror("semget fail");
		return -1;
	}




	listenfd = socket(AF_INET,SOCK_STREAM,0); //设置服务器文件描述符	
	if(-1 == listenfd){
		perror("socket fail");
		exit(EXIT_FAILURE);
	}
	//端口重用
	if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) < 0)
	{
		perror("setsockopt fail");
		exit(EXIT_FAILURE);
	}

	//绑定服务器地址
	bzero(&addr_server,sizeof(addr_server)); 
	addr_server.sin_family = AF_INET;
	addr_server.sin_addr.s_addr = inet_addr(argv[1]);
	addr_server.sin_port = htons(atoi(argv[2]));
	ret = bind(listenfd,(struct sockaddr*)&addr_server,sizeof(addr_server));
	if(ret != 0){
		perror("bind fail");
		exit(EXIT_FAILURE);
	}

	//监听服务器文件描述符
	ret = listen(listenfd,5);  
	if (ret == -1){
		perror("listen fail");
		exit(EXIT_FAILURE);
	}

	//设置监听表
	FD_ZERO(&readfds);
	FD_ZERO(&readfds_backup);
	FD_SET(listenfd,&readfds);
	maxfd = listenfd + 1;
	int i = 0;
	
	pid = fork();// 父进程负责APP与服务器通信，子进程负责与MO通信
	if(pid > 0){
		FM0Data.pgetM0StructData = &M0_Struct_Data;
		FM0Data.pmutex = &mutex;

		ret = requst_thread_work(pthread_pool,get_M0_data,&FM0Data);
		if(-1 == ret){
			printf("request_thread_work fail");
			return -1;
		}
		ret = sqlite3_open("Smart_Home.db",&db_smart_home);
		if(0 != ret){
			fprintf(stderr,"sqlite3_open fail :%s\n",sqlite3_errmsg(db_smart_home));
			return -1;
		}
		sql_create_user_info_table = "CREATE TABLE if not exists USER_INFO_TABLE("  \
									  "ID                 INT      PRIMARY KEY NOT NULL," \
									  "USER_TOKEN          TEXT    NOT NULL," \
									  "USER_NAME           TEXT    NOT NULL," \
									  "USER_PHONE         TEXT     NOT NULL,"\
									  "PASSWORD           TEXT     NOT NULL);";

		 ret = sqlite3_exec(db_smart_home, sql_create_user_info_table, print_table, 0, &sqlite_err_msg);
		 if( ret != SQLITE_OK ){
			 fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
			 sqlite3_free(sqlite_err_msg);
		 }else{
			 fprintf(stdout, "Table created successfully\n");
		 }

		//select
		while(1){
			readfds_backup = readfds;
			ret = select(maxfd,&readfds_backup,NULL,NULL,NULL);
			if (ret < 0){
				perror("select fail");
				exit(EXIT_FAILURE);
			}
			if(ret > 0){
				for(i = 0;i < maxfd;i++){
					if(FD_ISSET(i,&readfds_backup)){
						if(i == listenfd){
							client_fd = accept(listenfd,(struct sockaddr*)&addr_client,&addr_len);
							if(-1 == client_fd){
								perror("accept fail");
								exit(EXIT_FAILURE);
							}
							setsockopt(client_fd,SOL_SOCKET,SO_KEEPALIVE,&on,sizeof(on));
							setsockopt(client_fd,SOL_TCP,TCP_KEEPIDLE,&keepidle,sizeof(keepidle));
							setsockopt(client_fd,SOL_TCP,TCP_KEEPCNT,&keepcnt,sizeof(keepcnt));
							setsockopt(client_fd,SOL_TCP,TCP_KEEPINTVL,&keepinterval,sizeof(keepinterval));
#ifdef DEBUG
							printf("----------------------\n");
							printf("IP: %s\n",inet_ntoa(addr_client.sin_addr));
							printf("port: %d\n",ntohs(addr_client.sin_port));
							printf("-----------------------\n");
#endif
							FD_SET(client_fd,&readfds);
							if((client_fd + 1) > maxfd){
								maxfd = client_fd + 1;
							}
						}else{
							struct clientData *pclientdata = (struct clientData*)malloc(sizeof(struct clientData));
							pclientdata->data_len = recv(i, pclientdata->data, MAXBUFF, 0);
							pclientdata->client_fd = i;
#ifdef DEBUG
							printf("------------1----------\n");
							printf("rch[0]:%x\n",pclientdata->data[0]);
							printf("rch[1]:%x\n",pclientdata->data[1]);
							printf("rch[2,3]:%d\n",*((short*)(&pclientdata->data[2])));
							printf("rchdata:%s\n",&pclientdata->data[4]);
#endif
							ret = requst_thread_work(pthread_pool,do_something,pclientdata);
							if(-1 == ret){
								printf("request_thread_work fail");
								break;	
							}

							FD_CLR(i, &readfds);
						}
					}
				}
			} 


		}
	
	}

	if (0 == pid){
		int ret = 0;
		int uart_fd = 0;
		struct msg_element msg_ele;
		unsigned long pthread_id_shmemery = 0;
		unsigned long pthread_id_task = 0;
		//打开串口
		uart_fd = uart_open("/dev/ttyUSB0");
		if(0 == uart_fd){
			fprintf(stderr,"uart_open fail: %s\n","ttyUSB0");
			exit(EXIT_FAILURE);
		}
		//初始化串口
		ret = uart_init(uart_fd,115200,0,8,1,'n');
		if(-1 == ret){
			fprintf(stderr,"uart_init fail");
			return -1;
		}
		ret = pthread_create(&pthread_id_shmemery, NULL, read_M0, &uart_fd);
		if(0 != ret){
			printf("request_thread_work child process fail");
			return -1;
		}
		ret = pthread_detach(pthread_id_shmemery);
		if(0 != ret){
			printf("pthread_detach fail");
			return -1;
		}
#ifdef DEBUG_MSG_TO_M0
		printf("uart_fd:%d\n",uart_fd);
#endif
		ret = pthread_create(&pthread_id_task, NULL, tackle_msg, &uart_fd);
		if(0 != ret){
			printf("request_thread_work child process fail");
			return -1;
		}
		ret = pthread_detach(pthread_id_task);
		if(0 != ret){
			printf("pthread_detach fail");
			return -1;
		}
		msg_ele.type = 1;
		make_msg(msg_ele.msg,(unsigned char)DEV_STATA,(unsigned char)0x00,(unsigned char)0x00);
			
		while(1){
			ret = msgsnd(msgid, &msg_ele, MSGSZ,0);
			if(-1 == ret){
				perror("msgrcv fail");
				return -1;
			}
			sleep(30);
		}	
	}


	sqlite3_close(db_smart_home);
	return 0;
}




