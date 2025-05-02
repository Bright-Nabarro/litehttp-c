#include "test_util.h"
#include "http_parse_dfa.h"

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

int main()
{
	logger_init();
	test_http_parse_ctx_new_and_destroy_chain();
	test_http_parse_ctx_feed_base();
	return main_ret;
}
