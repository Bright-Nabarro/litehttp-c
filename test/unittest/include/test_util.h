#pragma once
#include <stdio.h>

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
