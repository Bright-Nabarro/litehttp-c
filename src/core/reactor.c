#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdlib.h>

#include "http_parser.h"
#include "http_types.h"
#include "base.h"
#include "socket_util.h"
#include "reactor.h"
#include "http_method_dispatch.h"
#include "configure.h"

static struct sockaddr_in server_addr;
static atomic_bool loop_terminal = false;

static http_err_t handle_each_event(int server_fd, int epoll_fd,
								  struct epoll_event* evs, int nfds);
static void handle_server_fd(void* vargs);
static void handle_client_fd(void* vargs);
static http_err_t read_client(string_t* rec_str, int epfd, epoll_usrdata_t* usrdata);
static http_err_t handle_http_request(const http_request_t* request, epoll_usrdata_t* usrdata);

#define CALL_BACK_SET(pherr, http_err_enum)                                    \
	do                                                                         \
	{                                                                          \
		if (pherr == nullptr)                                                  \
			break;                                                             \
		*pherr = http_err_enum;                                                \
	} while (0)

#define CALL_BACK_RETURN(pherr, http_err_enum)                                 \
	do                                                                         \
	{                                                                          \
		CALL_BACK_SET(pherr, http_err_enum);                                   \
		return;                                                                \
	} while (0)

typedef struct 
{
	http_err_t* pherr;
	int epoll_fd;
	epoll_usrdata_t* usrdata;
} fn_args_t;

void ipv4_terminal_fn(int signo)
{
	log_http_message(HTTP_SIGNAL_TERMINAL, signo);
	atomic_store(&loop_terminal, true);
}

http_err_t get_ipv4_server_fd(int* fd)
{
	http_err_t herr = http_success;
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		log_http_message_with_errno(HTTP_SOCKET_ERR);
		herr = http_socket;
		goto err;
	}
	*fd = server_fd;
	
	// 允许端口重用
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		log_http_message_with_errno(HTTP_SETSOCKETOPT_ERR);
		herr = http_setsocketopt;
		goto err;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(get_port());
	if (inet_pton(AF_INET, string_view_cstr(get_host()),
				  &server_addr.sin_addr) != 1)
	{
		log_http_message_with_errno(HTTP_INET_PTON_ERR);
		herr = http_inet_pton;
		goto clean_fd;
	}

	if (bind(server_fd, (SA*)&server_addr, sizeof(server_addr)) < 0)
	{
		log_http_message_with_errno(HTTP_BIND_ERR);
		herr = http_bind;
		goto clean_fd;
	}
	
	if (listen(server_fd, get_listen_que()) < 0)
	{
		log_http_message_with_errno(HTTP_LISTEN_ERR);
		herr = http_listen;
		goto clean_fd;
	}

	herr = setnoblocking(server_fd);
	if (herr != http_success)
	{
		goto clean_fd;
	}

	log_http_message(HTTP_START_LISTEN, string_view_cstr(get_host()), get_port());

	return herr;
clean_fd:
	close(server_fd);
err:
	return herr;
}

http_err_t handle_ipv4_main_loop(int server_fd)
{
	int epoll_fd = epoll_create1(0);
	if (epoll_fd < 0)
	{
		log_http_message_with_errno(HTTP_EPOLL_CREATE_ERR);
		return http_epoll_create;
	}

	struct epoll_event ini_ev;
	ini_ev.events = EPOLLIN | EPOLLET;
	epoll_usrdata_t* usrdata = create_epoll_usrdata(server_fd);
	string_init_from_string_view(&usrdata->ip, get_host());
	usrdata->port = get_port();
	ini_ev.data.ptr = usrdata;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ini_ev) < 0)
	{
		log_http_message_with_errno(HTTP_EPOLL_CTL_ADD_ERR);
		return http_epoll_ctl_add;
	}

	int max_events = get_max_events();
	struct epoll_event* events =
		malloc(max_events * sizeof(struct epoll_event));
	http_err_t herr = http_success;
	while(!atomic_load(&loop_terminal))
	{
		int nfds = epoll_wait(epoll_fd, events, max_events, -1);
		if (nfds < 0)
		{
			if (errno == EINTR)
				continue;

			log_http_message_with_errno(HTTP_EPOLL_WAIT_ERR);
			herr = http_epoll_wait;
			goto clean_fd;
		}

		herr = handle_each_event(server_fd, epoll_fd, events, nfds);
		if (herr != http_success)
			goto clean_fd;
	}

