#include "socket_util.h"

#include <fcntl.h>
#include <stdlib.h>
#include "base.h"
#include <unistd.h>
#include <limits.h>
#include "configure.h"

http_err_t setnoblocking(int fd)
{
	int flag = fcntl(fd, F_GETFL);
	if (flag < 0)
	{
		log_http_message(HTTP_FCNTL_F_GETFL_ERR);
		return http_err_fcntl_f_getfl;
	}
	flag |= O_NONBLOCK;
	
	if (fcntl(fd, F_SETFL, flag) < 0)
	{
		log_http_message(HTTP_FCNTL_F_SETFL_ERR);
		return http_err_fcntl_f_setfl;
	}
	return http_success;
}


void ipv4_net2str(string_t* s, struct in_addr* addr)
{
	char buf[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, addr, buf, sizeof(buf));
	string_init_from_cstr(s, buf);
}

epoll_usrdata_t* create_epoll_usrdata(int fd)
{
	epoll_usrdata_t* data = calloc(1, sizeof(epoll_usrdata_t));
	data->fd = fd;
	return data;
}

void epoll_usrdata_free(epoll_usrdata_t* data)
{
	free(data);
}

http_err_t get_absolue_path(string_view_t postfix, string_t* path)
{
	char buf[PATH_MAX];
	if (getcwd(buf, sizeof(buf)) == nullptr)
	{
		log_http_message_with_errno(HTTP_GETCWD_ERR);
		return http_err_getcwd;
	}
	if (!string_init_from_cstr(path, buf))
	{
		log_http_message_with_errno(HTTP_MALLOC_ERR);
		return http_err_malloc;
	}
	string_push_back(path, '/');
	log_debug_message("base dir %.*s", (int)string_view_len(get_base_dir()),
					  string_view_cstr(get_base_dir()));
	string_append_view(path, get_base_dir());
	string_push_back(path, '/');
	string_append_view(path, postfix);

	return http_success;
}

