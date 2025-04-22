#pragma once
#include "string_t.h"
#include "error_code.h"

// 请求方法
typedef enum: size_t
{
	http_method_get = 0,
	http_method_head,
	http_method_post,
	http_method_max_known,
	http_method_unkown,
} http_method_t;


typedef enum
{
	http_ver_1_0,
	http_ver_1_1,
	http_ver_2_0,
	http_ver_3_0,
	http_ver_unkown,
} http_version_t;

typedef enum
{
	http_hdr_host,
	http_hdr_user_agent,
	http_hdr_accept,
	http_hdr_content_type,
	http_hdr_connection,
	http_hdr_other,
	http_hdr_max,
} http_header_field_t;

typedef struct 
{
	http_header_field_t field;
	string_view_t value;		// 头部值
	string_view_t name;			// 只有 field == http_hdr_other 时填充
} http_header_t;

typedef struct 
{
	http_method_t method;
	string_view_t path;
	http_version_t version;
	http_header_t* headers;
	size_t header_count;
	string_view_t body;
} http_request_t;


const char** get_http_methods();
size_t get_http_methods_len();

// 所有的字符串又外部管理，确保外部的生命周期比 相关所有http解析声明周期长
http_request_t* http_request_alloc();
void http_request_free(http_request_t* req);

http_err_t http_request_set_body(http_request_t* req, string_view_t body);

// 添加请求头到 http_request_t
http_err_t http_request_add_header(http_request_t* req,
								   http_header_field_t field,
								   string_view_t value);

// 添加暂不支持的请求头
http_err_t http_request_add_header_other(http_request_t* req,
										 string_view_t name,
										 string_view_t value);

