#include "luo_httpd.h"

int luo_startup(u_short *port)
{
	int httpd = 0;
	luo_sockaddr_in name;

	httpd = socket(PF_INET, SOCK_STREAM, 0);
	if (httpd == LUO_ERROR)
	{
		luo_error("Socket error");
	}

	memset(&name, 0, sizeof(luo_sockaddr_in));

	name.sin_family = AF_INET;
	name.sin_port = htons(*port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(httpd, (luo_sockaddr *) &name, sizeof(name)) < 0)
	{
		luo_error("Bind error");
	}

	if (*port == 0)
	{
		socklen_t name_len = sizeof(name);

		if (getsockname(httpd, (luo_sockaddr *) &name, &name_len) == LUO_ERROR)
		{
			luo_error("Get socket name error");
		}

		*port = ntohs(name.sin_port);
	}

	if (listen(httpd, 5) < 0)
	{
		luo_error("Listen error");
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

	sprintf(path, "htdocs%s", url);

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

}

// 执行CGI脚本
void luo_execute_cgi(int client, const char *path, const char *method,
		const char *query_string)
{

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
	sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "your request because the resource specified\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "is unavailable or nonexistent.</P>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}

// 主函数
int main(void)
{
	int server_sock = SOCK_DEFAULT;
	u_short port = 0;
	int client_sock = SOCK_DEFAULT;
	socklen_t client_name_len = sizeof(luo_sockaddr_in);
	pthread_t new_thread;

	server_sock = luo_startup(&port);
	printf("luo_httpd running on port %d\n", port);

	while (1)
	{
		client_sock = accept(server_sock, (luo_sockaddr *) &client_sock,
				&client_name_len);

		if (client_sock == LUO_ERROR)
		{
			luo_error("Accept error");
		}

		if (pthread_create(&new_thread, NULL, luo_accept_request,
				(void *) &client_sock) != 0)
		{
			luo_error("Create error");
		}
	}

	close(server_sock);

	return 0;
}
