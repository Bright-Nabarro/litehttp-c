#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "error_code.h"

const char* http_message_str(http_message_t n)
{
#define HTTP_MSG(name, level, msg)                                             \
	case name:                                                                 \
		return msg;
	switch (n)
	{
#include "message.def"
	}
#undef HTTP_MSG
	assert(false);
}

const char* http_message_str_with_prefix(http_message_t n)
{
#define HTTP_MSG(name, level, msg)                                             \
	case name:                                                                 \
		return "%s " msg;
	switch (n)
	{
#include "message.def"
	}
#undef HTTP_MSG
	assert(false);
}

ctp_log_level_t http_message_level(http_message_t n)
{
#define HTTP_MSG(name, level, msg)                                             \
	case name:                                                                 \
		return ctp_##level;
	switch (n)
	{
#include "message.def"
	}
#undef HTTP_MSG
	assert(false);
}

http_message_t http_err_to_msg(http_err_t ec)
{
#define HTTP_EC(ec, name)                                                      \
	case ec:                                                                   \
		return name;
	switch (ec)
	{
	case http_success:
		return 0;
#include "http_ec.def"
	}
#undef HTTP_EC
	assert(false);
}

const char* http_err_str(http_err_t ec)
{
	switch (ec)
	{
	case http_success:
		return "Success";
#define HTTP_MSG(name) msg
#define HTTP_EC(ec, name)                                                      \
	case ec:                                                                   \
		return http_message_str(name);
#include "http_ec.def"
#undef HTTP_EC
	}
	assert(false);
}

void print_err_exit(http_err_t ec, const char* msg)
{
	printf("%s %s\n", http_err_str(ec), msg);
	_exit(1);
}

