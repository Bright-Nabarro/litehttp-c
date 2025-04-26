#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include "configure.h"
#include "base.h"
#include "reactor.h"
#include "server.h"
#include "resource_getter.h"

static void initial();

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	initial();
	http_err_t herr;	

	int server_fd;
	herr = get_ipv4_server_fd(&server_fd);
	if (herr != http_success)
		return 1;
	herr = http_signal(SIGINT, ipv4_terminal_fn);
	if (herr != http_success)
		return 1;

	herr = handle_ipv4_main_loop(server_fd);
	close(server_fd);

	thdpool_release();
	release_config_file();
	resource_getter_release();
	return 0;
}

static void initial()
{
	http_err_t herr;
	herr = parse_config_file("config/config.toml");

	char path[PATH_MAX];
	getcwd(path, sizeof(path));
	printf("work on %s\n", path);

	if (herr != http_success)
	{
		exit(1);
	}

	herr = log_config();
	if (herr != http_success)
		log_http_message(http_err_to_msg(herr));

	herr = server_config();
	if (herr == http_unset_fatal)
	{
		log_http_message(http_err_to_msg(herr));
		exit(1);
	}

	http_err_t extra = core_config();
	if (extra == http_unset_fatal)
	{
		log_http_message(http_err_to_msg(extra));
		exit(1);
	}

	if (herr != http_success || extra != http_success)
		log_http_message(http_err_to_msg(herr == http_success ? extra : herr));
	
	thdpool_initial();
	resource_getter_initial();
}

