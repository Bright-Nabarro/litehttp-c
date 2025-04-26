#pragma once
#include "error_code.h"
#include "string_t.h"

http_err_t parse_config_file(const char* file_name);
void release_config_file();

http_err_t server_config();
http_err_t log_config();
http_err_t core_config();

// server
int get_port();
string_view_t get_host();
string_view_t get_base_dir();

// core
int get_handle_threads();
int get_max_events();
int get_listen_que();

// thread pool
http_err_t thdpool_initial();
http_err_t thdpool_addjob(void (*callback)(void*), void* args,
						void (*clean)(void*));
http_err_t thdpool_release();
