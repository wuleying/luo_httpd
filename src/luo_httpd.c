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

	return (httpd);
}

void *
accept_request(void *tclient)
{
	return NULL;
}

// 输出错误信息
void luo_error(const char *error)
{
	perror(error);
	exit(1);
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

		if (pthread_create(&new_thread, NULL, accept_request,
				(void *) &client_sock) != 0)
		{
			luo_error("Create error");
		}
	}

	close(server_sock);

	return 0;
}
