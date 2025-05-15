#pragma once

#include <errno.h>
#include <string.h>
#include "string_t.h"

#include "error_code.h"

//#define err_handler

void log_http_message(http_message_t name, ...);

#define log_debug_message(fmt, ...)                                            \
	ctp_logger_global_log(ctp_debug, fmt __VA_OPT__(, ) __VA_ARGS__);

#define log_http_message_with_errno(name, ...)                                 \
	log_http_message_with_ec(errno, name __VA_OPT__(, ) __VA_ARGS__)

#define log_http_message_with_ec(ec, name, ...)                                \
	do                                                                         \
	{                                                                          \
		char buf[512];                                                         \
		strerror_r((ec), buf, sizeof(buf));                                    \
		ctp_logger_global_log(http_message_level(name),                        \
							  http_message_str_with_prefix(name),              \
							  buf __VA_OPT__(, ) __VA_ARGS__);                 \
	} while (0)

#define log_http_message_with_1sv(name, sv)                                    \
	do                                                                         \
	{                                                                          \
		char buf[string_view_len(sv) + 1];                                     \
		string_view_to_buf((sv), buf, sizeof(buf));                            \
		log_http_message(name, buf);                                           \
	} while (0)

http_err_t http_signal(int signo, void(*fn)(int));
