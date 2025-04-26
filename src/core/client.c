#include "client.h"

#include "base.h"
#include "error_code.h"
#include "http_method_dispatch.h"
#include "http_parser.h"
#include "http_types.h"
#include "socket_util.h"
#include "string_t.h"
#include <stdio.h>
#include <sys/epoll.h>

static
http_err_t read_client(string_t* rec_str, epoll_usrdata_t* usrdata);

void handle_client_fd(void* vargs)
{
	fn_args_t* args = vargs;
	epoll_usrdata_t* client_data = args->usrdata;
	int epoll_fd = args->epoll_fd;
	http_err_t herr = http_success;
	log_http_message(HTTP_REPORT_CLIENT_INFO, string_cstr(&client_data->ip),
					 client_data->port);

	http_request_t http_request;

	string_t receive_post;
	// 读取客户端, 写入receive_post
	herr = read_client(&receive_post, client_data);
	if (herr == http_err_client_close)
		goto closefd;
	if (herr != http_success)
		goto err;

	herr = http_parse_request(string_view_from_string(&receive_post),
							  &http_request);
	if (herr != http_success)
		goto err;
	
	herr = handle_http_request(&http_request, client_data);
	if (herr != http_success)
		goto err;

	CALL_BACK_RETURN(args->pherr, herr);

closefd:
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, epoll_fd, nullptr) < 0)
	{
		log_http_message(HTTP_EPOLL_CTL_DEL_ERR);
		herr = http_err_epoll_ctl_del;
	}
	close(epoll_fd);
	epoll_usrdata_free(client_data);
err:
	CALL_BACK_RETURN(args->pherr, herr);
}

static
http_err_t read_client(string_t* rec_str, epoll_usrdata_t* usrdata)
{
	char buffer[BUFSIZ];
	bool req_close = false;
	string_init(rec_str);
	while(true)
	{
		ssize_t readsize = 0;
		do
		{
			readsize = read(usrdata->fd, buffer, sizeof(buffer)-1);
		} while (readsize < 0 && errno == EINTR);
		if (readsize < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			log_http_message_with_errno(HTTP_READ_ERR);
			req_close = true;
			break;
		}
		else if (readsize == 0)
		{
			req_close = true;
			break;
		}
		buffer[readsize] = 0;
		log_http_message(HTTP_RECV_FROM_CLIENT, buffer);
		
		string_append_parts(rec_str, buffer, readsize);
	}

	if (req_close)
		return http_err_client_close;

	return http_success;
}


