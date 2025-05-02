#include "server.h"

#include "base.h"
#include "client.h"
#include "configure.h"
#include "socket_util.h"
#include <errno.h>
#include <sys/epoll.h>

static struct sockaddr_in server_addr;

http_err_t get_ipv4_server_fd(int* fd)
{
	http_err_t herr = http_success;
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		log_http_message_with_errno(HTTP_SOCKET_ERR);
		herr = http_err_socket;
		goto err;
	}
	*fd = server_fd;
	
	// 允许端口重用
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		log_http_message_with_errno(HTTP_SETSOCKETOPT_ERR);
		herr = http_err_setsocketopt;
		goto err;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(config_get_port());
	if (inet_pton(AF_INET, string_view_cstr(get_host()),
				  &server_addr.sin_addr) != 1)
	{
		log_http_message_with_errno(HTTP_INET_PTON_ERR);
		herr = http_err_inet_pton;
		goto clean_fd;
	}

	if (bind(server_fd, (SA*)&server_addr, sizeof(server_addr)) < 0)
	{
		log_http_message_with_errno(HTTP_BIND_ERR);
		herr = http_err_bind;
		goto clean_fd;
	}
	
	if (listen(server_fd, config_get_listen_que()) < 0)
	{
		log_http_message_with_errno(HTTP_LISTEN_ERR);
		herr = http_err_listen;
		goto clean_fd;
	}

	herr = setnoblocking(server_fd);
	if (herr != http_success)
	{
		goto clean_fd;
	}

	log_http_message(HTTP_START_LISTEN, string_view_cstr(get_host()), config_get_port());

	return herr;
clean_fd:
	close(server_fd);
err:
	return herr;
}

void handle_server_fd(void* vargs)
{
	fn_args_t* args = vargs;
	epoll_usrdata_t* server_data = args->usrdata;
	int epoll_fd = args->epoll_fd;
	http_err_t herr = http_success;
	while (true)
	{
		struct sockaddr_in client_addr;
		socklen_t socklen = sizeof(client_addr);
		int server_fd = server_data->fd;

		int client_fd = accept(server_fd, (SA*)&client_addr, &socklen);
		if (client_fd < 0)
		{
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				break;

			log_http_message_with_errno(HTTP_ACCEPT_ERR);
			CALL_BACK_RETURN(args->pherr, http_err_accept);
		}

		client_data_t* usrdata = nullptr;
		herr = create_client_data(&usrdata, client_fd, &client_addr.sin_addr, client_addr.sin_port);
		if (herr != http_success)
			goto clean_fd;

		log_http_message(HTTP_NEW_CLIENT_GREAT, string_cstr(&usrdata->ip),
						 usrdata->port);

		struct epoll_event client_ev;
		herr = setnoblocking(client_fd);
		if (herr != http_success)
			goto clean_fd;

		client_ev.events = EPOLLIN | EPOLLET;
		client_ev.data.ptr = usrdata;

		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev) < 0)
		{
			log_http_message(HTTP_EPOLL_CTL_ADD_ERR);
			goto clean_fd;
		}

		CALL_BACK_RETURN(args->pherr, herr);

	clean_fd:
		close(client_fd);
		CALL_BACK_RETURN(args->pherr, herr);
	}

	CALL_BACK_RETURN(args->pherr, herr);
}

