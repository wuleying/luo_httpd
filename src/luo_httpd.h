#ifndef _LUO_HTTPD_H_INCLUDED_
#define _LUO_HTTPD_H_INCLUDED_

/* 头文件 */
// 标准输入输出库 (Standard Input & Output)
#include <stdio.h>
// 标准库 (Standard Library)
#include <stdlib.h>
// 套接字接口 (Socket)
#include <sys/socket.h>
// 基本系统数据类型 (Types)
#include <sys/types.h>
// 文件状态 (unix/linux系统定义文件状态所在的伪标准头文件)
#include <sys/stat.h>
// 进程控制
#include <sys/wait.h>
// 互联网地址族 (Internet address family)
#include <netinet/in.h>
// 互联网操作的定义 (Definitions for internet operations)
#include <arpa/inet.h>
// 提供对 POSIX 操作系统 API 的访问功能
#include <unistd.h>
// 字符分类函数 (C character classification functions)
#include <ctype.h>
// 字符串操作函数 (POSIX标准定义的XSI扩展头文件)
#include <strings.h>
// 字符串操作函数 (ISO C标准定义的头文件)
#include <string.h>
// 线程 (POSIX threads)
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
#define LUO_OK						0
#define LUO_ERROR					-1

// 判断字符x是否为空白符
#define luo_isspace(x) 				isspace((int) (x))

/* 结构体声明 */
typedef struct sockaddr_in luo_sockaddr_in;
typedef struct sockaddr luo_sockaddr;
typedef struct stat luo_stat;

/* 函数声明 */
// 初始化httpd
int luo_httpd_init(u_short *port);
// 接收请求
void *luo_accept_request(void *tclient);
// 执行普通文件
void luo_execute_file(int client, const char *path);
// 执行CGI脚本
void luo_execute_cgi(int client, const char *path, const char *method,
		const char *query_string);
// 获取一行数据 并写入buf
int luo_get_line(int sock, char *buf, int buf_size);
// 读取文件内容
void luo_read_file(int client, FILE *file);
// 输出header
void luo_header(int client);
// 输出错误信息
void luo_error(const char *error);
// 不支持的请求方式
void luo_unimplemented(int client);
// 页面未发现
void luo_not_found(int client);
// 无效的请求
void luo_bad_request(int client);
// CGI无法执行
void luo_cannot_execute(int client);
// 输出http异常
void luo_exception(int client, int info_code, const char *title,
		const char *content);

#endif /* _LUO_HTTPD_H_INCLUDED_ */
