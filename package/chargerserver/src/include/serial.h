

#ifndef  __SERIAL__H_
#define __SERIAL__H_


int  UART_Open(char *port);
void UART_Close(int fd);
int UART_Init(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity);
int UART_Recv(int fd, char *rcv_buf, int data_len);
int UART_Send(int fd, char *send_buf, int data_len);
int power_bar_ctrl_send(int serial_fd, unsigned char pwn_status, unsigned char color, unsigned char color_cnt);
int power_bar_ctrl_recv(int serial_fd);

#endif  // serial.h

