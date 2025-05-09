#include "test_util.h"
#include "http_parse_dfa.h"
#include "http_parse_ctx_internal.h"
#include "http_request.h"
#include <assert.h>


void test_http_parse_ctx_new_and_destroy_chain()
{
    {
        // 基础创建与销毁
        http_parse_ctx_t* ctx = NULL;
        http_err_t err = http_parse_ctx_new(&ctx);
        TEST(err == http_success);
        TEST(ctx != NULL);

        // 销毁链表
        http_parse_ctx_destroy_chain(&ctx);
        TEST(ctx == NULL); // 应当被置空
    }

    {
        // 空指针安全销毁
        http_parse_ctx_t *head = NULL;
        http_parse_ctx_destroy_chain(&head);
        TEST(head == NULL);
    }
}

void test_http_parse_ctx_feed_base()
{
	{
        // 空请求（空字符串）
        http_parse_ctx_t *tail = nullptr;
        http_err_t err = http_parse_ctx_new(&tail);
        TEST(err == http_success && tail != nullptr);

        string_view_t post = string_view_from_cstr("");
        http_err_t feed_err = http_parse_feed(&tail, post);

        TEST(feed_err != http_success);
		
        http_parse_ctx_destroy_chain(&tail);
    }
    {
        // 简单GET请求（无body）
        const char* request =
            "GET /index.html HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Connection: close\r\n"
            "\r\n";

        http_parse_ctx_t *tail = nullptr;
        http_err_t err = http_parse_ctx_new(&tail);
        TEST(err == http_success && tail != nullptr);

        string_view_t post = string_view_from_cstr(request);
        http_err_t feed_err = http_parse_feed(&tail, post);

        TEST(feed_err == http_success);

        http_parse_ctx_destroy_chain(&tail);
    }
}

void test_parse_request_line()
{
	{
		const char request[] = "GET /index.html HTTP/1.1\r\n";
		http_parse_ctx_t ctx = {0};
		http_err_t herr = http_success;
		herr = http_request_new(&ctx.http_request);
		TEST(herr == http_success);

		string_init_from_cstr(&ctx.post, request);
		ctx.next_line = sizeof(request) - 1; //request结尾有一个null
		ctx.request_line = string_view_from_cstr(request);
		herr = parse_request_line(&ctx);
		TEST(herr == http_success);

		http_request_t* ret_request = ctx.http_request;
		TEST(ret_request->version == http_ver_1_1);
		TEST(ret_request->method == http_method_get);
		TEST(string_view_compare_cstr(ret_request->path, "/index.html") == 0);

		http_request_free(ret_request);
	}
	// Test HEAD
    {
        const char request[] = "HEAD /test/page HTTP/1.0\r\n";
        http_parse_ctx_t ctx = {0};
        http_err_t herr = http_success;
        herr = http_request_new(&ctx.http_request);
        TEST(herr == http_success);

        string_init_from_cstr(&ctx.post, request);
        ctx.next_line = sizeof(request) - 1;
        ctx.request_line = string_view_from_cstr(request);
        herr = parse_request_line(&ctx);
        TEST(herr == http_success);

        http_request_t* ret_request = ctx.http_request;
        TEST(ret_request->version == http_ver_1_0);
        TEST(ret_request->method == http_method_head);
        TEST(string_view_compare_cstr(ret_request->path, "/test/page") == 0);

        http_request_free(ret_request);
    }
    // Test POST
    {
        const char request[] = "POST /api/v1/update HTTP/1.1\r\n";
        http_parse_ctx_t ctx = {0};
        http_err_t herr = http_success;
        herr = http_request_new(&ctx.http_request);
        TEST(herr == http_success);

        string_init_from_cstr(&ctx.post, request);
        ctx.next_line = sizeof(request) - 1;
        ctx.request_line = string_view_from_cstr(request);
        herr = parse_request_line(&ctx);
        TEST(herr == http_success);

        http_request_t* ret_request = ctx.http_request;
        TEST(ret_request->version == http_ver_1_1);
        TEST(ret_request->method == http_method_post);
        TEST(string_view_compare_cstr(ret_request->path, "/api/v1/update") == 0);

        http_request_free(ret_request);
    }
    // Test invalid method
    {
        const char request[] = "FOO /invalid HTTP/1.1\r\n";
        http_parse_ctx_t ctx = {0};
        http_err_t herr = http_success;
        herr = http_request_new(&ctx.http_request);
        TEST(herr == http_success);

        string_init_from_cstr(&ctx.post, request);
        ctx.next_line = sizeof(request) - 1;
        ctx.request_line = string_view_from_cstr(request);
        herr = parse_request_line(&ctx);
        //TEST(herr == http_success); // Should return error for invalid method

        http_request_free(ctx.http_request);
    }
    // Test missing HTTP version
    {
        const char request[] = "GET /no-version\r\n";
        http_parse_ctx_t ctx = {0};
        http_err_t herr = http_success;
        herr = http_request_new(&ctx.http_request);
        TEST(herr == http_success);

        string_init_from_cstr(&ctx.post, request);
        ctx.next_line = sizeof(request) - 1;
        ctx.request_line = string_view_from_cstr(request);
        herr = parse_request_line(&ctx);
        TEST(herr != http_success); // Should return error for missing version

        http_request_free(ctx.http_request);
    }
}

