#include <stdio.h>
#include "./uart.h"
#include <unistd.h>

int main(int argc, const char *argv[])
{
	int uart_fd;
	int ret = 0;
	int i = 0;
	int j = 0;
	char data[1024] = {0};
	uart_fd = uart_open("/dev/ttyUSB0");
	if (-1 == uart_fd){
		fprintf(stderr,"uart_open fail");
		return -1;
	}
	ret = uart_init(uart_fd,115200,0,8,1,'n');
	if(-1 == ret){
		fprintf(stderr,"uart_init fail");
		return -1;
	}

	unsigned char comd[4] = {0};
	comd[0] = 0xdd;
	comd[1] = 0x04;
	comd[2] = 0x00;
	comd[3] = 0x01;

	ret = uart_send(uart_fd, comd,4);
	if(-1 == ret){
		perror("puart_fd fail");
		return -1;
	}
	ret = uart_recv(uart_fd,data,1024,NULL);
	if(-1 == ret){
		printf("uart_recv fail");
		return -1;
	}

	printf("data:%s\n",data);



//	for(i = 0;i < 3;i++){
//		ret = uart_send(uart_fd, &comd[i][0],4);
//		if(-1 == ret){
//			perror("puart_fd fail");
//			return -1;
//		}
//		sleep(10);
//	}
	uart_close(uart_fd);
	return 0;
}
