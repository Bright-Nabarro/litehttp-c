#pragma once

#include "cc_wrap.h"
#include "string_t.h"
#include "error_code.h"
#include "http_request.h"


struct http_parse_ctx_t;
typedef struct http_parse_ctx_t http_parse_ctx_t;

/**
 * @brief 创建一个空的解析节点 (首节点 / 新尾节点)
 */
http_err_t http_parse_ctx_new(http_parse_ctx_t** pctx);


/**
 * @brief 释放整个链表
 * @note 会释放ctx持有的request, 仅在紧急状态下使用
 */
void http_parse_ctx_destroy_chain(http_parse_ctx_t** head);

/**
 * @brief 解析post, 放入ctx链表尾部
 */
http_err_t http_parse_feed(http_parse_ctx_t** tail, string_view_t post);

/**
 * @brief 弹出request并删除持有request的节点, request节点需要手动管理 
 */
bool http_parse_pop_request_and_release(http_parse_ctx_t** head,
                       http_request_t** out_req);

/**
 * @brief 释放整个节点，包括包含的request
 * @note 需要手动维护链表，如果链表有多个元素，需要提前记录next
 */
void http_parse_ctx_free_node(http_parse_ctx_t* node);

#ifdef UNIT_TEST

http_err_t parse_request_line(http_parse_ctx_t* ctx);
http_err_t parse_header_line(http_parse_ctx_t* ctx);

#endif
