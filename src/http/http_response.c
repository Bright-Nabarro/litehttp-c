#include "http_response.h"

string_view_t get_simple_response()
{
	static const char* post = "HTTP/1.1 200 OK\r\n"
							  "Content-Type: text/html; charset=UTF-8\r\n"
							  "Content-Length: 25\r\n"
							  "\r\n"
							  "<html>Hello, world!</html>\r\n";
	return string_view_from_cstr(post);
}