void test_parse_header_line_single()
{
	{
		const char header[] = 
			"Host: example.com\r\n";
		http_err_t herr = http_success;
		http_parse_ctx_t* ctx = nullptr;
		herr = http_parse_ctx_new(&ctx);
		TEST(herr == http_success);
	    string_init_from_cstr(&ctx->post, header);
        ctx->next_line = sizeof(header) - 1;

        string_view_t new_header = string_view_from_parts(header, sizeof(header)-3);
		assert(header[sizeof(header)-3] == '\r');
		cc_push(&ctx->header_list, new_header);

		herr = parse_header_line(ctx);
		TEST(herr == http_success);
		http_request_t* request = ctx->http_request;
		TEST(cc_size(&request->header_list) == 1);
		http_header_t ret_header = *cc_get(&request->header_list, 0);
		TEST(ret_header.field == http_hdr_host);
		TEST(string_view_compare_cstr(ret_header.value.value_sv, "example.com") == 0);
		TEST(!ret_header.value.has_parsed_type);
		TEST(string_view_empty(ret_header.name));
		
		http_parse_ctx_free_node(ctx);
	}
	 //User-Agent
    {
        const char header[] = "User-Agent: test-agent/1.0\r\n";
        http_err_t herr = http_success;
        http_parse_ctx_t* ctx = nullptr;
        herr = http_parse_ctx_new(&ctx);
        TEST(herr == http_success);
        string_init_from_cstr(&ctx->post, header);
        ctx->next_line = sizeof(header) - 1;

        string_view_t new_header = string_view_from_parts(header, sizeof(header)-3);
		assert(header[sizeof(header)-3] == '\r');
        cc_push(&ctx->header_list, new_header);

        herr = parse_header_line(ctx);
        TEST(herr == http_success);
        http_request_t* request = ctx->http_request;
        TEST(cc_size(&request->header_list) == 1);
        http_header_t ret_header = *cc_get(&request->header_list, 0);
        TEST(ret_header.field == http_hdr_user_agent);
        TEST(string_view_compare_cstr(ret_header.value.value_sv, "test-agent/1.0") == 0);
        TEST(string_view_empty(ret_header.name));
        http_parse_ctx_free_node(ctx);
    }

	// Accept
	{
	    const char header[] = "Accept: */*\r\n";
	    http_err_t herr = http_success;
	    http_parse_ctx_t* ctx = nullptr;
	    herr = http_parse_ctx_new(&ctx);
	    TEST(herr == http_success);
	    string_init_from_cstr(&ctx->post, header);
	    ctx->next_line = sizeof(header) - 1;
	
	    string_view_t new_header = string_view_from_parts(header, sizeof(header) - 3);
	    cc_push(&ctx->header_list, new_header);
	
	    herr = parse_header_line(ctx);
	    TEST(herr == http_success);
	    http_request_t* request = ctx->http_request;
	    TEST(cc_size(&request->header_list) == 1);
	    http_header_t ret_header = *cc_get(&request->header_list, 0);
	    TEST(ret_header.field == http_hdr_accept);
	    TEST(string_view_compare_cstr(ret_header.value.value_sv, "*/*") == 0);
	    TEST(string_view_empty(ret_header.name));
	    http_parse_ctx_free_node(ctx);
	}
	// Content-Type
	{
	    const char header[] = "Content-Type: application/json\r\n";
	    http_err_t herr = http_success;
	    http_parse_ctx_t* ctx = nullptr;
	    herr = http_parse_ctx_new(&ctx);
	    TEST(herr == http_success);
	    string_init_from_cstr(&ctx->post, header);
	    ctx->next_line = sizeof(header) - 1;
	
	    string_view_t new_header = string_view_from_parts(header, sizeof(header) - 3);
	    cc_push(&ctx->header_list, new_header);
	
	    herr = parse_header_line(ctx);
	    TEST(herr == http_success);
	    http_request_t* request = ctx->http_request;
	    TEST(cc_size(&request->header_list) == 1);
	    http_header_t ret_header = *cc_get(&request->header_list, 0);
	    TEST(ret_header.field == http_hdr_content_type);
	    TEST(string_view_compare_cstr(ret_header.value.value_sv, "application/json") == 0);
	    TEST(ret_header.value.content_type.major == http_content_type_major_application);
	    TEST(ret_header.value.content_type.sub == http_content_type_sub_json);
	    TEST(string_view_empty(ret_header.name));
	    http_parse_ctx_free_node(ctx);
	}
	// Content-Length
	{
	    const char header[] = "Content-Length: 1234\r\n";
	    http_err_t herr = http_success;
	    http_parse_ctx_t* ctx = nullptr;
	    herr = http_parse_ctx_new(&ctx);
	    TEST(herr == http_success);
	    string_init_from_cstr(&ctx->post, header);
	    ctx->next_line = sizeof(header) - 1;
	
	    string_view_t new_header = string_view_from_parts(header, sizeof(header) - 3);
	    cc_push(&ctx->header_list, new_header);
	
	    herr = parse_header_line(ctx);
	    TEST(herr == http_success);
	    http_request_t* request = ctx->http_request;
	    TEST(cc_size(&request->header_list) == 1);
	    http_header_t ret_header = *cc_get(&request->header_list, 0);
	    TEST(ret_header.field == http_hdr_content_length);
	    TEST(string_view_compare_cstr(ret_header.value.value_sv, "1234") == 0);
	    TEST(ret_header.value.content.content_length == 1234);
	    TEST(string_view_empty(ret_header.name));
	    http_parse_ctx_free_node(ctx);
	}
	// Connection
	{
	    const char header[] = "Connection: keep-alive\r\n";
	    http_err_t herr = http_success;
	    http_parse_ctx_t* ctx = nullptr;
	    herr = http_parse_ctx_new(&ctx);
	    TEST(herr == http_success);
	    string_init_from_cstr(&ctx->post, header);
	    ctx->next_line = sizeof(header) - 1;
	
	    string_view_t new_header = string_view_from_parts(header, sizeof(header) - 3);
	    cc_push(&ctx->header_list, new_header);
	
	    herr = parse_header_line(ctx);
	    TEST(herr == http_success);
	    http_request_t* request = ctx->http_request;
	    TEST(cc_size(&request->header_list) == 1);
	    http_header_t ret_header = *cc_get(&request->header_list, 0);
	    TEST(ret_header.field == http_hdr_connection);
	    TEST(string_view_compare_cstr(ret_header.value.value_sv, "keep-alive") == 0);
	    TEST(ret_header.value.connection.type == http_connection_keep_alive);
	    TEST(string_view_empty(ret_header.name));
	    http_parse_ctx_free_node(ctx);
	}
	// Transfer-Encoding
	{
	    const char header[] = "Transfer-Encoding: chunked\r\n";
	    http_err_t herr = http_success;
	    http_parse_ctx_t* ctx = nullptr;
	    herr = http_parse_ctx_new(&ctx);
	    TEST(herr == http_success);
	    string_init_from_cstr(&ctx->post, header);
	    ctx->next_line = sizeof(header) - 1;
	
	    string_view_t new_header = string_view_from_parts(header, sizeof(header) - 3);
	    cc_push(&ctx->header_list, new_header);
	
	    herr = parse_header_line(ctx);
	    TEST(herr == http_success);
	    http_request_t* request = ctx->http_request;
	    TEST(cc_size(&request->header_list) == 1);
	    http_header_t ret_header = *cc_get(&request->header_list, 0);
	    TEST(ret_header.field == http_hdr_transfer_encoding);
	    TEST(string_view_compare_cstr(ret_header.value.value_sv, "chunked") == 0);
	    TEST(ret_header.value.transfer_encoding.type == http_transfer_encoding_chunked);
	    TEST(string_view_empty(ret_header.name));
	    http_parse_ctx_free_node(ctx);
	}
}

