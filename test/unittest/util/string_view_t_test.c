#include "test_util.h"
#include "string_t.h"
#include <string.h>

void test_string_view_from_cstr()
{
	const char* cstr = "abcde";
	string_view_t sv = string_view_from_cstr(cstr);
	TEST(string_view_len(sv) == strlen(cstr) && !string_view_empty(sv) &&
		 strcmp(string_view_cstr(sv), cstr) == 0);
}

void test_string_view_from_parts()
{
	const char* cstr = "abcdef";
	string_view_t sv = string_view_from_parts(cstr, 3);
	TEST(string_view_len(sv) == 3 && !string_view_empty(sv));
}

void test_string_view_from_string()
{
	string_t str;
	string_init_from_cstr(&str, "hello world");
	string_view_t sv = string_view_from_string(&str);
	TEST(string_view_len(sv) == string_len(&str) && !string_view_empty(sv));
	string_destroy(&str);
}

void test_string_view_len()
{
	string_view_t sv = string_view_from_parts("abc", 2);
	TEST(string_view_len(sv) == 2);
}

void test_string_view_empty()
{
	string_view_t sv1 = string_view_from_parts("abc", 0);
	string_view_t sv2 = string_view_from_parts("abc", 3);
	TEST(string_view_empty(sv1) && !string_view_empty(sv2));
}

void test_string_view_cstr()
{
	const char* text = "test";
	string_view_t sv = string_view_from_cstr(text);
	const char* cstr = string_view_cstr(sv);
	TEST(strcmp(cstr, text) == 0);
}

void test_string_view_compare()
{
	string_view_t sv1 = string_view_from_cstr("abc");
	string_view_t sv2 = string_view_from_cstr("abc");
	string_view_t sv3 = string_view_from_cstr("abd");
	string_view_t sv4 = string_view_from_parts("abcde", 3); // "abc"
	TEST(string_view_compare(sv1, sv2) == 0 &&
		 string_view_compare(sv1, sv3) < 0 &&
		 string_view_compare(sv3, sv1) > 0 &&
		 string_view_compare(sv1, sv4) == 0);
}

void test_string_view_hash()
{
    {
        const char* text = "hash_test";
        string_view_t sv = string_view_from_cstr(text);
        size_t hash1 = string_view_hash(sv);
        size_t hash2 = string_view_hash(sv);
        TEST(hash1 == hash2);
    }
    {
        // 空字符串
        const char* text = "";
        string_view_t sv = string_view_from_cstr(text);
        size_t hash = string_view_hash(sv);
        TEST(hash == string_view_hash(sv));
    }
    {
        // 不同内容
        string_view_t a = string_view_from_cstr("a");
        string_view_t b = string_view_from_cstr("b");
        TEST(string_view_hash(a) != string_view_hash(b));
    }
    {
        // 相同内容不同视图
        const char* text = "samecontent";
        string_view_t sv1 = string_view_from_parts(text, 4);
        string_view_t sv2 = string_view_from_parts(text, 4);
        TEST(string_view_hash(sv1) == string_view_hash(sv2));
    }
}

void test_string_view_to_buf()
{
    {
        // buf 长度大于 sv.len
        const char* text = "buffer";
        string_view_t sv = string_view_from_cstr(text);
        char buf[32] = {0};
        bool ok = string_view_to_buf(sv, buf, sizeof(buf));
        TEST(ok);
        TEST(strcmp(buf, text) == 0);
    }
    {
        // buf 长度小于 sv.len
        const char* text = "truncate";
        string_view_t sv = string_view_from_cstr(text);
        char buf[5] = {0};
        bool ok = string_view_to_buf(sv, buf, sizeof(buf));
        TEST(!ok);
        TEST(strncmp(buf, text, 4) == 0);
        TEST(buf[4] == '\0');
    }
    {
        // 用 string_view_from_parts 测试
        const char* text = "hello_world";
        string_view_t sv = string_view_from_parts(text, 5);
        char buf[16] = {0};
        bool ok = string_view_to_buf(sv, buf, sizeof(buf));
        TEST(ok);
        TEST(strcmp(buf, "hello") == 0);
    }
    {
        // buf长度为1
        string_view_t sv = string_view_from_cstr("abc");
        char buf[1] = {0};
        bool ok = string_view_to_buf(sv, buf, sizeof(buf));
        TEST(!ok);
        TEST(buf[0] == '\0');
    }
    {
        // sv长度为0
        string_view_t sv = string_view_from_cstr("");
        char buf[4] = {1,1,1,1};
        bool ok = string_view_to_buf(sv, buf, sizeof(buf));
        TEST(ok);
        TEST(buf[0] == '\0');
    }
    {
        // buf长度正好等于sv.len + 1
        string_view_t sv = string_view_from_cstr("abcd");
        char buf[5] = {0};
        bool ok = string_view_to_buf(sv, buf, sizeof(buf));
        TEST(ok);
        TEST(strcmp(buf, "abcd") == 0);
    }
}

