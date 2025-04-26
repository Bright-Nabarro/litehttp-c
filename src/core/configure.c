#include "configure.h"

#include <toml.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "ctp/logger.h"
#include "ctp/thd_pool.h"
#include "base.h"

static toml_table_t* toml_configure = nullptr;
static ctp_thdpool_t* thdpool = nullptr;

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 2233

static struct
{
	bool async;
	string_t log_file;
} config_logger;

static struct 
{
	string_t host;
	int port;
	string_t base_dir;
} config_server;

static struct
{
	int handle_threads;
	int listen_que;
	int max_events;
} config_core;

http_err_t parse_config_file(const char* filename)
{
	FILE* config_file = fopen(filename, "r");
	if (!config_file)
	{
		fprintf(stderr, "%s", strerror(errno));
		return http_path_err;
	}
	char buf[BUFSIZ];
	toml_configure = toml_parse_file(config_file, buf, sizeof(buf));
	fclose(config_file);
	if (!toml_configure)
	{
		fprintf(stderr, "toml_parse_file error: %s", buf);
		return http_config;
	}
	
	return http_success;
}

void release_config_file()
{
	ctp_logger_global_close();
	toml_free(toml_configure);
}

http_err_t log_config()
{
	ctp_logger_config_t ctp_logger_config;
	ctp_logger_config_default(&ctp_logger_config);
	toml_table_t* table = toml_table_table(toml_configure, "logger");
	http_err_t herr = http_success;
	if (table)
	{
		toml_value_t value;
		value = toml_table_bool(table, "async");
		if (value.ok)
		{
			config_logger.async = value.u.b;
			ctp_logger_config.async = config_logger.async;
		}
		value = toml_table_string(table, "log_file");
		if (value.ok)
		{
			string_take_ownership(&config_logger.log_file, value.u.s,
								  value.u.sl);
			
			ctp_logger_config.log_file = string_cstr(&config_logger.log_file);
		}
	}
	else
	{
		fprintf(stderr, "%s\n", http_message_str(HTTP_UNSET_LOGGER));
		herr = http_unset_use_default;
	}
	ctp_logger_global_init(&ctp_logger_config);

	return herr;
}

http_err_t server_config()
{
	toml_table_t* table = toml_table_table(toml_configure, "server");
	if (!table)
	{
		log_http_message(HTTP_UNSET_SERVER);
		return http_unset_fatal;
	}

	http_err_t herr = http_success;

	toml_value_t value;
	value = toml_table_string(table, "host");
	if (!value.ok)
	{
		log_http_message(HTTP_UNSET_HOST);
		herr = http_unset_use_default;
		string_init_from_cstr(&config_server.host, DEFAULT_HOST);
	}
	else
	{
		string_take_ownership(&config_server.host, value.u.s, value.u.sl);
	}
	
	value = toml_table_int(table, "port");
	if (!value.ok)
	{
		log_http_message(HTTP_UNSET_PORT);
		herr = http_unset_use_default;
		config_server.port = DEFAULT_PORT;
	}
	else
	{
		config_server.port = value.u.i;
	}

	value = toml_table_string(table, "base_dir");
	if (!value.ok)
	{
		log_http_message(HTTP_UNSET_BASE_DIR);
		return http_unset_fatal;
	}
	else
	{
		string_take_ownership(&config_server.base_dir, value.u.s, value.u.sl);
	}
	
	return herr;
}


http_err_t core_config()
{
	http_err_t herr = http_success;
	toml_table_t* table = toml_table_table(toml_configure, "core");
	if (table == nullptr)
		return http_unset_fatal;

	toml_value_t value;
	value = toml_table_int(table, "handle_threads");
	if (!value.ok)
	{
		log_http_message(HTTP_UNSET_HANDLE_THREADS);
		herr = http_unset_use_default;
		config_core.handle_threads = 4;
	}
	else
	{
		config_core.handle_threads = value.u.i;
	}

	value = toml_table_int(table, "listen_que");
	if (!value.ok)
	{
		log_http_message(HTTP_UNSET_LISTEN_QUE);
		herr = http_unset_use_default;
		config_core.listen_que = 10;
	}
	else
	{
		config_core.listen_que = value.u.i;
	}

	value = toml_table_int(table, "listen_que");
	if (!value.ok)
	{
		log_http_message(HTTP_UNSET_MAX_EVENT);
		herr = http_unset_use_default;
		config_core.max_events = 100;
	}
	else
	{
		config_core.max_events = value.u.i;
	}

	return herr;
}


// server
int get_port()
{
	return config_server.port;
}

string_view_t get_host()
{
	return string_view_from_string(&config_server.host);
}

string_view_t get_base_dir()
{
	return string_view_from_string(&config_server.base_dir);
}

// core
int get_handle_threads()
{
	return config_core.handle_threads;
}

int get_max_events()
{
	return config_core.max_events;
}

int get_listen_que()
{
	return config_core.listen_que;
}


// thread pool
http_err_t thdpool_initial()
{
	http_err_t herr = http_success;
	int ec = 0;
	thdpool = ctp_thdpool_create(get_handle_threads(), &ec);
	if (ec != 0)
	{
		log_http_message_with_ec(ec, HTTP_THD_POOL_INI_ERR);
		herr = http_thd_pool_ini;
	}

	return herr;
}

http_err_t thdpool_release()
{
	int* thd_ecs;
	int ec = 0;
	size_t thd_ecs_len = 0;
	ctp_thdpool_destroy(thdpool, &ec, &thd_ecs, &thd_ecs_len);
	thdpool = NULL;
	if (ec == 0)
	{
		return http_success;
	}
	else if (ec > 0)
	{
		log_http_message_with_ec(ec, HTTP_THD_POOL_DESTROY_ERR);
		return http_thd_pool_destroy;
	}

	for (size_t i = 0; i < thd_ecs_len; ++i)
	{
		if (thd_ecs[i] != 0)
		{
			log_http_message_with_ec(ec, HTTP_THD_POOL_THD_FN_ERR, i+1);
		}
	}
	free(thd_ecs);
	return http_thd_pool_thd_fn;
}

http_err_t thdpool_addjob(void (*callback)(void*), void* args, 
						void (*clean)(void*))
{
	int ec = 0;
	ctp_thdpool_add_job(thdpool, callback, args, clean, &ec);
	if (ec != 0)
	{
		log_http_message(HTTP_THD_POOL_ADD_JOB_ERR);
		return http_thd_pool_add_job;
	}

	return http_success;
}
