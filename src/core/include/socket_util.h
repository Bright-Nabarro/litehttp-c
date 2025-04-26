#pragma once
#include <arpa/inet.h>

#include "error_code.h"
#include "string_t.h"

#define SA struct sockaddr

typedef struct
{
	int fd;
	string_t ip;
	int port;
} epoll_usrdata_t;

typedef struct 
{
	http_err_t* pherr;
	int epoll_fd;
	epoll_usrdata_t* usrdata;
} fn_args_t;

http_err_t setnoblocking(int fd);

void ipv4_net2str(string_t* s, struct in_addr* addr);

epoll_usrdata_t* create_epoll_usrdata(int fd);
void epoll_usrdata_free(epoll_usrdata_t* data);

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

http_err_t get_absolue_path(string_view_t postfix, string_t* path);
