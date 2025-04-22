#include <assert.h>
#include "http_method_dispatch.h"
#include "http_response.h"
#include "base.h"

http_err_t http_method_get_handler(const http_request_t* request,
								   epoll_usrdata_t* client_data)
{
	assert(request->method == http_method_get);
	int client_fd = client_data->fd;
	string_view_t sp = get_simple_response();
	char buf[string_view_len(sp)+1];
	string_view_to_buf(sp, buf, sizeof(buf));
	ssize_t writebytes = write(client_fd, buf, sizeof(buf));
	log_debug_message("Server Write %d bytes to client");
	if (writebytes < 0)
	{
		log_http_message_with_errno(HTTP_WRITE_ERR);
		return http_write;
	}
	return http_success;
}
