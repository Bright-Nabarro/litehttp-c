#pragma once
#include "error_code.h"
#include "http_response.h"
#include "client.h"

http_err_t get_ipv4_server_fd(int* fd);
void handle_server_fd(void* vargs);

http_err_t http_send_response(const client_data_t* client,
							  const http_response_t* response);

