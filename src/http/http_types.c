#include <assert.h>
#include <stdlib.h>
#include "http_types.h"


// 添加方法时候需要注意和 http_method_t对应上
static const char* http_methods[] = {
	"GET",
	"HEAD",
	"POST",
};

const char** get_http_methods()
{
	return http_methods;
}

size_t get_http_methods_len()
{
	return sizeof(http_methods) / sizeof(http_methods[0]);
}

http_request_t* http_request_alloc()
{
	return calloc(1, sizeof(http_request_t));
}

void http_request_free(http_request_t* request)
{
	assert(request != nullptr);
	free(request->headers);
	free(request);
}



