#pragma once
#include "error_code.h"
#include "string_t.h"
#include <stddef.h>

http_err_t resource_getter_initial();
http_err_t resource_getter_release();

http_err_t get_resource_cached(string_view_t path, string_view_t* res);
http_err_t get_resource_refreshed(string_view_t path, string_view_t* res);

