#include "string_t.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#define STR_SMALL_BUF_SIZE 16

// 当capacity为0，len > 0时，代表使用了小缓冲区
typedef struct  
{
	alignas(STR_SMALL_BUF_SIZE) char small_buf[STR_SMALL_BUF_SIZE];
	alignas(sizeof(char*)) char* data;
	alignas(sizeof(size_t)) size_t size; 		//存储实际字符串长度，包括结尾0
	alignas(sizeof(size_t)) size_t capacity;	//存储容量，包括结尾0
} string_impl_t;

static_assert(sizeof(string_impl_t) == sizeof(string_t));

typedef struct
{
	const char* data;
	alignas(sizeof(char*)) const size_t len;
} sv_impl_t;

static_assert(sizeof(sv_impl_t) == sizeof(string_view_t));

static size_t next_capacity(size_t n);
static bool cpy_smbuf2data(string_impl_t* s, size_t new_cap);
inline static bool use_sbuf(const string_impl_t* s);
static 
void cpy_sv2str(string_impl_t* dest, string_view_t src, int offset);

static inline
string_impl_t* str2impl(string_t* sp);
static inline
string_t* impl2str(string_impl_t* s);
static inline
const string_impl_t* str2impl_const(const string_t* sp);
[[maybe_unused]] static inline 
const string_t* impl2str_const(const string_impl_t* s);

static inline 
string_view_t* impl2sv(sv_impl_t* sv);
static inline
const sv_impl_t* sv2impl_const(const string_view_t* svp);
/****
 * STRING_T INTERFACE IMPLEMENT
 ****/

void string_init(string_t* sp)
{
	string_impl_t* s = str2impl(sp);
	assert(s != nullptr);
	memset(s, 0, sizeof(string_t));
	s->data = s->small_buf;
}

bool string_init_from_cstr(string_t* s, const char* cstr)
{
	assert(s != nullptr);
	string_view_t sv = string_view_from_cstr(cstr);
	return string_init_from_string_view(s, sv);
}

bool string_init_from_parts(string_t* s, const char* cstr, size_t len)
{
	assert(s != nullptr);
	if (cstr == nullptr && len != 0)
	{
		errno = EINVAL;
		return false;
	}
	string_view_t sv = string_view_from_parts(cstr, len);
	return string_init_from_string_view(s, sv);
}

bool string_init_from_string_view(string_t* sp, const string_view_t sv)
{
	string_impl_t* s = str2impl(sp);
	assert(s != nullptr);
	string_init(impl2str(s));
	if (string_view_cstr(sv) == nullptr)
	{
		assert(string_view_len(sv) == 0);
		return true;
	}

	if (string_view_len(sv) < STR_SMALL_BUF_SIZE)
	{
		s->data = s->small_buf;
		cpy_sv2str(s, sv, 0);
		return true;
	}

	size_t new_capacity = next_capacity(string_view_len(sv)+1);
	s->data = malloc(new_capacity);
	if (s->data == nullptr)
		return false;
	cpy_sv2str(s, sv, 0);
	s->capacity = new_capacity;
	return true;
}

void string_destroy(string_t* sp)
{
	string_impl_t* s = str2impl(sp);
	if (s == nullptr)
		return;
	if (s->data == s->small_buf)
	{
		assert(s->capacity == 0);
		return;
	}

	free(s->data);
}

void string_take_ownership(string_t* sp, char* data, size_t len)
{
	assert(sp != nullptr);
	string_impl_t* s = str2impl(sp);
	s->size = len+1;
	s->capacity = s->size;
	s->data = data;
}

bool string_reserve(string_t* sp, size_t capacity)
{
	string_impl_t* s = str2impl(sp);
	assert(s != nullptr);
	assert(s->data != nullptr);
	if (capacity <= s->size)
	{
		errno = EINVAL;
		return false;
	}
	else if (s->capacity >= capacity)
	{
		return true;
	}
	
	capacity = next_capacity(capacity);
	if (use_sbuf(s))
		return cpy_smbuf2data(s, capacity);

	char* new_data = realloc(s->data, capacity);
	if (new_data == nullptr)
		return false;
	s->data = new_data;
	s->capacity = capacity;
	return true;
}

bool string_resize(string_t* sp, size_t new_len)
{
	string_impl_t* s = str2impl(sp);
	assert(s != nullptr);
	assert(s->data != nullptr);
	if (new_len == 0)
	{
		string_clear(impl2str(s));
		return true;
	}
	// 包括两个情况，第一个是small_buf, 第二个是动态分配
	if (new_len < s->size)
	{
		s->size = new_len+1;
		s->data[new_len] = 0;
		return true;
	}
	if (new_len < s->capacity)
	{
		memset(s->data + s->size, 0, new_len - s->size+1);
		s->size = new_len+1;
		s->data[new_len] = 0;
		return true;
	}
	
	// string为短小字符，new_len超过原存储内容情况
	if (use_sbuf(s))
	{
		assert(s->data == s->small_buf);
		// 短小字符仍然能够存放 new_len
		if (new_len < STR_SMALL_BUF_SIZE)
		{
			memset(s->data+s->size, 0, new_len-s->size+1);
			s->size = new_len+1;
			s->data[new_len] = 0;
			return true;
		}
		size_t capacity = next_capacity(new_len+1);
		if (!cpy_smbuf2data(s, capacity))
			return false;
		memset(s->data + s->size, 0, new_len-s->size+1);
		s->size = new_len+1;
		s->capacity = capacity;
		s->data[new_len] = 0;
		return true;
	}
	
	// string为动态分配，len 超过capacity情况
	size_t capacity = next_capacity(new_len+1);
	char* new_data = realloc(s->data, capacity);
	if (new_data == nullptr)
		return false;
	s->data = new_data;
	memset(s->data + s->size, 0, new_len-s->size+1);
	s->size = new_len+1;
	s->capacity = capacity;
	return true;
}

