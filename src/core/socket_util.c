#include "socket_util.h"

#include <fcntl.h>
#include <stdlib.h>
#include "base.h"

http_err_t setnoblocking(int fd)
{
	int flag = fcntl(fd, F_GETFL);
	if (flag < 0)
	{
		log_http_message(HTTP_FCNTL_F_GETFL_ERR);
		return http_fcntl_f_getfl;
	}
	flag |= O_NONBLOCK;
	
	if (fcntl(fd, F_SETFL, flag) < 0)
	{
		log_http_message(HTTP_FCNTL_F_SETFL_ERR);
		return http_fcntl_f_setfl;
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


