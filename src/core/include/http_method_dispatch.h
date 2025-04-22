#pragma once
#include "http_types.h"
#include "socket_util.h"

http_err_t http_method_get_handler(const http_request_t* request,
								   epoll_usrdata_t* client_data);
