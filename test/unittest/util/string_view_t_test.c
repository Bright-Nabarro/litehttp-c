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

int main()
{
	test_string_view_from_cstr();
	test_string_view_from_parts();
	test_string_view_from_string();
	test_string_view_len();
	test_string_view_empty();
	test_string_view_cstr();
	test_string_view_compare();
	return main_ret;
}
