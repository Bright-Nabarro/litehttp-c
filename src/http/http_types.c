#include "http_types.h"

#include "base.h"
#include <assert.h>
#include <stdlib.h>


// 添加方法时候需要注意和 http_method_t对应上
static const char* http_methods[] = {
	"GET",
	"HEAD",
	"POST",
};

const char** get_http_methods()
{
	return http_methods;
}

size_t get_http_methods_len()
{
	return sizeof(http_methods) / sizeof(http_methods[0]);
}

string_view_t http_get_header_field_sv(http_header_field_t method)
{
	const char* cstr = "";
	switch (method)
	{
#define HTTP_HDR(hdr, msg)                                                     \
	case (hdr):                                                                \
		cstr = msg;
#include "http_hdr.def"
#undef HTTP_HDR
	default:
	}
	return string_view_from_cstr(cstr);
}

http_err_t header_initial(http_request_header_t** header, size_t* header_size,
								 size_t* capacity)
{
	assert(header != nullptr);
	*capacity = 10;
	*header_size = 0;
	*header = calloc(*capacity, sizeof(http_request_header_t));
	if (*header == nullptr)
	{
		log_http_message_with_errno(HTTP_MALLOC_ERR);
		return http_malloc;
	}

	return http_success;
}

http_err_t header_push_back_empty(http_request_header_t** header,
		size_t* header_size, size_t* capacity)
{
	assert(header != nullptr);
	if (*header_size == *capacity)
	{
		*capacity *= 2;
		http_request_header_t* new_hdr =
			realloc(*header, *capacity * sizeof(http_request_header_t));
		if (new_hdr == nullptr)
		{
			log_http_message_with_errno(HTTP_REALLOC_ERR);
			return http_realloc_err;
		}
		*header = new_hdr;
	}

	++*header_size;
	assert(*header_size <= *capacity);
	return http_success;
}

http_err_t http_set_status(int code, http_status_t* status)
{
	status->code = code;
	status->message = http_get_status_msg(code);
	if (string_view_empty(status->message))
	{
		log_http_message(HTTP_GET_UNKOWN_STATUS);
		return http_get_unkown_status;
	}

	return http_success;
}

bool http_get_status_sv(int code, char* buf)
{
	if (code > 600 || code < 100)
		return false;
	buf[0] = code / 100 + '0';
	buf[1] = code / 10 % 10 + '0';
	buf[2] = code % 10 + '0';
	buf[3] = 0;
	return true;
}

string_view_t http_get_status_msg(int code)
{
	char* cstr = nullptr;
	switch (code)
	{
#define HTTP_STATUS(status, msg)                                               \
	case (status):                                                             \
		cstr = msg;                                                            \
		break;
#include "http_status.def"
#undef HTTP_STATUS
	default:
		cstr = "";
	}
	return string_view_from_cstr(cstr);
}

string_view_t http_get_version_sv(http_version_t version)
{
	char* cstr = nullptr;
	switch(version)
	{
	case http_ver_1_0:
		cstr = "HTTP/1.0";
		break;
	case http_ver_1_1:
		cstr = "HTTP/1.1";
		break;
	case http_ver_2_0:
		cstr = "HTTP/2.0";
		break;
	case http_ver_3_0:
		cstr = "HTTP/2.0";
		break;
	default:
		cstr = "";
	}
	return string_view_from_cstr(cstr);
}

/****
 * STATIC
 ****/


