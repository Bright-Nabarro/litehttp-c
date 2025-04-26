#include "test_util.h"
#include "resource_getter.h"
#include "string_t.h"

void test_initial_release()
{
	resource_getter_initial();
	resource_getter_release();
}

void test_get_resource_cached(const char* path)
{
	string_view_t path_sv = string_view_from_cstr(path);
	string_view_t result;
	http_err_t err = get_resource_cached(path_sv, &result);
	TEST(err == http_success);
	printf("read file:\n%.*s", (int)string_view_len(result),
		   string_view_cstr(result));
}

int main(int argc, char* argv[])
{
	logger_init();
	//test_initial_release();
	resource_getter_initial();
	for (int i = 1; i < argc; ++i)
	{
		test_get_resource_cached(argv[i]);
	}
	resource_getter_release();
	return main_ret;
}
