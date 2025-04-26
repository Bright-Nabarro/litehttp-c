#pragma once
#include "http_types.h"

// 所有的字符串由外部管理，确保外部的生命周期比 相关所有http解析声明周期长
typedef struct 
{
	http_method_t method;
	string_view_t path;
	http_version_t version;
	http_request_header_t* headers;
	size_t header_count;
	string_view_t body;
} http_request_t;

http_err_t http_request_set_body(http_request_t* req, string_view_t body);

// 添加请求头到 http_request_t
http_err_t http_request_add_header(http_request_t* req, size_t* capacity,
								   http_header_field_t field,
								   string_view_t value);

// 添加暂不支持的请求头
http_err_t http_request_add_header_other(http_request_t* req, size_t* capacity,
										 string_view_t name,
										 string_view_t value);

