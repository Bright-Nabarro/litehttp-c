#pragma once
#include "error_code.h"
#include "string_t.h"
#include "http_types.h"
#include <cc_wrap.h>

// 发送前需要检测各个字符串是否为空，如果是空，则不发送
typedef struct
{
	http_method_t method;
	http_version_t version;
	http_status_t status;
	cc_vec(http_header_t) header_list;
	string_view_t body;
} http_response_t;

http_err_t http_response_new(http_response_t** rep);

void http_response_free(http_response_t* rep);

http_err_t http_response_to_string(const http_response_t* rep, string_t* post);

string_view_t http_get_simple_response();

http_err_t http_response_add_header(http_response_t* response,
									http_header_field_t key,
									string_view_t value);
