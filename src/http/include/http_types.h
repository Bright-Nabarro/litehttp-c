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
#define HTTP_HDR_FIRST(hdr, msg) hdr = 0,
#include "http_hdr.def"
#undef HTTP_HDR_FIRST
#undef HTTP_HDR
	http_hdr_max,
	http_hdr_unkown,
} http_header_field_t;

typedef struct 
{
	http_header_field_t field;
	string_view_t value;		// 头部值
	string_view_t name;			// 只有 field == http_hdr_other 时填充
} http_header_t;

typedef enum
{
	http_hdr_transfer_encoding_chunked = 0,
	http_hdr_transfer_encoding_identity,
	http_hdr_transfer_encoding_unkown,
} http_hdr_transfer_encoding_t;

typedef struct
{
	int code;
	string_view_t message;
} http_status_t;


const char** http_get_methods();
size_t http_get_methods_len();

http_err_t header_initial(http_header_t** header, size_t* header_size,
								 size_t* capacity);

http_method_t http_match_method(string_view_t sv);

http_version_t http_match_version(string_view_t sv);

http_header_field_t http_match_header_field(string_view_t sv);

http_hdr_transfer_encoding_t
http_match_transfer_encoding_value(string_view_t sv);

http_err_t http_header_push_back_empty(http_header_t** header, size_t* header_size,
								  size_t* capacity);

http_err_t http_set_status(int code, http_status_t* status);

bool http_get_status_sv(int code, char* buf);

string_view_t http_get_status_msg(int code);

string_view_t http_get_version_sv(http_version_t version);

string_view_t http_get_header_field_sv(http_header_field_t method);


