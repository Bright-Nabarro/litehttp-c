#include "client.h"

#include "base.h"
#include "error_code.h"
#include "http_method_dispatch.h"
#include "http_parse_dfa.h"
#include "socket_util.h"
#include "string_t.h"
#include <stdio.h>
#include <sys/epoll.h>
#include "server.h"
#include <assert.h>

static
http_err_t read_client(string_t* rec_str, client_data_t* usrdata);

void handle_client_fd(void* vargs)
{
	fn_args_t* args = vargs;
	client_data_t* usrdata = args->usrdata;
	int epoll_fd = args->epoll_fd;
	http_err_t herr = http_success;
	log_http_message(HTTP_REPORT_CLIENT_INFO, string_cstr(&usrdata->ip),
					 usrdata->port);

	string_t receive_post;
	// 读取客户端, 写入receive_post
	herr = read_client(&receive_post, usrdata);
	if (herr == http_err_client_close)
		goto closefd;
	if (herr != http_success)
		goto err;

	herr = http_parse_feed(&usrdata->ctx_tail,
						   string_view_from_string(&receive_post));
	if (herr == http_err_request_fatal)
		goto err;

	http_request_t* request = nullptr;
	while (http_parse_pop_request_and_release(&usrdata->ctx_beg, &request))
	{
		http_response_t* response = nullptr;
		herr = handle_http_request(request, &response);
		HERR_TRANSFER(herr, goto err);
		assert(response != nullptr);
		herr = http_send_response(usrdata, response);
		HERR_TRANSFER(herr, goto err);
	}
	
	CALL_BACK_RETURN(args->pherr, herr);

closefd:
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, usrdata->fd, nullptr) < 0)
	{
		log_http_message_with_errno(HTTP_EPOLL_CTL_DEL_ERR);
		herr = http_err_epoll_ctl_del;
	}
	close(epoll_fd);
	client_data_free(usrdata);
err:
	CALL_BACK_RETURN(args->pherr, herr);
}

http_err_t create_client_data(client_data_t** ret, int fd,
							  struct in_addr* remote_addr,
							  int remote_port)
{
	*ret = calloc(1, sizeof(client_data_t));
	if (*ret == nullptr)
	{
		log_http_message(HTTP_MALLOC_ERR);
		return http_err_malloc;
	}
	(*ret)->fd = fd;
	ipv4_net2str(&(*ret)->ip, remote_addr);
	(*ret)->port = htons(remote_port);
	http_err_t herr = http_parse_ctx_new(&(*ret)->ctx_beg);
	if (herr != http_success)
		return herr;
	(*ret)->ctx_tail = (*ret)->ctx_beg;
	return http_success;
}

void client_data_free(client_data_t* data)
{
	http_parse_ctx_destroy_chain(&data->ctx_beg);
	string_destroy(&data->ip);
	free(data);
}

static
http_err_t read_client(string_t* rec_str, client_data_t* usrdata)
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


