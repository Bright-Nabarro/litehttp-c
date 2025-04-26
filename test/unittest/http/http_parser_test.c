#include "http_parser.c"

#include "test_util.h"

void test_parse_request_line() {
    // --- 正常情况 ---
    {
        http_parse_ctx_t ctx = {0};
        http_request_t req = {0};
        char raw[] = "GET /index.html HTTP/1.1\r\nHost: test\r\n\r\n";
        ctx.raw_data = string_view_from_cstr(raw);
        ctx.cur = raw;
        ctx.remaining_len = strlen(raw);
        ctx.cur_line_len = strlen("GET /index.html HTTP/1.1"); // 设置为请求行长度

        http_err_t err = parse_request_line(&ctx, &req);

        TEST(err == http_success);
        TEST(req.method == http_method_get);
        TEST(req.version == http_ver_1_1);

        TEST(string_view_len(req.path) == strlen("/index.html"));
		TEST(string_view_compare_cstr(req.path, "/index.html") == 0);
    }

    // --- 不合法请求行 ---
    {
        http_parse_ctx_t ctx = {0};
        http_request_t req = {0};
        char raw[] = "BADREQUEST\r\nHost: test\r\n\r\n";
        ctx.raw_data = string_view_from_cstr(raw);
        ctx.cur = raw;
        ctx.remaining_len = strlen(raw);
        ctx.cur_line_len = strlen("BADREQUEST");

        http_err_t err = parse_request_line(&ctx, &req);
        TEST(err != http_success);
        TEST(req.method == http_method_unkown || req.method >= http_method_max_known);
        TEST(req.version == http_ver_unkown);
        TEST(string_view_empty(req.path));
    }

    // --- 空请求行 ---
    {
        http_parse_ctx_t ctx = {0};
        http_request_t req = {0};
        char raw[] = "\r\nHost: test\r\n\r\n";
        ctx.raw_data = string_view_from_cstr(raw);
        ctx.cur = raw;
        ctx.remaining_len = strlen(raw);
        ctx.cur_line_len = 0;

        http_err_t err = parse_request_line(&ctx, &req);
        TEST(err != http_success);
        TEST(req.method == http_method_unkown || req.method >= http_method_max_known);
        TEST(req.version == http_ver_unkown);
        TEST(string_view_empty(req.path));
    }

    // --- 超长请求行 ---
    {
        http_parse_ctx_t ctx = {0};
        http_request_t req = {0};
        char raw[4096];
        memset(raw, 'A', sizeof(raw) - 4);
        memcpy(raw + sizeof(raw) - 4, "\r\n\r\n", 4);
        ctx.raw_data = string_view_from_parts(raw, sizeof(raw));
        ctx.cur = raw;
        ctx.remaining_len = sizeof(raw);
        ctx.cur_line_len = sizeof(raw) - 4; // 假定整行为超长长度

        http_err_t err = parse_request_line(&ctx, &req);
        // 假设超长会返回错误
        TEST(err != http_success);
        TEST(req.method == http_method_unkown || req.method >= http_method_max_known);
        TEST(req.version == http_ver_unkown);
        TEST(string_view_empty(req.path));
    }
}

void test_parse_header_line() {
    // --- 正常头部字段 ---
    {
        http_parse_ctx_t ctx = {0};
        http_request_header_t header = {0};
        char raw[] = "Host: example.com\r\n";
        ctx.raw_data = string_view_from_cstr(raw);
        ctx.cur = raw;
        ctx.remaining_len = strlen(raw);
        ctx.cur_line_len = strlen("Host: example.com");

        http_err_t err = parse_header_line(&ctx, &header);

        TEST(err == http_success);
        TEST(header.field == http_hdr_host);
        TEST(string_view_compare_cstr(header.value, "example.com") == 0);
    }

    // --- 其他头部字段 ---
    {
        http_parse_ctx_t ctx = {0};
        http_request_header_t header = {0};
        char raw[] = "X-Custom-Header: foobar\r\n";
        ctx.raw_data = string_view_from_cstr(raw);
        ctx.cur = raw;
        ctx.remaining_len = strlen(raw);
        ctx.cur_line_len = strlen("X-Custom-Header: foobar");

        http_err_t err = parse_header_line(&ctx, &header);

        TEST(err == http_success);
        TEST(header.field == http_hdr_other);
        TEST(string_view_compare_cstr(header.name, "X-Custom-Header") == 0);
        TEST(string_view_compare_cstr(header.value, "foobar") == 0);
    }

    // --- 空头部行 ---
    {
        http_parse_ctx_t ctx = {0};
        http_request_header_t header = {0};
        char raw[] = "\r\n";
        ctx.raw_data = string_view_from_cstr(raw);
        ctx.cur = raw;
        ctx.remaining_len = strlen(raw);
        ctx.cur_line_len = 0;

        http_err_t err = parse_header_line(&ctx, &header);

        TEST(err != http_success);
    }

    // --- 非法格式 ---
    {
        http_parse_ctx_t ctx = {0};
        http_request_header_t header = {0};
        char raw[] = "NoColonHere\r\n";
        ctx.raw_data = string_view_from_cstr(raw);
        ctx.cur = raw;
        ctx.remaining_len = strlen(raw);
        ctx.cur_line_len = strlen("NoColonHere");

        http_err_t err = parse_header_line(&ctx, &header);

        TEST(err != http_success);
    }
}

