#include <assert.h>
#include "http_method_dispatch.h"
#include "resource_getter.h"
#include "base.h"
#include "http_response.h"
#include <stdio.h>

http_err_t handle_http_request(const http_request_t* request,
									  epoll_usrdata_t* usrdata)
{
	switch(request->method)
	{
	case http_method_get:
		http_method_get_handler(request, usrdata);
	default:
	}
	return http_success;
}

http_err_t http_method_get_handler(const http_request_t* request,
								   epoll_usrdata_t* client_data)
{
	assert(request->method == http_method_get);
	int client_fd = client_data->fd;
	string_view_t html_post = {0};
	string_t path;
	http_err_t herr;
	log_debug_message("postfix path %.*s", (int)string_view_len(request->path),
					  string_view_cstr(request->path));
	herr = get_absolue_path(request->path, &path);
	if (herr != http_success)
		return herr;
	get_resource_cached(string_view_from_string(&path), &html_post);

	http_response_t response;
	size_t header_capacity;
	header_initial(&response.headers, &response.header_len, &header_capacity);
	http_response_add_header(&response, &header_capacity, http_hdr_content_type,
							 string_view_from_cstr("text/html; charset=UTF-8"));
	char numbuf[16];
	snprintf(numbuf, sizeof(numbuf), "%zu", string_view_len(html_post));
	http_response_add_header(&response, &header_capacity,
							 http_hdr_content_length,
							 string_view_from_cstr(numbuf));

	response.body = html_post;
	string_t post = {0};
	http_response_to_string(&response, &post);

	ssize_t writebytes =
		write(client_fd, string_cstr(&post), string_len(&post));
	log_debug_message("Server Write %d bytes to client");
	if (writebytes < 0)
	{
		log_http_message_with_errno(HTTP_WRITE_ERR);
		return http_write;
	}
	return http_success;
}