bool string_assign(string_t* s, const char* cstr)
{
	string_view_t sv = string_view_from_cstr(cstr);
	return string_assign_view(s, sv);
}

bool string_assign_view(string_t* sp, const string_view_t sv)
{
	string_impl_t* s = str2impl(sp);
	assert(s != nullptr);
	assert(s->data != nullptr);
	size_t new_capacity = next_capacity(string_view_len(sv) + 1);
	if (sv2impl_const(&sv)->data == nullptr)
	{
		assert(sv2impl_const(&sv)->len == 0);
		string_clear(sp);
		return true;
	}

	if (use_sbuf(s))
	{
		if (string_view_len(sv) < STR_SMALL_BUF_SIZE)
		{
			cpy_sv2str(s, sv, 0);
			return true;
		}
		char* new_data = malloc(new_capacity);
		if (new_data == nullptr)
			return false;
		s->data = new_data;
		
		cpy_sv2str(s, sv, 0);
		s->capacity = new_capacity;
		memset(s->small_buf, 0, STR_SMALL_BUF_SIZE);
		return true;
	}
	
	if (string_view_len(sv) < s->capacity)
	{
		cpy_sv2str(s, sv, 0);
		return true;
	}

	char* new_data = realloc(s->data, new_capacity);
	if (new_data == nullptr)
	{
		return false;
	}
	s->data = new_data;
	s->data[string_view_len(sv)] = 0;
	s->size = string_view_len(sv)+1;
	s->capacity = new_capacity;

	return true;
}

bool string_append_cstr(string_t* s, const char* cstr)
{
	assert(s != nullptr);
	if (cstr == nullptr)
	{
		errno = EINVAL;
		return false;
	}

	string_view_t sv = string_view_from_cstr(cstr);
	return string_append_view(s, sv);
}

bool string_append_string(string_t* s, const string_t* append_s)
{
	assert(s != nullptr);
	string_view_t sv = string_view_from_string(append_s);
	return string_append_view(s, sv);
}

bool string_append_parts(string_t* s, const char* cstr, size_t len)
{
	assert(s != nullptr);
	if (cstr == nullptr && len > 0)
	{
		errno = EINVAL;
		return false;
	}
	string_view_t sv = string_view_from_parts(cstr, len);
	return string_append_view(s, sv);
}

bool string_append(string_t* s, const char* cstr)
{
	assert(s != nullptr);
	string_view_t sv = string_view_from_cstr(cstr);
	return string_append_view(s, sv);
}

bool string_append_view(string_t* sp, string_view_t sv)
{
	string_impl_t* s = str2impl(sp);
	assert(s != nullptr);
	size_t old_len = string_len(impl2str(s));
	size_t new_size = string_view_len(sv) + old_len + 1;
	size_t new_capacity = next_capacity(new_size);
	
	if (use_sbuf(s))
	{
		if (!cpy_smbuf2data(s, new_capacity))
			return false;
		cpy_sv2str(s, sv, old_len);
		s->capacity = new_capacity;
		return true;
	}

	char* new_data = realloc(s->data, new_capacity);
	if (new_data == nullptr)
		return false;
	s->data = new_data;
	cpy_sv2str(s, sv, old_len);
	s->capacity = new_capacity;

	return true;
}

bool string_push_back(string_t* s, char ch)
{
	assert(s != nullptr);
	string_view_t sv = string_view_from_parts(&ch, 1);
	string_append_view(s, sv);
	return true;
}

const char* string_cstr(const string_t* sp)
{
	const string_impl_t* s = str2impl_const(sp);
	assert(s != nullptr);
	return s->data;
}

size_t string_len(const string_t* sp)
{
	const string_impl_t* s = str2impl_const(sp);
	assert(s != nullptr);
	return s->size == 0 ? 0 : s->size-1;
}

bool string_empty(const string_t* s)
{
	return string_len(s) == 0;
}

void string_clear(string_t* sp)
{
	string_impl_t* s = str2impl(sp);
	assert(s != nullptr);
	s->size = 0;
	if (use_sbuf(s))
	{
		return;
	}
	free(s->data);
	s->capacity = 0;
	s->data = s->small_buf;
}