void test_parse_http_request_ctx() {
    // --- 标准 GET 请求，无 body ---
    {
        http_parse_ctx_t ctx = {0};
        http_request_t req = {0};
        char raw[] =
            "GET /hello HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "User-Agent: test-agent\r\n"
            "Accept: */*\r\n"
            "\r\n";

		ctx.raw_data = string_view_from_cstr(raw);
        http_err_t err = parse_http_request_ctx(&ctx, &req);

        TEST(err == http_success);
        TEST(req.method == http_method_get);
        TEST(req.version == http_ver_1_1);
        TEST(string_view_compare_cstr(req.path, "/hello") == 0);

        // 头部数量应为3
        TEST(req.header_count == 3);

        // Host
        TEST(req.headers[0].field == http_hdr_host);
        TEST(string_view_compare_cstr(req.headers[0].value, "example.com") == 0);

        // User-Agent
        TEST(req.headers[1].field == http_hdr_user_agent);
        TEST(string_view_compare_cstr(req.headers[1].value, "test-agent") == 0);

        // Accept
        TEST(req.headers[2].field == http_hdr_accept);
        TEST(string_view_compare_cstr(req.headers[2].value, "*/*") == 0);

        // 没有body
        TEST(string_view_empty(req.body));

		free(req.headers);
    }

    // --- 只有请求行和一个首部 ---
    {
        http_parse_ctx_t ctx = {0};
        http_request_t req = {0};
        char raw[] =
            "HEAD /abc HTTP/1.0\r\n"
            "Host: foobar\r\n"
            "\r\n";
        ctx.raw_data = string_view_from_cstr(raw);
        http_err_t err = parse_http_request_ctx(&ctx, &req);

        TEST(err == http_success);
        TEST(req.method == http_method_head);
        TEST(req.version == http_ver_1_0);
        TEST(string_view_compare_cstr(req.path, "/abc") == 0);

        TEST(req.header_count == 1);
        TEST(req.headers[0].field == http_hdr_host);
        TEST(string_view_compare_cstr(req.headers[0].value, "foobar") == 0);

        TEST(string_view_empty(req.body));
		free(req.headers);
    }
}

void test_parse_http_request()
{
    // 1. HEAD method, 1 header
    {
        char raw[] =
            "HEAD /abc HTTP/1.0\r\n"
            "Host: foobar\r\n"
            "\r\n";
        string_view_t raw_view = string_view_from_cstr(raw);
        http_request_t req = {0};
        http_err_t err = parse_http_request(raw_view, &req);

        TEST(err == http_success);
        TEST(req.method == http_method_head);
        TEST(req.version == http_ver_1_0);
        TEST(string_view_compare_cstr(req.path, "/abc") == 0);
        TEST(req.header_count == 1);
        TEST(req.headers[0].field == http_hdr_host);
        TEST(string_view_compare_cstr(req.headers[0].value, "foobar") == 0);
        TEST(string_view_empty(req.body));
		free(req.headers);
    }

    // 2. GET method, 2 headers
    {
        char raw[] =
            "GET /foo/bar?x=1 HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "User-Agent: testagent\r\n"
            "\r\n";
        string_view_t raw_view = string_view_from_cstr(raw);
        http_request_t req = {0};
        http_err_t err = parse_http_request(raw_view, &req);

        TEST(err == http_success);
        TEST(req.method == http_method_get);
        TEST(req.version == http_ver_1_1);
        TEST(string_view_compare_cstr(req.path, "/foo/bar?x=1") == 0);
        TEST(req.header_count == 2);
        TEST(req.headers[0].field == http_hdr_host);
        TEST(string_view_compare_cstr(req.headers[0].value, "example.com") == 0);
        TEST(req.headers[1].field == http_hdr_user_agent);
        TEST(string_view_compare_cstr(req.headers[1].value, "testagent") == 0);
        TEST(string_view_empty(req.body));
		free(req.headers);
    }

    // 3. POST method, no headers
    {
        char raw[] =
            "POST /submit HTTP/1.1\r\n"
            "\r\n";
        string_view_t raw_view = string_view_from_cstr(raw);
        http_request_t req = {0};
        http_err_t err = parse_http_request(raw_view, &req);

        TEST(err == http_success);
        TEST(req.method == http_method_post);
        TEST(req.version == http_ver_1_1);
        TEST(string_view_compare_cstr(req.path, "/submit") == 0);
        TEST(req.header_count == 0);
        TEST(string_view_empty(req.body));
		free(req.headers);
    }

    // 4. Path with special characters, one header
    {
        char raw[] =
            "GET /a%20b/c_d~e HTTP/1.1\r\n"
            "Host: test.local\r\n"
            "\r\n";
        string_view_t raw_view = string_view_from_cstr(raw);
        http_request_t req = {0};
        http_err_t err = parse_http_request(raw_view, &req);

        TEST(err == http_success);
        TEST(req.method == http_method_get);
        TEST(req.version == http_ver_1_1);
        TEST(string_view_compare_cstr(req.path, "/a%20b/c_d~e") == 0);
        TEST(req.header_count == 1);
        TEST(req.headers[0].field == http_hdr_host);
        TEST(string_view_compare_cstr(req.headers[0].value, "test.local") == 0);
        TEST(string_view_empty(req.body));
		free(req.headers);
    }

    // 5. Only request line, no headers, no body
    {
        char raw[] =
            "GET / HTTP/1.0\r\n"
            "\r\n";
        string_view_t raw_view = string_view_from_cstr(raw);
        http_request_t req = {0};
        http_err_t err = parse_http_request(raw_view, &req);

        TEST(err == http_success);
        TEST(req.method == http_method_get);
        TEST(req.version == http_ver_1_0);
        TEST(string_view_compare_cstr(req.path, "/") == 0);
        TEST(req.header_count == 0);
        TEST(string_view_empty(req.body));
		free(req.headers);
    }
}

int main()
{
	logger_init();
	// static
	test_parse_request_line();
	test_parse_header_line();
	test_parse_http_request_ctx();
	// interface
	test_parse_http_request();
	return main_ret;
}
