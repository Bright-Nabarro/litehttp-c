#pragma once
#include <stdio.h>
#include <ctp/logger.h>

static int main_ret = 0;

#define TEST(cond)                                                             \
	do                                                                         \
	{                                                                          \
		if (cond)                                                              \
		{                                                                      \
			/*printf("%s: PASS\n", __func__);*/                                \
		}                                                                      \
		else                                                                   \
		{                                                                      \
			printf("%s:%d: FAIL\n", __func__, __LINE__);                       \
			main_ret = 1;                                                      \
		}                                                                      \
	} while (0)

static void logger_init()
{
	ctp_logger_config_t config;
	config.async = false;
	config.log_file = NULL;
	config.msg_max_len = 2048;

	ctp_logger_global_init(&config);
}