int string_compare(const string_t* lhsp, const string_t* rhsp)
{
	return string_view_compare(string_view_from_string(lhsp),
							   string_view_from_string(rhsp));
}

int string_compare_cstr(const string_t* lhsp, const char* rhs)
{
	return string_view_compare(string_view_from_string(lhsp),
			string_view_from_cstr(rhs));
}

// -- string_view_t --
string_view_t string_view_from_cstr(const char* cstr)
{
	if (cstr == nullptr)
		return string_view_from_parts(nullptr, 0);
	else
		return string_view_from_parts(cstr, strlen(cstr));
}

string_view_t string_view_from_parts(const char* data, size_t len)
{
	if (data == nullptr)
		len = 0;
	sv_impl_t sv = {.data = data, .len = len};
	return *impl2sv(&sv);
}

string_view_t string_view_from_string(const string_t* sp)
{
	const string_impl_t* s = str2impl_const(sp);
	assert(s != nullptr);
	size_t len = s->size ? s->size -1  : 0;
	return string_view_from_parts(s->data, len);
}

size_t string_view_len(string_view_t svp)
{
	const sv_impl_t* sv = sv2impl_const(&svp);
	assert(sv != nullptr);
	return sv->len;
}
bool string_view_empty(string_view_t sv)
{
	return string_view_len(sv) == 0;
}

const char* string_view_cstr(string_view_t svp)
{
	const sv_impl_t* sv = sv2impl_const(&svp);
	assert(sv != nullptr);
	return sv->data;
}

int string_view_compare(string_view_t lhsp, string_view_t rhsp)
{
	const sv_impl_t* lhs = sv2impl_const(&lhsp);
	const sv_impl_t* rhs = sv2impl_const(&rhsp);
	assert(lhs != nullptr);
	size_t llen = string_view_len(lhsp), rlen = string_view_len(rhsp);
	size_t minlen = llen < rlen ? llen : rlen;
	int cmp = 0;
	if (minlen > 0)
		cmp = strncmp(lhs->data, rhs->data, minlen);
	if (cmp != 0)
		return cmp;
	return llen - rlen;
}

bool string_view_equal(string_view_t lhs, string_view_t rhs)
{
	return string_view_compare(lhs, rhs) == 0;
}

bool string_view_to_buf(string_view_t sv, char* buf, size_t buf_len)
{
	size_t sv_len = string_view_len(sv);
	if (sv_len >= buf_len)
	{
		errno = EINVAL;
		return false;
	}

	memcpy(buf, string_view_cstr(sv), sv_len);
	buf[sv_len] = 0;
	return true;
}

int string_view_compare_cstr(string_view_t lhs, const char* rhs)
{
	return string_view_compare(lhs, string_view_from_cstr(rhs));
}

size_t string_view_hash(string_view_t sv)
{
	size_t hash = 5381;
	const char* str = string_view_cstr(sv);
	const size_t len = string_view_len(sv);

	for (size_t i = 0; i < len; ++i)
	{
		hash = ((hash << 5) + hash) + str[i];
	}
	return hash;
}

/*****
 * STATIC
 *****/

static
size_t next_capacity(size_t n)
{
	if (n == 0)
		return 1;
	if ((n & (n-1)) == 0)
		return n;

#if defined(__GNUC__) || defined(__clang__)
	size_t bits = sizeof(size_t) * 8;
	return 1 << (bits - __builtin_clzl(n));
#else
	size_t ret = 1;
	while(n > 0)
	{
		ret <<= 1;
		n >>= 0;
	}
	return ret;
#endif
}

static bool cpy_smbuf2data(string_impl_t* s, size_t new_cap)
{
	assert(s->capacity == 0);
	assert(s->data == s->small_buf);
	assert(new_cap != 0);

	s->data = malloc(new_cap);
	if (s->data == nullptr)
	{
		s->data = s->small_buf;
		return false;
	}
	memcpy(s->data, s->small_buf, s->size);
	s->capacity = new_cap;
	return true;
}

static 
void cpy_sv2str(string_impl_t* dest, string_view_t srcp, int offset)
{
	const sv_impl_t* src = sv2impl_const(&srcp);
	memcpy(dest->data + offset, src->data, src->len);
	dest->data[offset+src->len] = 0;
	dest->size = offset+src->len+1;
}
static inline
string_impl_t* str2impl(string_t* sp)
{
	return (string_impl_t*)sp;
}

static inline
string_t* impl2str(string_impl_t* s)
{
	return (string_t*)s;
}

static inline
const string_impl_t* str2impl_const(const string_t* sp)
{
	return (const string_impl_t*)sp;
}

static inline
const string_t* impl2str_const(const string_impl_t* s)
{
	return (const string_t*)s;
}

static inline
const sv_impl_t* sv2impl_const(const string_view_t* svp)
{
	return (const sv_impl_t*) svp;
}

static inline 
string_view_t* impl2sv(sv_impl_t* sv)
{
	return (string_view_t*)sv;
}

inline static bool use_sbuf(const string_impl_t* s)
{
	if (s->data == s->small_buf)
	{
		assert(s->capacity == 0);
		return true;
	}
	return false;
}
