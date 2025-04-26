#include "http_request.h"

http_err_t http_request_add_header(http_request_t* req, size_t* capacity,
								   http_header_field_t field,
								   string_view_t value)
{
	http_err_t herr = http_success;
	herr = http_header_push_back_empty(&req->headers, &req->header_count, capacity);
	if (herr != http_success)
	{
		return herr;
	}
	req->headers[req->header_count-1].field = field;
	req->headers[req->header_count-1].value = value;

	return herr;
}

http_err_t http_request_add_header_other(http_request_t* req, size_t* capacity,
										 string_view_t name,
										 string_view_t value)
{
	http_err_t herr = http_success;
	herr = http_header_push_back_empty(&req->headers, &req->header_count, capacity);
	if (herr != http_success)
		return herr;
	req->headers[req->header_count-1].name = name;
	req->headers[req->header_count-1].value = value;

	return herr;
}


