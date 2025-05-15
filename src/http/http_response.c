#include "http_response.h"
#include "base.h"
#include "assert.h"

/****
 * INTERFACE IMPLEMENTATION
 ****/

http_err_t http_response_to_string(const http_response_t* rep, string_t* post)
{
	string_init(post);
	string_view_t sv = {0};
	// version
	sv = http_get_version_sv(rep->version);
	if (string_view_empty(sv))
	{
		log_http_message(HTTP_GET_UNKOWN_VERSION);
		return http_err_get_unkown_version;
	}
	string_append_view(post, sv);
	string_push_back(post, ' ');

	// status
	char buf[4] = {0};
	http_get_status_sv(rep->status.code, buf);
	string_append_cstr(post, buf);
	string_push_back(post, ' ');
	string_append_view(post, rep->status.message);
	// status msg
	string_append_cstr(post, "\r\n");
	size_t header_size = cc_size(&rep->header_list);
	for (size_t i = 0; i < header_size; ++i)
	{
		string_append_view(
			post, http_get_header_field_sv(cc_get(&rep->header_list, i)->field));
		string_append_cstr(post, ": ");
		string_append_view(post, cc_get(&rep->header_list, i)->value.value_sv);
		string_append_cstr(post, "\r\n");
	}
	string_append_cstr(post, "\r\n");
	string_append_view(post, rep->body);
	string_append_cstr(post, "\r\n");
	return http_success;
}

string_view_t http_get_simple_response()
{
	static const char* post = "HTTP/1.1 200 OK\r\n"
							  "Content-Type: text/html; charset=UTF-8\r\n"
							  "Content-Length: 25\r\n"
							  "\r\n"
							  "<html>Hello, world!</html>\r\n";
	return string_view_from_cstr(post);
}

http_err_t http_response_new(http_response_t** rep)
{
	assert(rep != nullptr);
	*rep = calloc(1, sizeof(http_response_t));
	ERR_HANDLER_PTR(*rep, return http_err_malloc, HTTP_MALLOC_ERR);
	cc_init(&(*rep)->header_list);
	ERR_HANDLER_PTR((*rep)->header_list, return http_err_malloc,
					HTTP_MALLOC_ERR);
	return http_success;
}

void http_response_free(http_response_t* rep)
{
	if (rep == nullptr)
		return;
	cc_cleanup(&rep->header_list);
	free(rep);
}

http_err_t http_response_add_header(http_response_t* response,
									http_header_field_t key,
									string_view_t value)
{
	http_header_t new_header = {0};
	new_header.field = key;
	new_header.value.value_sv = value;
	if (cc_push(&response->header_list, new_header) == nullptr)
	{
		log_http_message_with_errno(HTTP_MALLOC_ERR);
		return http_err_malloc;
	}
	return http_success;
}
