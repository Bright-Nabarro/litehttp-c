#pragma once
#include <limits.h>
#include "error_code.h"
#include "http_request.h"

http_err_t http_parse_request(string_view_t post, http_request_t* request);

