#include "reactor.h"

#include "base.h"
#include "client.h"
#include "configure.h"
#include "server.h"
#include "socket_util.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

static atomic_bool loop_terminal = false;

static http_err_t handle_each_event(int server_fd, int epoll_fd,
								  struct epoll_event* evs, int nfds);

void ipv4_terminal_fn(int signo)
{
	log_http_message(HTTP_SIGNAL_TERMINAL, signo);
	atomic_store(&loop_terminal, true);
}

http_err_t handle_ipv4_main_loop(int server_fd)
{
	int epoll_fd = epoll_create1(0);
	if (epoll_fd < 0)
	{
		log_http_message_with_errno(HTTP_EPOLL_CREATE_ERR);
		return http_err_epoll_create;
	}

	struct epoll_event ini_ev;
	ini_ev.events = EPOLLIN | EPOLLET;
	epoll_usrdata_t* usrdata = create_epoll_usrdata(server_fd);
	string_init_from_string_view(&usrdata->ip, get_host());
	usrdata->port = config_get_port();
	ini_ev.data.ptr = usrdata;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ini_ev) < 0)
	{
		log_http_message_with_errno(HTTP_EPOLL_CTL_ADD_ERR);
		return http_err_epoll_ctl_add;
	}

	int max_events = config_get_max_events();
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
			herr = http_err_epoll_wait;
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
			return http_err_malloc;
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

