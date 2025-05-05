#include "http_response.h"
#include "base.h"

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
	http_get_status_sv(rep->version, buf);
	string_append_cstr(post, buf);
	// status msg
	string_append_cstr(post, "\r\n");
	for (size_t i = 0; i < rep->header_len; ++i)
	{
		string_append_view(post, http_get_header_field_sv(rep->headers[i].field));
		string_append_cstr(post, ": ");
		string_append_view(post, rep->headers[i].value.value_sv);
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

