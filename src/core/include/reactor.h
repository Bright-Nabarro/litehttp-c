#pragma once
#include "error_code.h"

http_err_t get_ipv4_server_fd(int* fd);
http_err_t handle_ipv4_main_loop(int fd);

void ipv4_terminal_fn(int);
