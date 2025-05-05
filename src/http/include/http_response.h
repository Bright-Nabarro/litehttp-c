#pragma once
#include "error_code.h"
#include "string_t.h"
#include "http_types.h"

typedef struct
{
	http_version_t version;
	http_status_t status;
	http_header_t* headers;
	size_t header_len;
	string_view_t body;
} http_response_t;

http_err_t http_response_to_string(const http_response_t* rep, string_t* post);

string_view_t http_get_simple_response();

