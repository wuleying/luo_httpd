#include "luo_httpd.h"

int luo_startup(u_short *port)
{
	int httpd = 0;
	luo_sockaddr_in addr;

	httpd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);

	if (httpd == LUO_ERROR)
	{
		luo_error("socket() error. in luo_httpd.c luo_startup()");
	}

	memset(&addr, 0, sizeof(luo_sockaddr_in));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(*port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// 绑定socket
	if (bind(httpd, (luo_sockaddr *) &addr, sizeof(addr)) < 0)
	{
		luo_error("bind() error. in luo_httpd.c luo_startup()");
	}

	if (*port == 0)
	{
		socklen_t addr_len = sizeof(addr);

		if (getsockname(httpd, (luo_sockaddr *) &addr, &addr_len) == LUO_ERROR)
		{
			luo_error("getsockname() error. in luo_httpd.c luo_startup()");
		}

		*port = ntohs(addr.sin_port);
	}

	// 监听socket
	if (listen(httpd, LISTEN_BACKLOG) < 0)
	{
		luo_error("listen() error. in luo_httpd.c luo_startup()");
	}

	return httpd;
}

void *
luo_accept_request(void *tclient)
{
	int client = *(int *) tclient;
	int char_number;
	char buf[BUF_MAX_SIZE];
	char method[METHOD_MAX_SIZE];
	char url[URL_MAX_SIZE];
	char path[PATH_MAX_SIZE];
	size_t i, j;
	luo_stat st;
	int cgi = 0;
	char *query_string = NULL;

	char_number = luo_get_line(client, buf, sizeof(buf));

	// 获取请求方式
	i = 0;
	j = 0;
	while (!luo_isspace(buf[j]) && (i < sizeof(method) - 1))
	{
		method[i] = buf[j];
		i++;
		j++;
	}
	method[i] = '\0';

	// 不为GET或POST方式请求(暂时只支持两种方式)
	if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
	{
		// 不支持的请求方式
		luo_unimplemented(client);
		return NULL;
	}

	// 请求方式为POST，启用CGI
	if (strcasecmp(method, "POST") == 0)
	{
		cgi = 1;
	}

	// 获取URL
	i = 0;
	while (luo_isspace(buf[j]) && j < sizeof(buf))
	{
		j++;
	}

	while (!luo_isspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
	{
		url[i] = buf[j];
		i++;
		j++;
	}
	url[i] = '\0';

	// 获取查询字符串
	if (strcasecmp(method, "GET") == 0)
	{
		query_string = url;

		while (*query_string != '?' && *query_string != '\0')
		{
			query_string++;
		}

		if (*query_string == '?')
		{
			cgi = 1;
			*query_string = '\0';
			query_string++;
		}
	}

	sprintf(path, "%s%s", DEFAULT_ROOT_PATH, url);

	// path最后一位为/时 设置默认首页
	if (path[strlen(path) - 1] == '/')
	{
		strcat(path, DEFAULT_HOME_PAGE);
	}

	if (stat(path, &st) == -1)
	{
		while ((char_number > 0) && strcmp("\n", buf))
		{
			char_number = luo_get_line(client, buf, sizeof(buf));
		}

		// 页面未发现
		luo_not_found(client);
	}
	else
	{
		// 档案类型为目录时 设置默认首页
		if ((st.st_mode & S_IFMT) == S_IFDIR)
		{
			strcat(path, DEFAULT_HOME_PAGE);
		}

		// 检查是否对文件拥有可执行权限 user | group | other
		if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP)
				|| (st.st_mode & S_IXOTH))
		{
			cgi = 1;
		}

		if (cgi)
		{
			// 执行CGI脚本
			luo_execute_cgi(client, path, method, query_string);
		}
		else
		{
			// 执行普通文件
			luo_execute_file(client, path);
		}
	}

	close(client);

	return NULL;
}

// 执行普通文件
void luo_execute_file(int client, const char *path)
{
	FILE *file = NULL;
	int char_number = 1;
	char buf[BUF_MAX_SIZE];

	buf[0] = 'A';
	buf[1] = '\0';
	while ((char_number > 0) && strcmp("\n", buf))
	{
		char_number = luo_get_line(client, buf, sizeof(buf));
	}

	file = fopen(path, "r");

	if (file == NULL)
	{
		luo_not_found(client);
	}
	else
	{
		luo_headers(client);
		luo_cat(client, file);
	}
	fclose(file);
}

