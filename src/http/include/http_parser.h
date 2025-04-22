#pragma once
#include <limits.h>
#include "error_code.h"
#include "http_types.h"

http_err_t parse_http_request(string_view_t post, http_request_t** request);

