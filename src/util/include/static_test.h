#pragma once

#ifdef UNIT_TEST
#define UT_STATIC extern
#else
#define UT_STATIC static
#endif