void test_string_view_substr()
{
    {
        const char* text = "substring";
        string_view_t sv = string_view_from_cstr(text);
        string_view_t sub = string_view_substr(sv, 3, 4);
        const char* expected = "stri";
        char buf[10] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, expected) == 0);
        TEST(string_view_len(sub) == 4);
    }
    {
        // pos超出范围
        string_view_t sv = string_view_from_cstr("abcde");
        string_view_t sub = string_view_substr(sv, 10, 3);
        TEST(string_view_len(sub) == 0);
    }
    {
        // len超出范围
        string_view_t sv = string_view_from_cstr("abcdef");
        string_view_t sub = string_view_substr(sv, 4, 100);
        char buf[16] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, "ef") == 0);
        TEST(string_view_len(sub) == 2);
    }
    {
        // pos为0，len为0
        string_view_t sv = string_view_from_cstr("abcdef");
        string_view_t sub = string_view_substr(sv, 0, 0);
        TEST(string_view_len(sub) == 0);
    }
    {
        // 整体子串
        string_view_t sv = string_view_from_cstr("abcdef");
        string_view_t sub = string_view_substr(sv, 0, string_view_len(sv));
        char buf[16] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, "abcdef") == 0);
        TEST(string_view_len(sub) == 6);
    }
    {
        // len为string_t_pos(-1)的情况
        string_view_t sv = string_view_from_cstr("abcdef");
        size_t string_t_pos = (size_t)-1;
        string_view_t sub = string_view_substr(sv, 2, string_t_pos);
        char buf[16] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, "cdef") == 0);
        TEST(string_view_len(sub) == 4);
    }
    {
        // pos为string_t_pos(-1)的情况
        string_view_t sv = string_view_from_cstr("abcdef");
        size_t string_t_pos = (size_t)-1;
        string_view_t sub = string_view_substr(sv, string_t_pos, 3);
        // 应为长度为0
        TEST(string_view_len(sub) == 0);
    }
    {
        // pos和len都在范围内，截取中间一段，后面还有字符
        string_view_t sv = string_view_from_cstr("hello_world");
        string_view_t sub = string_view_substr(sv, 2, 5); // "llo_w"
        char buf[16] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, "llo_w") == 0);
        TEST(string_view_len(sub) == 5);
    }
    {
        // pos=0, len小于总长，后面有字符
        string_view_t sv = string_view_from_cstr("openai");
        string_view_t sub = string_view_substr(sv, 0, 3); // "ope"
        char buf[8] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, "ope") == 0);
        TEST(string_view_len(sub) == 3);
    }
    {
        // pos指向末尾前一个字符，len=1，后面还有字符
        string_view_t sv = string_view_from_cstr("abcdefg");
        string_view_t sub = string_view_substr(sv, 5, 1); // "f"
        char buf[4] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, "f") == 0);
        TEST(string_view_len(sub) == 1);
    }
    {
        // pos=1, len=3，后面还有字符
        string_view_t sv = string_view_from_cstr("123456789");
        string_view_t sub = string_view_substr(sv, 1, 3); // "234"
        char buf[8] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, "234") == 0);
        TEST(string_view_len(sub) == 3);
    }
    {
        // len为0但pos合法，应返回空子串
        string_view_t sv = string_view_from_cstr("abcdef");
        string_view_t sub = string_view_substr(sv, 2, 0);
        TEST(string_view_len(sub) == 0);
    }
    {
        // 只取最后一个字符
        string_view_t sv = string_view_from_cstr("abcdef");
        string_view_t sub = string_view_substr(sv, 5, 1);
        char buf[4] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, "f") == 0);
        TEST(string_view_len(sub) == 1);
    }
    {
        // 源字符串本身为空
        string_view_t sv = string_view_from_cstr("");
        string_view_t sub = string_view_substr(sv, 0, 3);
        TEST(string_view_len(sub) == 0);
    }
    {
        // len恰好等于剩余长度
        string_view_t sv = string_view_from_cstr("abcdef");
        string_view_t sub = string_view_substr(sv, 3, 3); // "def"
        char buf[8] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, "def") == 0);
        TEST(string_view_len(sub) == 3);
    }
    {
        // pos+len==sv_len（最后一个字符刚好包含进来）
        string_view_t sv = string_view_from_cstr("abcdef");
        string_view_t sub = string_view_substr(sv, 2, 4); // "cdef"
        char buf[8] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, "cdef") == 0);
        TEST(string_view_len(sub) == 4);
    }
    {
        // pos+len>sv_len，pos在末尾，len大于1
        string_view_t sv = string_view_from_cstr("abcdef");
        string_view_t sub = string_view_substr(sv, 5, 10); // "f"
        char buf[8] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, "f") == 0);
        TEST(string_view_len(sub) == 1);
    }
    {
        // pos=0, len比字符串长
        string_view_t sv = string_view_from_cstr("abcdef");
        string_view_t sub = string_view_substr(sv, 0, 100); // "abcdef"
        char buf[16] = {0};
        string_view_to_buf(sub, buf, sizeof(buf));
        TEST(strcmp(buf, "abcdef") == 0);
        TEST(string_view_len(sub) == 6);
    }
}

int main()
{
	test_string_view_from_cstr();
	test_string_view_from_parts();
	test_string_view_from_string();
	test_string_view_len();
	test_string_view_empty();
	test_string_view_cstr();
	test_string_view_compare();
	test_string_view_hash();
	test_string_view_substr();
	test_string_view_to_buf();

	return main_ret;
}