clean_fd:
	close(epoll_fd);
	return herr;
}

static http_err_t handle_each_event(int server_fd, int epoll_fd,
								  struct epoll_event* evs, int nfds)
{
	http_err_t herr = http_success;
	for (int i = 0 ; i < nfds; ++i)
	{
		epoll_usrdata_t* data = evs[i].data.ptr;
		fn_args_t* args = malloc(sizeof(fn_args_t));
		if (args == nullptr)
		{
			log_http_message_with_errno(HTTP_MALLOC_ERR);
			return http_malloc;
		}
		args->usrdata = data;
		args->epoll_fd = epoll_fd;
		args->pherr = nullptr;

		if (data->fd == server_fd)
		{
			thdpool_addjob(handle_server_fd, args, free);
		}
		else
		{
			thdpool_addjob(handle_client_fd, args, free);
		}
	}
	return herr;
}

static void handle_server_fd(void* vargs)
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
			CALL_BACK_RETURN(args->pherr, http_accept);
		}

		epoll_usrdata_t* usrdata = create_epoll_usrdata(client_fd);
		string_t *point = &usrdata->ip;

		ipv4_net2str(point, &client_addr.sin_addr);
		int client_port = ntohs(client_addr.sin_port);
		log_http_message(HTTP_NEW_CLIENT_GREAT, string_cstr(point), client_port);

		usrdata->port = client_port;

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

static void handle_client_fd(void* vargs)
{
	fn_args_t* args = vargs;
	epoll_usrdata_t* client_data = args->usrdata;
	int epoll_fd = args->epoll_fd;
	http_err_t herr = http_success;

	log_http_message(HTTP_REPORT_CLIENT_INFO, string_cstr(&client_data->ip),
					 client_data->port);
	bool client_request_close = false;
	string_t receive_post;
	// 读取客户端, 写入receive_post
	herr = read_client(&receive_post, epoll_fd, client_data);
	if (herr == http_client_close)
	{
		client_request_close = true;
	}
	if (herr != http_success)
		CALL_BACK_RETURN(args->pherr, herr);

	http_request_t* http_request = http_request_alloc();
	herr = parse_http_request(string_view_from_string(&receive_post),
							  &http_request);
	if (herr != http_success)
		CALL_BACK_RETURN(args->pherr, herr);
	
	herr = handle_http_request(http_request, client_data);
	http_request_free(http_request);

	CALL_BACK_RETURN(args->pherr, herr);
}

static
http_err_t read_client(string_t* rec_str, int epfd, epoll_usrdata_t* usrdata)
{
	char buffer[BUFSIZ];
	http_err_t herr = http_success;
	bool req_close = false;
	string_init(rec_str);
	while(true)
	{
		ssize_t readsize = 0;
		do
		{
			readsize = read(usrdata->fd, buffer, sizeof(buffer)-1);
		} while (readsize < 0 && errno == EINTR);
		if (readsize < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			log_http_message_with_errno(HTTP_READ_ERR);
			req_close = true;
			break;
		}
		else if (readsize == 0)
		{
			req_close = true;
			break;
		}
		buffer[readsize] = 0;
		log_http_message(HTTP_RECV_FROM_CLIENT, buffer);
		
		string_append_parts(rec_str, buffer, readsize);
	}

	if (req_close)
	{
		log_http_message(HTTP_CLIENT_CLOSE, string_cstr(&usrdata->ip), usrdata->port);	
		if (epoll_ctl(epfd, EPOLL_CTL_DEL, usrdata->fd, NULL) < 0)
		{
			log_http_message(HTTP_EPOLL_CTL_DEL_ERR);
			herr = http_epoll_ctl_del;
		}
		close(usrdata->fd);
		epoll_usrdata_free(usrdata);
		return http_client_close;
	}
	return herr;
}

static http_err_t handle_http_request(const http_request_t* request,
									  epoll_usrdata_t* usrdata)
{
	switch(request->method)
	{
	case http_method_get:
		http_method_get_handler(request, usrdata);
	default:
	}
	return http_success;
}

