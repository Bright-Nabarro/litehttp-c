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
	cc_init(&(*req)->header_list);
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

http_err_t http_request_add_header(http_request_t* req, http_header_field_t field,
								   string_view_t value)
{
	http_header_t new_header = { .field = field, .value = value };
	http_header_t* push_ret = cc_push(&req->header_list, new_header);
	if (push_ret == nullptr)
	{
		return http_err_malloc;
	}

	return http_success;
}

http_err_t http_request_add_header_other(http_request_t* req, string_view_t name,
										 string_view_t value)
{
	http_header_t new_header = { .name = name, .value = value };
	http_header_t* push_ret = cc_push(&req->header_list, new_header);
	if (push_ret == nullptr)
	{
		return http_err_malloc;
	}

	return http_success;
}


