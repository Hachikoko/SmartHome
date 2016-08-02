#ifndef _UART_H_
#define _UART_H_ 

/* According to POSIX.1-2001 */
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
extern int uart_open(char * serial_port);
extern void uart_close(int fd);
extern int uart_init(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity);
extern int uart_recv(int fd, char *rcv_buf,int data_len,struct timeval *timeout);
extern int uart_send(int fd, char *send_buf,int data_len);
#endif 
