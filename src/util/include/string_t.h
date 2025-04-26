#pragma once

#include <unistd.h>
#include <stdint.h>

typedef struct 
{
	uint8_t _private[48];
} string_t;

typedef struct
{
	uint8_t _private[16];
} string_view_t;

// ---- string_t ----
void string_init(string_t* s);
bool string_init_from_cstr(string_t* s, const char* cstr);
bool string_init_from_parts(string_t* s, const char* cstr, size_t len);
bool string_init_from_string_view(string_t* s, const string_view_t sv);
void string_destroy(string_t* s);
/**
 * @note char* data 必须是由外部 malloc/calloc/realloc 分配的data \
 * 调用后不得再自行 free(data)
 * @param len 给字符指针分配的完整长度，包括结尾0
 */
void string_take_ownership(string_t* s, char* data, size_t len);

/**
 * @note 禁止传入未初始化的string_t
 */
bool string_reserve(string_t* s, size_t capacity);
/**
 * @note 禁止传入未初始化的string_t
 */
bool string_resize(string_t* s, size_t new_len);

bool string_assign(string_t* s, const char* cstr);
bool string_assign_view(string_t* s, string_view_t sv);
bool string_append(string_t* s, const char* cstr);
bool string_append_cstr(string_t* s, const char* cstr);
bool string_append_parts(string_t* s, const char* cstr, size_t len);
bool string_append_string(string_t* s, const string_t* append_s);
bool string_append_view(string_t* s, string_view_t sv);
bool string_push_back(string_t* s, char ch);

const char* string_cstr(const string_t* s); // 以 NUL 结尾，便于与C标准库兼容
size_t string_len(const string_t* s);
bool string_empty(const string_t* s);
void string_clear(string_t* s);

int string_compare(const string_t* lhs, const string_t* rhs);
int string_compare_cstr(const string_t* lhs, const char* rhs);


// --- string_view_t ----
string_view_t string_view_from_cstr(const char* cstr);
/**
 * @note 如果data = NULL, 则自动截断len = 0, 其他情况需要用户保证
 */
string_view_t string_view_from_parts(const char* data, size_t len);
/**
 * @note 需要仔细管理string的生命周期, 不可变动
 */
string_view_t string_view_from_string(const string_t* s);

size_t string_view_len(string_view_t sv);
bool string_view_empty(string_view_t sv);
/**
 * @note 只有string_view_t 完全对应于一个以0结尾的cstr才能返回预期结果、
 * 		 需要由调用者保证
 */
const char* string_view_cstr(string_view_t sv);
bool string_view_to_buf(string_view_t sv, char* buf, size_t buf_len);
int string_view_compare(string_view_t lhs, string_view_t rhs);
bool string_view_equal(string_view_t lhs, string_view_t rhs);
int string_view_compare_cstr(string_view_t lhs, const char* rhs);
size_t string_view_hash(string_view_t sv);

//// 可选：查找、子串等
//ssize_t string_find(const string_t* s, const char* substr);
//string_view_t string_subview(const string_view_t* view, size_t start, size_t len);

