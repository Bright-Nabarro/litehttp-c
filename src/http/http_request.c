#include "http_request.h"

#include "base.h"
#include <assert.h>
#include "static_test.h"

http_err_t http_request_new(http_request_t** req)
{
	assert(req != nullptr);
	*req = calloc(1, sizeof(http_request_t));
	if (*req == nullptr)
		goto err;
	string_init(&(*req)->path);
	cc_init(&(*req)->header_list);
	string_init(&(*req)->body);
	if ((*req)->header_list == nullptr)
		goto cleanup;
	return http_success;

cleanup:
	free(req);
err:
	log_http_message_with_errno(HTTP_MALLOC_ERR);
	return http_err_malloc;
}

void http_request_free(http_request_t* req)
{
	if (req == nullptr)
		return;
	cc_cleanup(&req->header_list);
	free(req);
}



