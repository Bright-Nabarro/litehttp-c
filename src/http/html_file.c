#include <klib/khash.h>
#include <pthread.h>
#include <stdio.h>
#include <assert.h>

#include "html_file.h"

//KHASH_MAP_INIT_STR(path2html, string_t*);
//static khash_t(path2html)* table = NULL;
//static pthread_mutex_t table_mtx;
//
//static http_err_t read_file2sv(string_t** html, const char* path);
//
//void initial_table()
//{
//	table = kh_init(path2html);
//	pthread_mutex_init(&table_mtx, NULL);
//}
//
//http_err_t get_html_lazy(const char* filename, string_t** html_post)
//{
//	static pthread_once_t once = PTHREAD_ONCE_INIT;
//	pthread_once(&once, initial_table);
//
//	pthread_mutex_lock(&table_mtx);
//	khiter_t kitr = kh_get(path2html, table, filename);
//	if (kitr == kh_end(table))
//	{
//		pthread_mutex_unlock(&table_mtx);
//		string_t* html;
//		http_err_t ec = read_file2sv(&html, filename);
//		if (ec != http_success)
//			return ec;
//		pthread_mutex_lock(&table_mtx);
//		int ret;
//		kitr = kh_put(path2html, table, filename, &ret);
//		assert(ret != 0);
//		kh_value(table, kitr) = html;
//		*html_post = html;
//		pthread_mutex_unlock(&table_mtx);
//	}
//	else
//	{
//		*html_post = kh_value(table, kitr);
//		pthread_mutex_unlock(&table_mtx);
//	}
//	return http_success;
//}
//
//http_err_t clean_html_res()
//{
//	kh_destroy(path2html, table);
//	pthread_mutex_destroy(&table_mtx);
//
//	return http_success;
//}
//
//static http_err_t read_file2sv(string_t** html, const char* path)
//{
//	FILE* html_file = fopen(path, "r");
//	if (html_file == NULL)
//		return http_path_err;
//
//	fseek(html_file, 0, SEEK_END);
//	size_t file_size = ftell(html_file);
//	fseek(html_file, 0, SEEK_SET);
//
//	*html = string_view_empty(file_size);
//	if (*html == NULL)
//		return http_malloc;
//	size_t readbytes = fread((*html)->data, 1, file_size, html_file);
//	if (readbytes != file_size)
//	{
//		fclose(html_file);
//		string_free(*html);
//		return http_fread_err;
//	}
//
//	fclose(html_file);
//	return http_success;
//}
//