// 执行CGI脚本
void luo_execute_cgi(int client, const char *path, const char *method,
		const char *query_string)
{
	char buf[BUF_MAX_SIZE];
	int cgi_input[2];
	int cgi_output[2];
	pid_t pid;
	int status;
	int i;
	char c;
	int char_number = 1;
	int content_len = -1;

	buf[0] = 'A';
	buf[1] = '\0';

	// 根据请求方式作不同处理
	if (strcasecmp(method, "GET") == 0)
	{
		// GET方式
		while ((char_number > 0) && strcmp("\n", buf))
		{
			char_number = luo_get_line(client, buf, sizeof(buf));
		}
	}
	else
	{
		// POST 方式
		char_number = luo_get_line(client, buf, sizeof(buf));

		while ((char_number > 0) && strcmp("\n", buf))
		{
			buf[15] = '\0';
			if (strcasecmp(buf, "Content-Length:") == 0)
			{
				content_len = atoi(&(buf[16]));
			}
		}

		if (content_len == -1)
		{
			// todo bad request
			return;
		}
	}

	// 发送200状态
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);

	if ((pipe(cgi_output) < 0) || (pipe(cgi_input) < 0))
	{
		// todo cannot execute
		return;
	}

	if ((pid = fork()) < 0)
	{
		// todo cannot execute
		return;
	}

	if (pid == 0)
	{
		// 子进程 执行CGI脚本
		char method_env[255];
		char query_env[255];
		char length_env[255];

		dup2(cgi_output[1], 1);
		dup2(cgi_input[0], 0);

		close(cgi_output[0]);
		close(cgi_input[1]);

		sprintf(method_env, "REQUEST_METHOD=%s", method);
		// 将请求方式添加到环境变量
		putenv(method_env);

		if (strcasecmp(method, "GET") == 0)
		{
			// GET方式
			sprintf(query_env, "QUERY_STRING=%s", query_string);
			// 将查询字符串添加到环境变量
			putenv(query_env);
		}
		else
		{
			// POST方式
			sprintf(length_env, "CONTENT_LENGTH=%d", content_len);
			// 将内容长度添加到环境变量
			putenv(length_env);
		}

		// 执行CGI脚本
		execl(path, path, NULL);
		exit(0);
	}
	else
	{
		// 父进程
		close(cgi_output[1]);
		close(cgi_input[0]);

		if (strcasecmp(method, "POST") == 0)
		{
			for (i = 0; i < content_len; i++)
			{
				recv(client, &c, 1, 0);
				write(cgi_input[1], &c, 1);
			}
		}

		while (read(cgi_output[0], &c, 1) > 0)
		{
			send(client, &c, 1, 0);
		}

		close(cgi_output[0]);
		close(cgi_input[1]);

		// 停止目前进程的执行 直到有信号来到或子进程结束
		waitpid(pid, &status, 0);
	}
}

int luo_get_line(int sock, char *buf, int buf_size)
{
	int i = 0;
	int n;
	char c = '\0';

	while ((i < buf_size - 1) && c != '\n')
	{
		n = recv(sock, &c, 1, 0);

		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);

				if ((n > 0) && (c == '\n'))
				{
					recv(sock, &c, 1, 0);
				}
				else
				{
					c = '\n';
				}
			}

			buf[i] = c;
			i++;
		}
		else
		{
			c = '\n';
		}
	}

	buf[i] = '\0';
	return i;
}

// 读取文件内容
void luo_cat(int client, FILE *file)
{
	char buf[BUF_MAX_SIZE];

	while (!feof(file))
	{
		fgets(buf, sizeof(buf), file);
		send(client, buf, strlen(buf), 0);
	}
}

void luo_headers(int client)
{
	char buf[BUF_MAX_SIZE];

	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, SERVER_DESC);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
}

// 输出错误信息
void luo_error(const char *error)
{
	perror(error);
	exit(1);
}

// 不支持的请求方式
void luo_unimplemented(int client)
{
	char buf[BUF_MAX_SIZE];

	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, SERVER_DESC);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</TITLE></HEAD>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>HTTP request method not supported.</P>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}

// 页面未发现
void luo_not_found(int client)
{
	char buf[BUF_MAX_SIZE];

	sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, SERVER_DESC);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>404 Not Found</P>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}

// 主函数
int main(void)
{
	int server_sock = SOCK_DEFAULT;
	u_short port = DEFAULT_PORT;
	int client_sock = SOCK_DEFAULT;
	luo_sockaddr_in client_name;
	socklen_t client_name_len = sizeof(client_name);
	pthread_t new_thread;

	server_sock = luo_startup(&port);
	printf("luo_httpd running on port %d\n", port);

	while (1)
	{
		client_sock = (accept(server_sock, (luo_sockaddr *) &client_name,
				&client_name_len));

		if (client_sock == LUO_ERROR)
		{
			luo_error("accept() error. in luo_httpd.c main()");
		}

		if (pthread_create(&new_thread, NULL, luo_accept_request,
				(void *) &client_sock) != 0)
		{
			luo_error("pthread_create() error. in luo_httpd.c main()");
		}
	}

	close(server_sock);

	return 0;
}
