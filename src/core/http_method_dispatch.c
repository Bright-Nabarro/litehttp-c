#include <assert.h>
#include "http_method_dispatch.h"
#include "resource_getter.h"
#include "base.h"
#include "http_response.h"
#include "http_types.h"
#include "socket_util.h"
#include <stdio.h>

http_err_t handle_http_request(const http_request_t* request,
								   http_response_t** res)
{
	assert(res != nullptr);
	switch(request->method)
	{
	case http_method_get:
		http_method_get_handler(request, res);
		break;
	default:
		log_http_message(HTTP_REQUEST_UNKOWN_METHOD);
		return http_err_request_unkown_method;
	}
	return http_success;
}

http_err_t http_method_get_handler(const http_request_t* request,
								   http_response_t** res)
{
	assert(request->method == http_method_get);
	string_view_t html_post = {0};
	string_t path = {0};
	http_err_t herr = http_success;
	http_status_t status = {0};
	log_debug_message("postfix path: %.*s", (int)string_len(&request->path),
					  string_cstr(&request->path));
	herr = get_absolue_path(string_view_from_string(&request->path), &path);
	HERR_TRANSFER(herr, return herr);
	herr = get_resource_cached(string_view_from_string(&path), &html_post);
	/*********************************************
	 * 这里应该返回 404
	 *********************************************/
	HERR_TRANSFER(herr, return herr);
	herr = http_set_status(200, &status);
	HERR_TRANSFER(herr, return herr);

	http_response_t* response = nullptr;
	herr = http_response_new(&response);
	response->status = status;
	HERR_TRANSFER(herr, return herr);
	// 添加编码头部
	herr = http_response_add_header(response, http_hdr_content_type,
							 string_view_from_cstr("text/html; charset=UTF-8"));
	HERR_TRANSFER(herr, return herr);
	// 添加Content-Length头部
	char numbuf[16];
	snprintf(numbuf, sizeof(numbuf), "%zu", string_view_len(html_post));
	http_response_add_header(response, http_hdr_content_length,
							 string_view_from_cstr(numbuf));
	response->body = html_post;
	*res = response;
	return http_success;
}

