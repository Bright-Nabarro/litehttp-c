#include "http_parser.h"
#include "base.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
    // 原始HTTP报文数据
	string_view_t raw_data;

    // 解析进度指针
    const char* cur;            // 当前解析到的位置
	const char* next;
	size_t cur_line_len;		// 每行长度，不带 /r/n
	size_t remaining_len;		// 解析剩余长度

    // 各部分的指针和长度
	string_view_t request_line;

	string_view_t* headers;
    size_t headers_len;

	string_view_t body;

    http_err_t last_err;
} http_parse_ctx_t;

// 解析整个HTTP请求
static
http_err_t parse_http_request_ctx(http_parse_ctx_t* ctx, http_request_t* request);
// 解析请求行，如 "GET /index.html HTTP/1.1"
static
http_err_t parse_request_line(http_parse_ctx_t* ctx, http_request_t* request);
// 解析单个请求头行
static
http_err_t parse_header_line(http_parse_ctx_t* ctx, http_request_header_t* ret);
// 解析所有请求头
static
http_err_t parse_body(http_parse_ctx_t* ctx, http_request_t* request);

/**
 * @brief 工具函数：查找/r/n
 * @param line_len 解析每行长度，不包括/r/n
 * @return 如果出错, 在len范围内没有查找到，则返回空指针
 */
static http_err_t find_next_clrf(const char* beg, size_t maxlen,
								const char** ret, size_t* line_len);

static http_err_t find_next_char(char c, const char* beg, size_t maxlen,
								 const char** ret, size_t* part_len);

static void update_ctx_remaining(http_parse_ctx_t* ctx);

static
http_header_field_t parse_header2header_field(string_view_t header_sv);

/****
 * INTERFACE IMPLEMENTAT
 ****/
http_err_t parse_http_request(string_view_t post, http_request_t* request)
{
	http_err_t herr = http_success;

	http_parse_ctx_t context;
	memset(&context, 0, sizeof(context));
	context.raw_data = post;

	herr = parse_http_request_ctx(&context, request);
	if (herr != http_success)
		return herr;
	return http_success;
}


/****
 * STATIC IMPLEMENT
 ****/
static
http_err_t parse_http_request_ctx(http_parse_ctx_t* ctx, http_request_t* request)
{
	http_err_t herr = http_success;
	ctx->cur = string_view_cstr(ctx->raw_data);
	ctx->remaining_len = string_view_len(ctx->raw_data);

	// 解析 http 请求行
	herr = find_next_clrf(ctx->cur, ctx->remaining_len, &ctx->next,
						 &ctx->cur_line_len);
	if (herr != http_success)
		return herr;
	update_ctx_remaining(ctx);

	herr = parse_request_line(ctx, request);
	if (herr != http_success
	 && herr != http_request_unkown_method)
		return herr;
	ctx->cur = ctx->next;
	
	// 解析 http 首部
	size_t header_arr_len = 10;
	http_request_header_t* header_arr = malloc(sizeof(http_request_header_t) * header_arr_len);
	if (header_arr == nullptr)
	{
		log_http_message_with_errno(HTTP_MALLOC_ERR);
		return http_malloc;
	}

	size_t i;
	for(i = 0;; ++i)
	{
		if (i == header_arr_len)
		{
			header_arr_len *= 2;
			http_request_header_t* new_hdr =
				realloc(header_arr, header_arr_len * sizeof(http_request_header_t));
			if (new_hdr == nullptr)
			{
				free(header_arr);
				log_http_message_with_errno(HTTP_REALLOC_ERR);
				return http_realloc_err;
			}
			header_arr = new_hdr;
		}

		herr = find_next_clrf(ctx->cur, ctx->remaining_len, &ctx->next,
							  &ctx->cur_line_len);
		if (herr != http_success)
			return herr;
		if (ctx->cur_line_len == 0)
		{
			assert(ctx->remaining_len == 2);
			log_debug_message("Header context end");
			break;
		}

		update_ctx_remaining(ctx);
		herr = parse_header_line(ctx, &header_arr[i]);
		ctx->cur = ctx->next;
	}
	http_request_header_t* new_hdr = realloc(header_arr, (i+1) * sizeof(http_request_header_t));
	if (new_hdr == nullptr)
	{
		free(header_arr);
		log_http_message_with_errno(HTTP_REALLOC_ERR);
		return http_realloc_err;
	}
	
	request->headers = new_hdr;
	request->header_count = i;
	
	// 解析Http请求报文段
	return herr;
}

