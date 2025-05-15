#pragma once
#include "http_request.h"
#include "http_response.h"

http_err_t handle_http_request(const http_request_t* request,
								   http_response_t** res);

http_err_t http_method_get_handler(const http_request_t* request,
								   http_response_t** res);

