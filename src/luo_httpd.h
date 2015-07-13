#ifndef _LUO_HTTPD_H_INCLUDED_
#define _LUO_HTTPD_H_INCLUDED_

/* 头文件 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
// 互联网地址族头文件(Internet address family)
#include <netinet/in.h>
// 互联网操作的定义(Definitions for internet operations)
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <pthread.h>

/* 常量/宏声明 */
// 服务描述
#define SERVER_DESC 				"Server: luo_httpd/0.0.1\r\n"

// sock默认值
#define SOCK_DEFAULT 				-1

// 状态码
#define LUO_OK 		 	 			0
#define LUO_ERROR 					-1

// 判断字符x是否为空白符
#define luo_isspace(x) 				isspace((int) (x))

/* 结构体声明 */
typedef struct sockaddr_in 			luo_sockaddr_in;
typedef struct sockaddr 			luo_sockaddr;
typedef struct stat 				luo_stat;

/* 函数声明 */
int luo_startup(u_short *port);
void *luo_accept_request(void *tclient);
int luo_get_line(int sock, char *buf, int buf_size);
// 输出错误信息
void luo_error(const char *error);


#endif /* _LUO_HTTPD_H_INCLUDED_ */