static
http_err_t parse_request_line(http_parse_ctx_t* ctx, http_request_t* request)
{
	assert(request != nullptr);
	http_err_t herr = http_success;
	size_t line_remain_len = ctx->cur_line_len;
	size_t part_len;
	const char* next;
	// 解析 http request 的方法
	herr = find_next_char(' ', ctx->cur, line_remain_len, &next, &part_len);
	if (herr != http_success)
	{
		request->method = http_method_unkown;
		request->version = http_ver_unkown;
		return herr;
	}
	assert(part_len + 1 == (size_t)(next - ctx->cur));
	line_remain_len -= (next - ctx->cur);

	const char** methods_str = get_http_methods();
	size_t methods_size = get_http_methods_len();
	assert(methods_size == http_method_max_known);
	http_method_t method = http_method_unkown;

	for (size_t i = 0; i < methods_size; ++i)
	{
		size_t method_len = strlen(methods_str[i]);
		if (method_len > part_len)
		{
			continue;
		}

		if (strncmp(ctx->cur, methods_str[i], method_len) == 0)
		{
			assert(i < http_method_max_known);
			method = i;
		}
	}
	if (method == http_method_unkown)
	{
		herr = http_request_unkown_method;
		log_http_message(HTTP_REQUEST_UNKOWN_METHOD);
	}
	request->method = method;
	
	// 移动到下一个: 目录
	ctx->cur = next;
	herr = find_next_char(' ', ctx->cur, line_remain_len, &next, &part_len);
	if (herr != http_success)
		return herr;
	assert(part_len + 1 == (size_t)(next - ctx->cur));
	line_remain_len -= (next - ctx->cur);
	request->path = string_view_from_parts(ctx->cur, part_len);

	// 移动到下一个: http版本
	ctx->cur = next;
	const static char http_ver_prefix[] = "HTTP/";

	if (line_remain_len < 6 || strncmp(ctx->cur, http_ver_prefix, 5) != 0)
	{
		log_http_message(HTTP_REQUEST_VERSION_INVALID);
		return http_request_version_invalid;
	}

	switch(*(ctx->cur+5))
	{
	case '1':
		if (line_remain_len != 8
		 || *(ctx->cur+6) != '.'
		 || (*(ctx->cur+7) != '1' && *(ctx->cur+7) != '0'))
		{
			log_http_message(HTTP_REQUEST_VERSION_INVALID);
			return http_request_version_invalid;
		}
		if (*(ctx->cur+7) == '1')
			request->version = http_ver_1_1;
		else
			request->version = http_ver_1_0;
		break;
	case '2':
		request->version = http_ver_2_0;
		break;
	case '3':
		request->version = http_ver_3_0;
		break;
	default:
		log_http_message(HTTP_REQUEST_VERSION_INVALID);
		return http_request_version_invalid;
	}

	return herr;
}

static
http_err_t parse_header_line(http_parse_ctx_t* ctx, http_request_header_t* header_ret)
{
	assert(header_ret != nullptr);
	size_t cur_remaining = ctx->cur_line_len;
	http_err_t herr;
	const char* next = nullptr;
	size_t part_len = 0;
	// 解析 http head 键
	herr = find_next_char(':', ctx->cur, cur_remaining, &next, &part_len);
	if (herr != http_success)
		return herr;
	assert(part_len != 0);
	string_view_t header_sv = string_view_from_parts(ctx->cur, part_len);
	header_ret->field = parse_header2header_field(header_sv);
	if (header_ret->field == http_hdr_other)
	{
		char buf[string_view_len(header_sv) + 1];
		[[maybe_unused]] bool ret = string_view_to_buf(header_sv, buf, sizeof(buf));
		assert(ret);
		log_http_message(HTTP_REQUEST_UNKOWN_HEADER, buf);
		header_ret->name = header_sv;
	}
	else
	{
		header_ret->name = string_view_from_cstr("");
	}
	
	// 解析 http head 值
	cur_remaining -= next - ctx->cur;
	ctx->cur = next;
	if (*(ctx->cur) == ' ')
	{
		++ctx->cur;
		--cur_remaining;
	}
	assert(*(ctx->cur+cur_remaining) == '\r');
	assert(*(ctx->cur+cur_remaining+1) == '\n');
	header_ret->value = string_view_from_parts(ctx->cur, cur_remaining);
	return http_success;
}


/****
 * TOOLS
 ****/

static http_err_t find_next_clrf(const char* beg, size_t maxlen,
								const char** ret, size_t* line_len)
{
	assert(beg != nullptr);
	assert(line_len != nullptr);
	size_t i;
	for (i = 0; i < maxlen - 1; ++i)
	{
		if (beg[i] == '\r' && beg[i+1] == '\n')
		{
			*line_len = i;
			*ret = beg + i+2;
			return http_success;
		}
	}
	*ret = nullptr;
	*line_len = 0;
	log_http_message(HTTP_PARSE_CANNOT_FIND_SPLIT);
	return http_parse_cannot_find_split;
}

static http_err_t find_next_char(char c, const char* beg, size_t maxlen,
								 const char** ret, size_t* part_len)
{
	assert(ret != nullptr);
	assert(beg != nullptr);
	assert(part_len != nullptr);
	for (size_t i = 0; i < maxlen; ++i)
	{
		if (beg[i] == c)
		{
			*ret = beg + i + 1;
			*part_len = i;
			return http_success;
		}
	}
	log_http_message(HTTP_PARSE_CANNOT_FIND_SPLIT, c);
	return http_parse_cannot_find_space;
}

static
void update_ctx_remaining(http_parse_ctx_t* ctx)
{
	assert(ctx->next > ctx->cur);
	assert(*(ctx->next-2) == '\r');
	assert(*(ctx->next-1) == '\n');
	ctx->remaining_len -= (ctx->next - ctx->cur);
}

static
http_header_field_t parse_header2header_field(string_view_t header_sv)
{
	const static char* headers_str[] = {
		"Host",
    	"User-Agent",
    	"Accept",
    	"Content-Type",
    	"Connection",
	};

	for (size_t i = 0; i < sizeof(headers_str) / sizeof(headers_str[0]); ++i)
	{
		if (string_view_compare_cstr(header_sv, headers_str[i]) == 0)
			return i;
	}

	return http_hdr_other;
}

