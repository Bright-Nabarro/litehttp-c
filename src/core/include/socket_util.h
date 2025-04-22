#pragma once
#define SA struct sockaddr
#include <arpa/inet.h>

#include "error_code.h"
#include "string_t.h"

typedef struct
{
	int fd;
	string_t ip;
	int port;
} epoll_usrdata_t;

http_err_t setnoblocking(int fd);

void ipv4_net2str(string_t* s, struct in_addr* addr);

epoll_usrdata_t* create_epoll_usrdata(int fd);
void epoll_usrdata_free(epoll_usrdata_t* data);

