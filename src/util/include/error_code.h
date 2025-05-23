#pragma once
#include "ctp/logger.h"

typedef enum 
{
#	define HTTP_MSG(name, level, msg) name,
#	include "message.def"
# 	undef HTTP_MSG
} http_message_t;

typedef enum : int
{
	http_success = 0,
#	define HTTP_EC(ec, msg) ec,
#	include "http_ec.def"
#	undef HTTP_EC
} http_err_t;

const char* http_message_str(http_message_t n);
const char* http_message_str_with_prefix(http_message_t n);
ctp_log_level_t http_message_level(http_message_t n);
http_message_t http_err_to_msg(http_err_t ec);
const char* http_err_str(http_err_t ec);

void print_err_exit(http_err_t ec, const char* msg);

#define ERR_HANDLER_BOOL(ret, next, err_msg, ...)                              \
	do                                                                         \
	{                                                                          \
		if (!(ret))                                                            \
		{                                                                      \
			log_http_message(err_msg __VA_OPT__(, ) __VA_ARGS__);              \
			next;                                                              \
		}                                                                      \
	} while (0)

#define ERR_HANDLER_PTR(ret, next_receive, err_msg, ...)                       \
	ERR_HANDLER_BOOL(((ret) != nullptr), next_receive, err_msg,                \
					 __VA_OPT__(, ) __VA_ARGS__)

#define HERR_TRANSFER(herr, next_receive)                                      \
	do                                                                         \
	{                                                                          \
		if (herr != http_success)                                              \
		{                                                                      \
			next_receive;                                                      \
		}                                                                      \
	} while (0)
