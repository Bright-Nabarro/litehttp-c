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
#define HTTP_HDR(hdr, msg) hdr,
#include "http_hdr.def"
#undef HTTP_HDR
	http_hdr_max,
} http_header_field_t;

typedef struct 
{
	http_header_field_t field;
	string_view_t value;		// 头部值
	string_view_t name;			// 只有 field == http_hdr_other 时填充
} http_request_header_t;



typedef struct
{
	int code;
	string_view_t message;
} http_status_t;


const char** get_http_methods();
size_t get_http_methods_len();


http_err_t header_initial(http_request_header_t** header, size_t* header_size,
								 size_t* capacity);


http_err_t header_push_back_empty(http_request_header_t** header, size_t* header_size,
								  size_t* capacity);

http_err_t http_set_status(int code, http_status_t* status);

bool http_get_status_sv(int code, char* buf);
string_view_t http_get_status_msg(int code);

string_view_t http_get_version_sv(http_version_t version);

string_view_t http_get_header_field_sv(http_header_field_t method);

