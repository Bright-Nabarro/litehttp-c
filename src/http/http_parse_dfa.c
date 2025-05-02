#include "http_parse_dfa.h"

#include <assert.h>
#include <stdlib.h>
#include "base.h"
#include <string.h>
#include "static_test.h"

#include "http_parse_ctx_internal.h"

#define CLRF_ERR_HANDLE(herr, ctx, state)                                      \
	do                                                                         \
	{                                                                          \
		if (herr == http_err_parse_cannot_find_clrf)                           \
		{                                                                      \
			log_http_message(HTTP_PARSE_CANNOT_FIND_CLRF, #state);             \
			return herr;                                                       \
		}                                                                      \
	} while (0)

#define FATAL_ERR_HANDLE(herr)                                                 \
	do                                                                         \
	{                                                                          \
		if (herr == http_err_request_fatal)                                    \
		{                                                                      \
			log_http_message(HTTP_REQUEST_FATAL_ERR);                          \
			return herr;                                                       \
		}                                                                      \
	} while (0)

/****
 * 工具函数
 ****/
static const char* get_ctx_cur(const http_parse_ctx_t* ctx);
/// 返回超尾指针
static const char* get_ctx_end(const http_parse_ctx_t* ctx);
static const char* get_ctx_next(const http_parse_ctx_t* ctx);
static size_t get_ctx_line(const http_parse_ctx_t* ctx);
static http_err_t update_ctx_to_next_clrf(http_parse_ctx_t* ctx);

static http_err_t pos_next_token(string_view_t view, char c, size_t this_token,
								 size_t* cur_len, size_t* next);
[[maybe_unused]]
static size_t get_ctx_post_remain(const http_parse_ctx_t* ctx);
[[maybe_unused]]
static size_t get_ctx_line_remain(const char* cur, const http_parse_ctx_t* ctx);
static string_view_t get_ctx_focus_line(const http_parse_ctx_t* ctx);

/****
 * 解析函数
 ****/
UT_STATIC http_err_t parse_request_line(http_parse_ctx_t* ctx);
UT_STATIC http_err_t parse_header_line(http_parse_ctx_t* ctx);

/****
 * INTERFACE IMPLETATION
 ****/

http_err_t http_parse_ctx_new(http_parse_ctx_t** pctx)
{
	assert(pctx != nullptr);
	*pctx = calloc(1, sizeof(http_parse_ctx_t));
	ERR_HANDLER_PTR(*pctx, return http_err_malloc, HTTP_MALLOC_ERR);
	cc_init(&(*pctx)->header_list);
	ERR_HANDLER_PTR((*pctx)->header_list,
					{
						free(pctx);
						return http_err_malloc;
					},
					HTTP_MALLOC_ERR);

	http_err_t herr = http_request_new(&(*pctx)->http_request);
	return herr;
}

void http_parse_ctx_destroy_chain(http_parse_ctx_t** head)
{
	assert(head != nullptr);
	if (*head == nullptr)
		return;
	if ((*head)->next_ctx != nullptr)
		http_parse_ctx_destroy_chain(&(*head)->next_ctx);
	http_parse_ctx_free_node(*head);
	*head = nullptr;
}

http_err_t http_parse_feed(http_parse_ctx_t** ptail, string_view_t post)
{
	assert(ptail != nullptr);
	http_parse_ctx_t* ctx = *ptail;
	http_err_t herr = http_success;
	bool bsuccess = true;

	bsuccess = string_append_view(&ctx->post, post);
	ERR_HANDLER_BOOL(bsuccess, return http_err_malloc, HTTP_MALLOC_ERR);

	switch(ctx->wait_state)
	{
	case http_parse_start:
	case http_parse_wait_req: goto wait_req;
	case http_parse_wait_hdr: goto wait_hdr;
	case http_parse_wait_fix_body: goto wait_fix_body;
	case http_parse_wait_clen: goto wait_clen;
	case http_parse_wait_cval: goto wait_cval;
	case http_parse_end: goto end;
	}
	
wait_req:
	ctx->wait_state = http_parse_wait_req;
	herr = update_ctx_to_next_clrf(ctx);
	CLRF_ERR_HANDLE(herr, ctx, http_parse_start);
	ctx->request_line = get_ctx_focus_line(ctx);
	herr = parse_request_line(ctx);
	FATAL_ERR_HANDLE(herr);
wait_hdr:
	ctx->wait_state = http_parse_wait_hdr;
	size_t header_cnt = 1;
	for (; header_cnt <= 100; ++header_cnt)
	{
		herr = update_ctx_to_next_clrf(ctx);
		CLRF_ERR_HANDLE(herr, ctx, http_parse_start);
		if (get_ctx_line(ctx) == 0)
		{
			break;
		}
		string_view_t hdr_sv = get_ctx_focus_line(ctx);
		if (cc_push(&ctx->header_list, hdr_sv) == nullptr)
		{
			log_http_message(HTTP_MALLOC_ERR);
			return http_err_malloc;
		}
		herr = parse_header_line(ctx);
		FATAL_ERR_HANDLE(herr);
		
	}
	ERR_HANDLER_BOOL(header_cnt != 100, return http_err_request_fatal,
					 HTTP_REQUEST_HEADER_FILL_UP);
	switch(ctx->body_type)
	{
	case http_transfer_body_unset:
		ctx->post_full_length = ctx->next_line;
		goto end;
	case http_transfer_body_fixed:
		goto wait_fix_body;
	case http_transfer_body_chunked:
		goto wait_clen;
	}
wait_fix_body:
	ctx->wait_state = http_parse_wait_fix_body;
	// 下一行指向post尾部
	ctx->body_cur = 0;
	ctx->post_full_length = ctx->body_cur + ctx->body_length;
	// 出现拆包状况
	if (ctx->post_full_length > string_len(&ctx->post))
	{
		log_http_message(HTTP_REQUEST_TCP_SEGMENT_IN_BODY);
		return http_err_request_tcp_segment_in_body;
	}
	ctx->next_line += ctx->body_length;
	ctx->body_view = get_ctx_focus_line(ctx);
	assert(string_view_len(ctx->body_view) == ctx->body_length);
	
	ctx->http_request->body = ctx->body_view;
	goto end;
wait_clen:
	ctx->wait_state = http_parse_wait_clen;
	
wait_cval:
	ctx->wait_state = http_parse_wait_cval;
end:	
	assert(ctx->post_full_length == ctx->next_line);
	// 出现黏包状况
	if (ctx->next_line < ctx->post_full_length)
	{
		assert(ctx->post_full_length < string_len(&ctx->post));
		string_resize(&ctx->post, ctx->post_full_length);
		herr = http_parse_ctx_new(&ctx->next_ctx);
		if (herr != http_success)
			return http_err_request_fatal;
		size_t bias = ctx->body_length - ctx->post_full_length;
		ctx = ctx->next_ctx;
		bsuccess = string_init_from_string_view (
			&ctx->post, string_view_substr(post, bias, string_t_npos));
		ERR_HANDLER_BOOL(bsuccess, return http_err_malloc, HTTP_MALLOC_ERR);
		goto wait_req;
	}
	assert(ctx->post_full_length == string_len(&ctx->post));
	ctx->wait_state = http_parse_end;
	*ptail = ctx;
	return http_success;
}

bool http_parse_pop_request_and_release(http_parse_ctx_t** head,
                       http_request_t** out_req)
{
	assert(head != nullptr);
	assert(out_req != nullptr);
	if (*head == nullptr)
		return false;

	if ((*head)->wait_state != http_parse_end)
	{
		*out_req = nullptr;
		return false;
	}

	*out_req = (*head)->http_request;
	http_parse_ctx_t* tmp = *head;
	*head = (*head)->next_ctx;
	http_parse_ctx_free_node(tmp);
	return true;
}

void http_parse_ctx_free_node(http_parse_ctx_t* node)
{
	if (node == nullptr)
		return;
	http_request_free(node->http_request);
	cc_cleanup(&node->header_list);
	string_destroy(&node->post);
	free(node);
}

/****
 * Parse
 ****/
static http_err_t parse_request_line(http_parse_ctx_t* ctx)
{
	http_err_t herr = http_success;
	http_request_t* request = ctx->http_request;
	assert(request != nullptr);
	string_view_t request_line_sv = ctx->request_line;
	size_t cur_token = 0, token_len = 0, next_token = 0;
	// 方法
	herr = pos_next_token(request_line_sv, ' ', cur_token, &token_len,
						  &next_token);
	if (herr != http_success)
	{
		log_http_message(HTTP_REQUEST_LINE_INVALID);
		request->method = http_method_unkown;
		request->version = http_ver_unkown;
		return herr;
	}
	assert(next_token <= ctx->next_line);
	assert(next_token >= ctx->cur);

	string_view_t method_sv =
		string_view_substr(request_line_sv, cur_token, token_len);
	assert(string_view_len(method_sv) == token_len);
	http_method_t method = http_match_method(method_sv);
	if (method == http_method_unkown)
		log_http_message(HTTP_REQUEST_UNKOWN_METHOD);
	request->method = method;

	// 相对目录
	cur_token = next_token;
	herr = pos_next_token(request_line_sv, ' ', cur_token, &token_len,
						  &next_token);
	if (herr != http_success)
	{
		log_http_message(HTTP_REQUEST_LINE_INVALID);
		return herr;
	}
	assert(next_token <= ctx->next_line);
	assert(next_token >= ctx->cur);
	request->path = string_view_substr(request_line_sv, ctx->cur, token_len);

	// http版本
	cur_token = next_token;
	string_view_t version_sv =
		string_view_substr(request_line_sv, ctx->cur, string_t_npos);
	request->version = http_match_version(version_sv);
	if (request->version == http_ver_unkown)
	{
		log_http_message(HTTP_REQUEST_VERSION_INVALID);
	}
	
	return herr;
}

static http_err_t parse_header_line(http_parse_ctx_t* ctx)
{
	assert(ctx != nullptr);
	http_err_t herr = http_success;
	string_view_t header_sv = *cc_last(&ctx->header_list);
	size_t cur_token = ctx->cur, token_len = 0, next_token = 0;
	herr = pos_next_token(header_sv, ' ', cur_token, &token_len, &next_token);
	if (herr != http_success)
	{
		return herr;
	}
	assert(next_token >= ctx->cur);
	http_header_t new_header = {0};
	string_view_t key_sv = string_view_substr(header_sv, cur_token, token_len);
	new_header.field = http_match_header_field(key_sv);
	if (new_header.field == http_hdr_unkown)
		new_header.name = key_sv;
	
	cur_token = next_token;
	if (string_view_cstr(header_sv)[cur_token] == ' ')
		++cur_token;
	new_header.value = string_view_substr(header_sv, cur_token, string_t_npos);
	if (cc_push(&ctx->http_request->header_list, new_header) == nullptr)
	{
		log_http_message(HTTP_MALLOC_ERR);
		return http_err_malloc;
	}
	if (new_header.field == http_hdr_content_length)
	{
		char buf[string_view_len(new_header.value)+1];
		string_view_to_buf(new_header.value, buf, sizeof(buf));
		int body_length = atoi(buf);
		if (body_length == 0)
			return http_success;
		ctx->body_type = http_transfer_body_fixed;
		ctx->body_length = body_length;
		log_http_message(HTTP_REQUEST_FIXED_BODY_LEN, body_length);
		return http_err_request_fixed_body_len;
	}
	else if (new_header.field == http_hdr_transfer_encoding)
	{
		http_hdr_transfer_encoding_t encoding =
			http_match_transfer_encoding_value(new_header.value);
		
		switch(encoding)
		{
		case http_hdr_transfer_encoding_chunked:
			ctx->body_type = http_transfer_body_chunked;
			return http_err_request_chunked_body;
		default:
			return http_success;
		}
	}
	return http_success;
}

/****
 * STATIC
 ****/

static const char* get_ctx_cur(const http_parse_ctx_t* ctx)
{
	return &string_cstr(&ctx->post)[ctx->cur];
}

static const char* get_ctx_end(const http_parse_ctx_t* ctx)
{
	return &string_cstr(&ctx->post)[string_len(&ctx->post)];
}

static const char* get_ctx_next(const http_parse_ctx_t* ctx)
{
	return &string_cstr(&ctx->post)[ctx->next_line];
}

static size_t get_ctx_line(const http_parse_ctx_t* ctx)
{
	assert(ctx->next_line >= ctx->cur+2);
	return ctx->next_line - ctx->cur - 2;
}

static http_err_t update_ctx_to_next_clrf(http_parse_ctx_t* ctx)
{
	const char* next = get_ctx_next(ctx);
	const char* end = get_ctx_end(ctx);
	for (size_t i = 0; next + i + 1 < end; ++i)
	{
		if (*(next+i) == '\r' && *(next+i+1) == '\n')
		{
			ctx->cur = ctx->next_line;
			ctx->next_line = ctx->next_line+i+2;
			return http_success;
		}
	}
	return http_err_parse_cannot_find_clrf;
}

static http_err_t pos_next_token(string_view_t view, char c, size_t this_token,
								 size_t* cur_len, size_t* next)
{
	const char* beg = string_view_cstr(view);
	size_t sv_len = string_view_len(view);
	for (size_t i = this_token; i < sv_len; ++i)
	{
		if (beg[i] == c)
		{
			*next = i+1;
			*cur_len = i - this_token;
			return http_success;
		}
	}
	log_http_message(HTTP_PARSE_CANNOT_FIND_SPACE);
	return http_err_parse_cannot_find_space;
}

static size_t get_ctx_line_remain(const char* cur, const http_parse_ctx_t* ctx)
{
	const char* next_ptr = get_ctx_next(ctx);
	assert(cur != nullptr && next_ptr != nullptr);
	assert(next_ptr > cur);
	assert(*(next_ptr-2) == '\r' && *(next_ptr-1) == '\n');
	assert(next_ptr - cur - 2 >= 0);
	return next_ptr - cur - 2;
}

static string_view_t get_ctx_focus_line(const http_parse_ctx_t* ctx)
{
	const char* cur = get_ctx_cur(ctx);
	const char* next = get_ctx_next(ctx);
	assert(next >= cur +2);
	return string_view_from_parts(cur, next - cur - 2);
}

static size_t get_ctx_post_remain(const http_parse_ctx_t* ctx)
{
	size_t post_len = string_len(&ctx->post);
	assert(post_len >= ctx->cur);
	return post_len - ctx->cur;
}
