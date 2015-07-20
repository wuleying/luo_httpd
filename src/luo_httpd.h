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
// 默认根目录
#define DEFAULT_ROOT_PATH			"htdocs"
// 默认首页
#define DEFAULT_HOME_PAGE			"index.html"
// 默认端口
#define DEFAULT_PORT				10008
// sock默认值
#define SOCK_DEFAULT 				-1
// 请求连接队列的最大长度
#define LISTEN_BACKLOG				5
// buf最大值
#define BUF_MAX_SIZE				1024
// method最大值
#define METHOD_MAX_SIZE				32
// url最大值
#define URL_MAX_SIZE				256
// path最大值
#define PATH_MAX_SIZE				512

// 状态码
#define LUO_OK 		 	 			0
#define LUO_ERROR 					-1

// 判断字符x是否为空白符
#define luo_isspace(x) 				isspace((int) (x))

/* 结构体声明 */
typedef struct sockaddr_in luo_sockaddr_in;
typedef struct sockaddr luo_sockaddr;
typedef struct stat luo_stat;

/* 函数声明 */
//
int luo_startup(u_short *port);
//
void *luo_accept_request(void *tclient);
// 执行普通文件
void luo_execute_file(int client, const char *path);
// 执行CGI脚本
void luo_execute_cgi(int client, const char *path, const char *method,
		const char *query_string);
//
int luo_get_line(int sock, char *buf, int buf_size);
// 读取文件内容
void luo_cat(int client, FILE *file);
// 输出header
void luo_headers(int client);
// 输出错误信息
void luo_error(const char *error);
// 不支持的请求方式
void luo_unimplemented(int client);
// 页面未发现
void luo_not_found(int client);

#endif /* _LUO_HTTPD_H_INCLUDED_ */
