#pragma once
#define CC_NO_SHORT_NAME
#include "http_types.h"
#include "cc.h"

// 所有的字符串由外部管理，确保外部的生命周期比 相关所有http解析声明周期长
typedef struct 
{
	http_method_t method;
	string_view_t path;
	http_version_t version;
	cc_vec(http_header_t) header_list;
	string_view_t body;
} http_request_t;

http_err_t http_request_new(http_request_t** req);

void http_request_free(http_request_t* req);

http_err_t http_request_set_body(http_request_t* req, string_view_t body);

