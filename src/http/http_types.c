#include "http_types.h"

#include "base.h"
#include <assert.h>
#include <stdlib.h>


// 添加方法时候需要注意和 http_method_t对应上
static const char* http_methods_str[] = {
	"GET",
	"HEAD",
	"POST",
};

static const char* http_header_fields_str[] = {
#define HTTP_HDR(hdr, msg) msg,
#include "http_hdr.def"
#undef HTTP_HDR
};

const char** http_get_methods()
{
	return http_methods_str;
}

size_t http_get_methods_len()
{
	return sizeof(http_methods_str) / sizeof(http_methods_str[0]);
}

string_view_t http_get_header_field_sv(http_header_field_t method)
{
	const char* cstr = "";
	switch (method)
	{
#define HTTP_HDR(hdr, msg)                                                     \
	case (hdr):                                                                \
		cstr = msg;                                                            \
		break;
#include "http_hdr.def"
#undef HTTP_HDR
	default:
	}
	return string_view_from_cstr(cstr);
}

http_err_t header_initial(http_header_t** header, size_t* header_size,
								 size_t* capacity)
{
	assert(header != nullptr);
	*capacity = 10;
	*header_size = 0;
	*header = calloc(*capacity, sizeof(http_header_t));
	if (*header == nullptr)
	{
		log_http_message_with_errno(HTTP_MALLOC_ERR);
		return http_err_malloc;
	}

	return http_success;
}

http_err_t http_header_push_back_empty(http_header_t** header,
		size_t* header_size, size_t* capacity)
{
	assert(header != nullptr);
	if (*header_size == *capacity)
	{
		*capacity *= 2;
		http_header_t* new_hdr =
			realloc(*header, *capacity * sizeof(http_header_t));
		if (new_hdr == nullptr)
		{
			log_http_message_with_errno(HTTP_REALLOC_ERR);
			return http_err_realloc;
		}
		*header = new_hdr;
	}

	++*header_size;
	assert(*header_size <= *capacity);
	return http_success;
}

http_method_t http_match_method(string_view_t sv)
{
	for (size_t i = 0; i < sizeof(http_methods_str) / sizeof(http_methods_str[0]); ++i)
	{
		if (string_view_compare_cstr(sv, http_methods_str[i]) == 0)
		{
			return i;
		}
	}
	return http_method_unkown;
}

http_version_t http_match_version(string_view_t sv)
{
	static const char prefix[] = "HTTP/";
	
	const char* sv_buf = string_view_cstr(sv);
	if (string_view_len(sv) < 6
	 || strncmp(prefix, sv_buf, 5) != 0)
	{
		return http_ver_unkown;
	}

	switch(sv_buf[5])
	{
	case '1':
		if (string_view_len(sv) != 8
		 || sv_buf[6] != '.'
		 || (sv_buf[7] != '1' && sv_buf[7] != '0'))
			return http_ver_unkown;
		if (sv_buf[7] != '1')
			return http_ver_1_1;
		else
			return http_ver_1_0;
	case '2':
		return http_ver_2_0;
	case '3':
		return http_ver_3_0;
	default:
		return http_ver_unkown;
	}
}

http_header_field_t http_match_header_field(string_view_t sv)
{
	for (size_t i = 0; i < (size_t)http_hdr_max; ++i)
	{
		if (string_view_compare_cstr(sv, http_header_fields_str[i]) == 0)
		{
			return (http_header_field_t)i;
		}
	}
	return http_hdr_unkown;
}

http_err_t http_set_status(int code, http_status_t* status)
{
	status->code = code;
	status->message = http_get_status_msg(code);
	if (string_view_empty(status->message))
	{
		log_http_message(HTTP_GET_UNKOWN_STATUS);
		return http_err_get_unkown_status;
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
		cstr = "HTTP/3.0";
		break;
	default:
		cstr = "";
	}
	return string_view_from_cstr(cstr);
}

http_hdr_transfer_encoding_t
http_match_transfer_encoding_value(string_view_t sv)
{
	static char* value[] = {"chunked"
						   "identity"};
	for (size_t i = 0; i < sizeof(value) / sizeof(value[0]); ++i)
	{
		if (string_view_compare_cstr(sv, value[i]) == 0)
			return i;
	}
	return http_hdr_transfer_encoding_unkown;
}

/****
 * STATIC
 ****/