void test_parse_header_line_mixed()
{
	{
		const char headers[] = 
			"Host: example.com\r\n"
			"User-Agent: test-agent/1.0\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: 1234\r\n"
			"Connection: keep-alive\r\n"
		;

		const char* sheaders[] = {
			"Host: example.com",
			"User-Agent: test-agent/1.0",
			"Content-Type: application/json",
			"Content-Length: 1234",
			"Connection: keep-alive"
		};

		http_header_field_t header_key[] = {
			http_hdr_host,
			http_hdr_user_agent,
			http_hdr_content_type,
			http_hdr_content_length,
			http_hdr_connection,
		};

		http_err_t herr = http_success;
		http_parse_ctx_t* ctx = nullptr;
		herr = http_parse_ctx_new(&ctx);
	    TEST(herr == http_success);
		string_init_from_cstr(&ctx->post, headers);
	    ctx->next_line = sizeof(headers) - 1;

		size_t header_size = sizeof(sheaders)/sizeof(sheaders[0]);
		for (size_t i = 0; i < header_size; ++i)
		{
			string_view_t new_header = string_view_from_cstr(sheaders[i]);
			cc_push(&ctx->header_list, new_header);
			herr = parse_header_line(ctx);
			TEST(herr == http_success);
		}
		http_request_t* request = ctx->http_request;
		TEST(cc_size(&request->header_list) == header_size);
		
		for (size_t i = 0; i < header_size; ++i)
		{
			http_header_t* ret_header = cc_get(&request->header_list, i);
			TEST(ret_header->field == header_key[i]);
			switch(header_key[i])
			{
			case http_hdr_host:
				string_view_compare_cstr(ret_header->value.value_sv,
										 "example.com");
				TEST(!ret_header->value.has_parsed_type);
				break;
			case http_hdr_user_agent:
				TEST(string_view_compare_cstr(ret_header->value.value_sv,
										 "test-agent/1.0") == 0);
				TEST(!ret_header->value.has_parsed_type);
				break;
			case http_hdr_content_type:
				TEST(string_view_compare_cstr(ret_header->value.value_sv,
										 "application/json") == 0);
				TEST(ret_header->value.has_parsed_type);
				TEST(ret_header->value.content_type.major ==
					 http_content_type_major_application);
				TEST(ret_header->value.content_type.sub ==
					 http_content_type_sub_json);
				break;
			case http_hdr_content_length:
				TEST(string_view_compare_cstr(ret_header->value.value_sv,
					 "1234") == 0);
				TEST(ret_header->value.has_parsed_type);
				TEST(ret_header->value.content.content_length == 1234);
				break;
			case http_hdr_connection:
				TEST(string_view_compare_cstr(ret_header->value.value_sv,
					 "keep-alive") == 0);
				TEST(ret_header->value.has_parsed_type);
				TEST(ret_header->value.connection.type ==
					 http_connection_keep_alive);
				break;
			default:
				assert(false);
			}
		}
	}
}

int main()
{
	logger_init();
	test_http_parse_ctx_new_and_destroy_chain();
	test_http_parse_ctx_feed_base();
	test_parse_request_line();
	test_parse_header_line_single();
	test_parse_header_line_mixed();
	return main_ret;
}
