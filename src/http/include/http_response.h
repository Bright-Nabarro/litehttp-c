#pragma once
#include "error_code.h"
#include "string_t.h"
#include "http_types.h"

typedef struct
{
	http_version_t version;
	http_status_t status;
	http_request_header_t* headers;
	size_t header_len;
	string_view_t body;
} http_response_t;

http_err_t http_response_add_header(http_response_t* rep, size_t* capacity,
									http_header_field_t field,
									string_view_t value);

http_err_t http_response_to_string(const http_response_t* rep, string_t* post);

string_view_t get_simple_response();

http_err_t get_success_response(string_view_t html_post,
								http_response_t* response);

