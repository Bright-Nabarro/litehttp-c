#include "resource_getter.h"

#include "base.h"
#include "string_t.h"
#include <assert.h>
#include <fcntl.h>
#include <klib/khash.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef struct 
{
	size_t len;
	void* data;
} mapped_memory_t;

KHASH_INIT(path2resmem, string_view_t, mapped_memory_t, true, string_view_hash,
		   string_view_equal);
static khash_t(path2resmem)* table = nullptr;
static pthread_mutex_t table_mtx;

static http_err_t get_resource_lazy(string_view_t path, string_view_t* res, bool flush);
static http_err_t open_resource(string_view_t path, mapped_memory_t* mem);
static http_err_t close_resource(mapped_memory_t mem);

/****
 * INTERFACE IMPLAMENTATION
 ****/
http_err_t resource_getter_initial()
{
	table = kh_init(path2resmem);
	if (table == nullptr)
	{
		log_http_message_with_errno(HTTP_MALLOC_ERR);
		return http_malloc;
	}
	int err = pthread_mutex_init(&table_mtx, nullptr);
	if (err != 0)
	{
		log_http_message_with_ec(err, HTTP_PTHREAD_MUTEX_INIT_ERR);
		return http_pthread_mutex_init_err;
	}
	return http_success;
}

http_err_t resource_getter_release()
{
	if (table == nullptr)
	{
		log_http_message(HTTP_HASH_UNINITIALIZED_ERR);
		return http_hash_uninitialized_err;
	}

	khiter_t kitr;
	for (kitr = kh_begin(table); kitr != kh_end(table); ++kitr)
	{
		if (!kh_exist(table, kitr))
			continue;
		mapped_memory_t resmem = kh_value(table, kitr);
		close_resource(resmem);
	}

	kh_destroy(path2resmem, table);
	int err = pthread_mutex_destroy(&table_mtx);
	if (err != 0)
	{
		log_http_message_with_ec(err, HTTP_PTHREAD_MUTEX_DESTROY_ERR);
		return http_pthread_mutex_destroy_err;
	}

	table = nullptr;
	return http_success;
}

http_err_t get_resource_cached(string_view_t path, string_view_t* res)
{
	return get_resource_lazy(path, res, false);
}

http_err_t get_resource_refreshed(string_view_t path, string_view_t* res)
{
	return get_resource_lazy(path, res, true);
}

/****
 * STATIC
 ****/

static http_err_t get_resource_lazy(string_view_t path, string_view_t* res, bool flush)
{
	int err = 0;
	if (table == nullptr)
	{
		log_http_message(HTTP_HASH_UNINITIALIZED_ERR);
		return http_hash_uninitialized_err;
	}


	err = pthread_mutex_lock(&table_mtx);
	if (err != 0)
	{
		log_http_message_with_ec(err, HTTP_PTHREAD_MUTEX_LOCK);
		return http_pthread_mutex_lock_err;
	}

	// 查找res是否已经读取
	khiter_t kitr = kh_get(path2resmem, table, path);
	mapped_memory_t resmem = {0};

	if (kitr != kh_end(table) && !flush)
	{
		resmem = kh_value(table, kitr);
	}
	else
	{
		err = pthread_mutex_unlock(&table_mtx);
		if (err != 0)
		{
			log_http_message_with_ec(err, HTTP_PTHREAD_MUTEX_UNLOCK);
			return http_pthread_mutex_unlock_err;
		}
		open_resource(path, &resmem);
		*res = string_view_from_parts(resmem.data, resmem.len);
		err = pthread_mutex_lock(&table_mtx);
		if (err != 0)
		{
			log_http_message_with_ec(err, HTTP_PTHREAD_MUTEX_LOCK);
			return http_pthread_mutex_lock_err;
		}
		int ret;
		kitr = kh_put(path2resmem, table, path, &ret);
		assert(ret != 0);
		kh_value(table, kitr) = resmem;
	}

	err = pthread_mutex_unlock(&table_mtx);
	if (err != 0)
	{
		log_http_message_with_ec(err, HTTP_PTHREAD_MUTEX_UNLOCK);
		return http_pthread_mutex_unlock_err;
	}

	return http_success;
}

static http_err_t open_resource(string_view_t path, mapped_memory_t* mem)
{
	char path_buf[string_view_len(path)+1];
	string_view_to_buf(path, path_buf, sizeof(path_buf));
	
	int res_fd = open(path_buf, O_RDONLY);
	if (res_fd < 0)
	{
		log_http_message_with_errno(HTTP_OPEN_ERR, path_buf);
		return http_open_err;
	}
	
	struct stat res_stat;
	if (fstat(res_fd, &res_stat) < 0)
	{
		log_http_message_with_errno(HTTP_FSTAT_ERR, path_buf);
		return http_fstat_err;
	}

	mem->len = res_stat.st_size;
	mem->data = mmap(nullptr, mem->len, PROT_READ, MAP_PRIVATE, res_fd, 0);
	if (mem->data == MAP_FAILED)
	{
		log_http_message_with_errno(HTTP_MMAP_ERR, res_fd);
		return http_mmap_err;
	}
	close(res_fd);

	log_debug_message("mmaped file %.*s\n", (int)mem->len, mem->data);
	return http_success;
}

static http_err_t close_resource(mapped_memory_t mem)
{
	if (munmap(mem.data, mem.len) < 0)
	{
		log_http_message_with_errno(HTTP_MUNMAP_ERR);
	}
	return http_success;
}

