#pragma once
#include "error_code.h"
#include "string_t.h"

http_err_t parse_config_file(const char* file_name);
void release_config_file();

http_err_t config_server();
http_err_t config_log();
http_err_t config_core();

// server
int config_get_port();
string_view_t get_host();
string_view_t get_base_dir();

// core
int config_get_handle_threads();
int config_get_max_events();
int config_get_listen_que();

// thread pool
http_err_t thdpool_initial();
http_err_t thdpool_addjob(void (*callback)(void*), void* args,
						void (*clean)(void*));
http_err_t thdpool_release();
