#include "http_types.h"

#include "base.h"
#include <assert.h>
#include <stdlib.h>

// 添加方法时候需要注意和 http_method_t对应上
static const char* http_methods_str[] = {
	"GET",
	"HEAD",
	"POST",
	"PUT",
};

static const char* http_header_fields_str[] = {
#define HTTP_HDR(hdr, msg) msg,
#include "http_hdr.def"
#undef HTTP_HDR
};

/****
 * STATIC DECLARE
 ****/
static http_err_t http_hdr_value_parse_content_type(http_hdr_value_t* value);
static http_err_t http_hdr_value_parse_content_length(http_hdr_value_t* value);
static http_err_t http_hdr_value_parse_connection(http_hdr_value_t* value);
static http_err_t http_hdr_value_parse_transfer_encoding(http_hdr_value_t* value);

/****
 * INTERFACE IMPLAMENTATION
 ****/

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
		if (sv_buf[7] == '1')
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

http_err_t http_hdr_parse_value(http_header_t* header, string_view_t value_sv)
{
	if (header == nullptr)
	{
		log_http_message(HTTP_PARAM_INVALID_NULLPTR);
		return http_err_param_invalid_nullptr;
	}

	http_hdr_value_t* value = &header->value;
	value->value_sv = value_sv;
	value->has_parsed_type = true;
	http_err_t herr = http_success;
	switch(header->field)
	{
	case http_hdr_content_type:
		herr = http_hdr_value_parse_content_type(value);
		break;
	case http_hdr_content_length:
		herr = http_hdr_value_parse_content_length(value);
		break;
	case http_hdr_connection:
		herr = http_hdr_value_parse_connection(value);
		break;
	case http_hdr_transfer_encoding:
		herr = http_hdr_value_parse_transfer_encoding(value);
		break;
	default:
		value->has_parsed_type = false;
		break;
	}
	if (herr != http_success)
	{
		value->has_parsed_type = false;
		log_http_message_with_1sv(HTTP_PARSE_VALUE_ERR, value_sv);
		return http_err_parse_value;
	}

	return http_success;
}

/****
 * STATIC
 ****/
static http_err_t http_hdr_value_parse_content_type(http_hdr_value_t* value)
{
	string_view_t sv = value->value_sv;
    char buf[string_view_len(sv) + 1];
    string_view_to_buf(sv, buf, sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';

    // 跳过前导空格
    char* p = buf;
    while (*p == ' ' || *p == '\t') ++p;

    // 找到斜杠
    char* slash = strchr(p, '/');
    if (!slash) {
        value->content_type.major = http_content_type_major_unkown;
        value->content_type.sub = http_content_type_sub_unknown;
        return http_err_parse_unkown_content_type;
    }

    // 拆分主类型和子类型
    *slash = '\0';
    char* major_str = p;
    char* sub_str = slash + 1;

    // 去除主类型尾部空格
    char* end_major = major_str + strlen(major_str) - 1;
    while (end_major > major_str && (*end_major == ' ' || *end_major == '\t')) {
        *end_major-- = '\0';
    }
    // 去除子类型头部空格
    while (*sub_str == ' ' || *sub_str == '\t') ++sub_str;

    // 遇到参数分号截断
    char* semi = strchr(sub_str, ';');
    if (semi) *semi = '\0';

    // 识别 major
    if (strcmp(major_str, "text") == 0) {
        value->content_type.major = http_content_type_major_text;
    } else if (strcmp(major_str, "application") == 0) {
        value->content_type.major = http_content_type_major_application;
    } else if (strcmp(major_str, "image") == 0) {
        value->content_type.major = http_content_type_major_image;
    } else if (strcmp(major_str, "audio") == 0) {
        value->content_type.major = http_content_type_major_audio;
    } else if (strcmp(major_str, "video") == 0) {
        value->content_type.major = http_content_type_major_video;
    } else {
        value->content_type.major = http_content_type_major_unkown;
    }

    // 识别 sub
    if (strcmp(sub_str, "plain") == 0) {
        value->content_type.sub = http_content_type_sub_plain;
    } else if (strcmp(sub_str, "html") == 0) {
        value->content_type.sub = http_content_type_sub_html;
    } else if (strcmp(sub_str, "json") == 0) {
        value->content_type.sub = http_content_type_sub_json;
    } else if (strcmp(sub_str, "xml") == 0) {
        value->content_type.sub = http_content_type_sub_xml;
    } else if (strcmp(sub_str, "x-www-form-urlencoded") == 0) {
        value->content_type.sub = http_content_type_sub_x_www_form_urlencoded;
    } else if (strcmp(sub_str, "form-data") == 0) {
        value->content_type.sub = http_content_type_sub_form_data;
    } else if (strcmp(sub_str, "png") == 0) {
        value->content_type.sub = http_content_type_sub_png;
    } else if (strcmp(sub_str, "jpeg") == 0) {
        value->content_type.sub = http_content_type_sub_jpeg;
    } else if (strcmp(sub_str, "gif") == 0) {
        value->content_type.sub = http_content_type_sub_gif;
    } else {
        value->content_type.sub = http_content_type_sub_unknown;
    }

    value->has_parsed_type = true;

    // 只要major和sub有一个不是unknown就算成功
    if (value->content_type.major == http_content_type_major_unkown &&
        value->content_type.sub == http_content_type_sub_unknown)
	{
		log_http_message_with_1sv(HTTP_PARSE_UNKOWN_CONTENT_TYPE,
								  value->value_sv);
		return http_err_parse_unkown_content_type;
	}

    return http_success;
}

static http_err_t http_hdr_value_parse_content_length(http_hdr_value_t* value)
{
	string_view_t sv = value->value_sv;
	char buf[string_view_len(sv)+1];
	string_view_to_buf(sv, buf, sizeof(buf));
	
	char* p = buf;
	while(*p == ' ' || *p == '\t') ++p;

	char* endptr = nullptr;
	int64_t content_length = strtoll(p, &endptr, 10);
	if (p == endptr)
	{
		log_http_message(HTTP_PARSE_INVALID_CONTENT_LENGTH);
		return http_err_parse_invalid_content_length;
	}
	
	value->content.content_length = content_length;
	return http_success;
}

static http_err_t http_hdr_value_parse_connection(http_hdr_value_t* value)
{
	static char* value_strs[] = {
        "keep-alive",
        "close"
    };

    for (size_t i = 0; i < sizeof(value_strs) / sizeof(value_strs[0]); ++i)
    {
        if (string_view_compare_cstr(value->value_sv, value_strs[i]) == 0)
        {
            value->connection.type = i + 1; // 枚举从1开始
            return http_success;
        }
    }
    value->connection.type = http_connection_unkown;
	log_http_message_with_1sv(HTTP_PARSE_UNKOWN_CONNECTION_TYPE, value->value_sv);
    return http_err_parse_unkown_connection_type;
}

static http_err_t
http_hdr_value_parse_transfer_encoding(http_hdr_value_t* value)
{
	static char* value_strs[] = {
		"chunked"
		//"identity"
	};

	for (size_t i = 0; i < sizeof(value_strs) / sizeof(value_strs[0]); ++i)
	{
		if (string_view_compare_cstr(value->value_sv, value_strs[i]) == 0)
		{
			value->transfer_encoding.type = i + 1;
			return http_success;
		}
	}
	value->transfer_encoding.type = http_transfer_encoding_unkown;
	log_http_message_with_1sv(HTTP_PARSE_UNKOWN_TRANSFER_ENCODING, value->value_sv);
	return http_err_parse_unkown_transfer_encoding;
}

