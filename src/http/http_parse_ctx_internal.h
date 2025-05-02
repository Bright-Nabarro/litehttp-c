#pragma once
#include "string_t.h"
#include "cc_wrap.h"
#include "http_request.h"

typedef enum 
{
	http_parse_start = 0,
	http_parse_wait_req,
	http_parse_wait_hdr,
	http_parse_wait_fix_body,
	http_parse_wait_clen,
	http_parse_wait_cval,
	http_parse_end,
} http_parse_wait_state_t;

typedef enum
{
	http_transfer_body_unset = 0,
	http_transfer_body_fixed,
	http_transfer_body_chunked,
} http_transfer_body_type;

typedef struct http_parse_ctx_t
{
	http_parse_wait_state_t wait_state;
	string_t post;
	size_t post_full_length;

	// 解析进度指针
	size_t cur;
	size_t next_line; // 下一行起始位置

	// 各部分分段
	string_view_t request_line;
	cc_vec(string_view_t) header_list;

	http_transfer_body_type body_type;
	size_t body_length;
	size_t body_cur;
	string_view_t body_view;

	http_request_t* http_request;
	struct http_parse_ctx_t* next_ctx;

} http_parse_ctx_t;

