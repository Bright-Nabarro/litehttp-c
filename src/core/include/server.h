#pragma once
#include "error_code.h"

http_err_t get_ipv4_server_fd(int* fd);
void handle_server_fd(void* vargs);
