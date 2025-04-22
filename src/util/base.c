#include "base.h"

#include <unistd.h>
#include <stdarg.h>
#include <signal.h>

void log_http_message(http_message_t name, ...)
{
	va_list args;
	va_start(args, name);
	ctp_logger_global_logv(http_message_level(name), http_message_str(name), args);
	va_end(args);
}

http_err_t http_signal(int signo, void(*fn)(int))
{
	struct sigaction sa;
	sa.sa_handler = fn;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(signo, &sa, nullptr) < 0)
	{
		log_http_message_with_errno(HTTP_SIGACTION_ERR);
		return http_sigaction;
	}
	return http_success;
}
