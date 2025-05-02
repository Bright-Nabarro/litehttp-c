#pragma once
#include "string_t.h"
#include "http_parse_dfa.h"
#include <arpa/inet.h>

typedef struct
{
	int fd;
	string_t ip;
	int port;
	http_parse_ctx_t* ctx_beg;
	http_parse_ctx_t* ctx_tail;
} client_data_t;
void handle_client_fd(void* vargs);

http_err_t create_client_data(client_data_t** ret, int fd,
							  struct in_addr* remote_addr, int remote_port);
void client_data_free(client_data_t* data);

