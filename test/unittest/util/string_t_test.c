#include <string.h>
#include "test_util.h"
#include "string_t.h"
#include <stdlib.h>

// 测试 string_init 函数
void test_string_init() {
    string_t s;
    string_init(&s);
    
    // 初始化后字符串应该为空
    TEST(string_len(&s) == 0);
	TEST(string_empty(&s));
	TEST(string_cstr(&s) != nullptr);
	TEST(string_cstr(&s)[0] == 0);
    
    string_destroy(&s);
}

// 测试 string_init_from_cstr 函数
void test_string_init_from_cstr() {
    string_t s;
    const char* test_str = "Hello World";
    
    bool result = string_init_from_cstr(&s, test_str);
    
    // 确保初始化成功，长度正确，内容匹配
    TEST(result && 
         string_len(&s) == strlen(test_str) && 
         !string_empty(&s) && 
         strcmp(string_cstr(&s), test_str) == 0);
    
    string_destroy(&s);
    
    // 测试空字符串
    result = string_init_from_cstr(&s, "");
    TEST(result && string_len(&s) == 0 && string_empty(&s));
    
    string_destroy(&s);
    
    // 测试NULL指针
    result = string_init_from_cstr(&s, NULL);
    TEST(result); // 应该返回false表示失败
	TEST(string_len(&s) == 0);
	TEST(string_cstr(&s) != NULL);
	TEST(string_cstr(&s)[0] == 0);
    
    // 即使失败也调用destroy确保安全
    string_destroy(&s);
}

// 测试 string_init_from_parts 函数
void test_string_init_from_parts() {
    string_t s;
    const char* test_str = "Hello World";
    
    // 正常情况
    bool result = string_init_from_parts(&s, test_str, 5); // 只取"Hello"
    TEST(result && 
         string_len(&s) == 5 && 
         !string_empty(&s) && 
         strcmp(string_cstr(&s), "Hello") == 0);
    
    string_destroy(&s);
    
    // 零长度
    result = string_init_from_parts(&s, test_str, 0);
    TEST(result && string_len(&s) == 0 && string_empty(&s));
    
    string_destroy(&s);
    
    // NULL指针但长度为0
    result = string_init_from_parts(&s, NULL, 0);
    TEST(result && string_len(&s) == 0); // 应该成功，因为长度为0时不需要复制数据
    
    string_destroy(&s);
    
    // NULL指针且长度大于0
    result = string_init_from_parts(&s, NULL, 5);
    TEST(!result); // 应该失败
    
    string_destroy(&s);
}

// 测试 string_init_from_string_view 函数
void test_string_init_from_string_view() {
    string_t s;
    const char* test_str = "Hello World";
    
    // 创建一个string_view
    string_view_t view = string_view_from_parts(test_str, 5); // "Hello"
    
    bool result = string_init_from_string_view(&s, view); // 注意这里传递值而不是指针
    TEST(result && 
         string_len(&s) == 5 && 
         !string_empty(&s) && 
         strcmp(string_cstr(&s), "Hello") == 0);
    
    string_destroy(&s);
    
    // 空字符串视图
    view = string_view_from_parts("", 0);
    result = string_init_from_string_view(&s, view);
    TEST(result && string_len(&s) == 0 && string_empty(&s));
    
    string_destroy(&s);
    
    // 包含空字符的字符串视图
    view = string_view_from_parts("Hello\0World", 11);
    result = string_init_from_string_view(&s, view);
    // 验证包含空字符的完整内容
    TEST(result && 
         string_len(&s) == 11 && 
         memcmp(string_cstr(&s), "Hello\0World", 11) == 0);
    
    string_destroy(&s);
}

// 测试 string_destroy 函数
void test_string_destroy() {
    string_t s;
    string_init(&s);
    
    // 没有直接方式测试销毁是否成功，主要测试调用后不崩溃
    string_destroy(&s);
    TEST(true); // 如果到达这里，没有崩溃
    
    // 测试重复销毁
    string_destroy(&s); // 理想情况下应该安全，但取决于实现
    TEST(true);
    
    // 测试非空字符串的销毁
    string_init_from_cstr(&s, "Hello World");
    string_destroy(&s);
    TEST(true);
}

// 测试 string_take_ownership 函数
void test_string_take_ownership() {
    string_t s;
    string_init(&s);
    
    // 分配一个字符串并让string_t接管它
    char* data = (char*)malloc(12);
    if (!data) {
        printf("Failed to allocate memory\n");
        return;
    }
    strcpy(data, "Hello World");
    
    string_take_ownership(&s, data, 12);
    
    // 验证字符串内容和长度
    TEST(strcmp(string_cstr(&s), "Hello World") == 0 && 
         string_len(&s) == 11);
    
    // 不应该再使用data，因为它现在由string_t管理
    // 这里不要调用free(data)!
    
    string_destroy(&s);
    
    // 测试空字符串的所有权接管
    data = (char*)malloc(1);
    if (!data) {
        printf("Failed to allocate memory\n");
        return;
    }
    data[0] = '\0';
    
    string_take_ownership(&s, data, 1);
    TEST(string_len(&s) == 0 && string_empty(&s));
    
    string_destroy(&s);
    
    // 测试包含空字符的字符串
    data = (char*)malloc(12);
    if (!data) {
        printf("Failed to allocate memory\n");
        return;
    }
    memcpy(data, "Hello\0World", 11);
    data[11] = '\0'; // 确保最后有终止符
    
    string_take_ownership(&s, data, 12);
    // 验证包含内部空字符的字符串（这依赖于实现如何处理内嵌空字符）
    TEST(string_len(&s) == 5 || string_len(&s) == 11); // 取决于实现是否将第一个\0视为结束
    
    string_destroy(&s);
}

// 测试 string_reserve 函数
void test_string_reserve() {
    string_t s;
    string_init(&s);
    
    // 测试增加容量
    bool result = string_reserve(&s, 100);
    
    // 保留内存成功后，字符串长度应该仍然为0，但容量已增加
    // 注意：我们没有直接访问容量的方法，只能通过后续操作间接验证
    TEST(result && string_len(&s) == 0 && string_empty(&s));
    
    // 验证保留的容量生效 - 尝试添加少于保留容量的内容
    // 如果保留成功，这应该不会导致内存重新分配
    char test_str[90];
    memset(test_str, 'A', 89);
    test_str[89] = '\0';
    
    result = string_append(&s, test_str);
    TEST(result && string_len(&s) == 89 && !string_empty(&s));
    
    // 尝试将容量减少到当前长度以下
    // 这通常会被忽略或调整为当前长度
    result = string_reserve(&s, 10);
    // 即使调用成功，容量不应该小于字符串当前长度
    TEST(string_len(&s) == 89);
    
    // 测试保留为0，这应该不会影响当前长度
    result = string_reserve(&s, 0);
    TEST(string_len(&s) == 89);
    
    string_destroy(&s);
    
    // 尝试为较长字符串保留空间，然后填满它
    string_init(&s);
    result = string_reserve(&s, 1024);
    TEST(result);
    
    char large_str[1000];
    memset(large_str, 'B', 999);
    large_str[999] = '\0';
    
    result = string_assign(&s, large_str);
    TEST(result && string_len(&s) == 999);
    
    string_destroy(&s);
}

// 测试 string_resize 函数
void test_string_resize() {
    string_t s;
    string_init(&s);
    
    // 测试增加大小 - 新增部分应填充空字符或NUL字节
    bool result = string_resize(&s, 10);
    TEST(result && string_len(&s) == 10 && !string_empty(&s));
    
    // 验证填充的内容 - 期望是空字符或NUL字节
    const char* content = string_cstr(&s);
    bool all_zeros = true;
    for (size_t i = 0; i < 10; i++) {
        if (content[i] != '\0') {
            all_zeros = false;
            break;
        }
    }
    TEST(all_zeros);
    
    // 测试减小大小，应截断字符串
    result = string_resize(&s, 3);
    TEST(result && string_len(&s) == 3);
    
    // 测试调整为0大小
    result = string_resize(&s, 0);
    TEST(result && string_len(&s) == 0 && string_empty(&s));
    
    // 直接初始化一个带内容的字符串
    string_destroy(&s);
    string_init_from_cstr(&s, "Hello World");
    
    // 测试减小大小，应截断字符串
    result = string_resize(&s, 5);
    TEST(result && string_len(&s) == 5 && strcmp(string_cstr(&s), "Hello") == 0);
    
    // 测试增加大小，原有内容应保留
    result = string_resize(&s, 8);
    TEST(result && 
         string_len(&s) == 8 && 
         strncmp(string_cstr(&s), "Hello", 5) == 0);
    
    // 验证新增部分填充
    content = string_cstr(&s);
    all_zeros = true;
    for (size_t i = 5; i < 8; i++) {
        if (content[i] != '\0') {
            all_zeros = false;
            break;
        }
    }
    TEST(all_zeros);
    
    // 在循环中测试各种大小
    string_destroy(&s);
    string_init(&s);
    
    // 测试递增的大小
    size_t test_sizes[] = {0, 1, 2, 5, 10, 32, 64, 128, 256, 512, 1024, 2048};
    size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (size_t i = 0; i < num_sizes; i++) {
        result = string_resize(&s, test_sizes[i]);
        TEST(result && string_len(&s) == test_sizes[i]);
        
        // 对于非零大小，验证内容
        if (test_sizes[i] > 0) {
            content = string_cstr(&s);
            
            // 检查开头、中间和结尾的几个位置
            size_t positions[] = {0, test_sizes[i]/4, test_sizes[i]/2, test_sizes[i] - 1};
            size_t num_positions = test_sizes[i] > 3 ? 4 : test_sizes[i];
            
            for (size_t j = 0; j < num_positions; j++) {
                if (positions[j] >= test_sizes[i]) continue;
                TEST(content[positions[j]] == '\0');
            }
        } else {
            TEST(string_empty(&s));
        }
    }
    
    // 测试递减的大小
    for (size_t i = num_sizes - 1; i > 0; i--) {
        result = string_resize(&s, test_sizes[i-1]);
        TEST(result && string_len(&s) == test_sizes[i-1]);
    }
    
    // 测试交替增减大小
    for (size_t i = 0; i < num_sizes - 1; i++) {
        // 先增加
        result = string_resize(&s, test_sizes[i+1]);
        TEST(result && string_len(&s) == test_sizes[i+1]);
        
        // 再减少
        result = string_resize(&s, test_sizes[i]);
        TEST(result && string_len(&s) == test_sizes[i]);
    }
    
    string_destroy(&s);
}

// 测试 string_assign 函数
void test_string_assign() {
    string_t s;
    string_init(&s);
    
    // 测试赋值普通字符串
    bool result = string_assign(&s, "Hello World");
    TEST(result);
    TEST(string_len(&s) == 11);
	TEST(!string_empty(&s));
	TEST(strcmp(string_cstr(&s), "Hello World") == 0);
    
    // 测试赋值空字符串
    result = string_assign(&s, "");
    TEST(result && string_len(&s) == 0 && string_empty(&s));
    
    // 测试赋值NULL
    result = string_assign(&s, NULL);
    TEST(result); 
	TEST(string_len(&s) == 0);
	TEST(string_cstr(&s) != nullptr);
	TEST(string_cstr(&s)[0] == 0);
    
    // 测试再次赋值（检查清除原有内容的功能）
    string_assign(&s, "First");
    result = string_assign(&s, "Second");
    TEST(result && 
         string_len(&s) == 6 && 
         strcmp(string_cstr(&s), "Second") == 0);
    
    // 测试赋值较长字符串
    char long_str[1000];
    memset(long_str, 'A', 999);
    long_str[999] = '\0';
    
    result = string_assign(&s, long_str);
    TEST(result && string_len(&s) == 999);
    
    // 验证长字符串内容
    const char* content = string_cstr(&s);
    bool all_as = true;
    for (size_t i = 0; i < 999; i++) {
        if (content[i] != 'A') {
            all_as = false;
            break;
        }
    }
    TEST(all_as);
    
    string_destroy(&s);
}

// 测试 string_assign_view 函数
void test_string_assign_view() {
    string_t s;
    string_init(&s);
    
    // 测试赋值普通视图
    string_view_t view = string_view_from_parts("Hello World", 11);
    bool result = string_assign_view(&s, view);
    TEST(result && 
         string_len(&s) == 11 && 
         !string_empty(&s) && 
         strcmp(string_cstr(&s), "Hello World") == 0);
    
    // 测试赋值部分视图
    view = string_view_from_parts("Hello World", 5); // 只取"Hello"
    result = string_assign_view(&s, view);
    TEST(result && 
         string_len(&s) == 5 && 
         !string_empty(&s) && 
         strcmp(string_cstr(&s), "Hello") == 0);
    
    // 测试赋值空视图
    view = string_view_from_parts("", 0);
    result = string_assign_view(&s, view);
    TEST(result && string_len(&s) == 0 && string_empty(&s));
    
    // 测试包含空字符的视图
    view = string_view_from_parts("Hello\0World", 11);
    result = string_assign_view(&s, view);
    // 验证完整内容，包括内嵌的空字符
    TEST(result && 
         string_len(&s) == 11 && 
         memcmp(string_cstr(&s), "Hello\0World", 11) == 0);
    
    string_destroy(&s);
}

// 测试 string_append 函数
void test_string_append() {
    string_t s;
    string_init(&s);
    
    // 测试向空字符串追加
    bool result = string_append(&s, "Hello");
    TEST(result && 
         string_len(&s) == 5 && 
         !string_empty(&s) && 
         strcmp(string_cstr(&s), "Hello") == 0);
    
    // 测试追加到非空字符串
    result = string_append(&s, " World");
    TEST(result && 
         string_len(&s) == 11 && 
         strcmp(string_cstr(&s), "Hello World") == 0);
    
    // 测试追加空字符串
    result = string_append(&s, "");
    TEST(result && 
         string_len(&s) == 11 && 
         strcmp(string_cstr(&s), "Hello World") == 0); // 长度和内容不变
    
    // 测试追加NULL
    result = string_append(&s, NULL);
    TEST(result);
	TEST(string_len(&s) == 11);
	TEST(strcmp(string_cstr(&s), "Hello World") == 0);
    
    // 测试追加较长字符串
    string_assign(&s, ""); // 先清空
    
    char long_str[500];
    memset(long_str, 'A', 499);
    long_str[499] = '\0';
    
    result = string_append(&s, long_str);
    TEST(result && string_len(&s) == 499);
    
    // 再追加一个长字符串
    memset(long_str, 'B', 499);
    long_str[499] = '\0';
    
    result = string_append(&s, long_str);
    TEST(result && string_len(&s) == 998);
    
    // 验证内容
    const char* content = string_cstr(&s);
    bool content_correct = true;
    
    for (size_t i = 0; i < 499; i++) {
        if (content[i] != 'A') {
            content_correct = false;
            break;
        }
    }
    
    for (size_t i = 499; i < 998; i++) {
        if (content[i] != 'B') {
            content_correct = false;
            break;
        }
    }
    
    TEST(content_correct);
    
    string_destroy(&s);
}

// 测试 string_append_parts 函数
void test_string_append_parts() {
    string_t s;
    string_init(&s);
    
    // 测试向空字符串追加部分
    const char* test_str = "Hello World";
    bool result = string_append_parts(&s, test_str, 5); // 只追加"Hello"
    TEST(result && 
         string_len(&s) == 5 && 
         !string_empty(&s) && 
         strcmp(string_cstr(&s), "Hello") == 0);
    
    // 测试向非空字符串追加部分
    result = string_append_parts(&s, test_str + 5, 6); // 追加" World"
    TEST(result && 
         string_len(&s) == 11 && 
         strcmp(string_cstr(&s), "Hello World") == 0);
    
    // 测试追加0长度
    result = string_append_parts(&s, test_str, 0);
    TEST(result && 
         string_len(&s) == 11 && 
         strcmp(string_cstr(&s), "Hello World") == 0); // 长度和内容不变
    
    // 测试追加NULL但长度为0
    result = string_append_parts(&s, NULL, 0);
    TEST(result && string_len(&s) == 11); // 应该成功
    
    // 测试追加NULL但长度大于0
    result = string_append_parts(&s, NULL, 5);
    TEST(!result); // 应该失败
    
    // 测试追加包含空字符的部分
    result = string_append_parts(&s, "Hello\0World", 11);
    TEST(result && 
         string_len(&s) == 22 && 
         memcmp(string_cstr(&s), "Hello WorldHello\0World", 22) == 0);
    
    string_destroy(&s);
    
    // 测试大量连续追加
    string_init(&s);
    
    for (int i = 0; i < 100; i++) {
        char tmp[4];
        snprintf(tmp, sizeof(tmp), "%03d", i);
        result = string_append_parts(&s, tmp, 3);
        TEST(result);
    }
    
    TEST(string_len(&s) == 300);
    
    // 随机检查几个位置
    const char* content = string_cstr(&s);
    TEST(memcmp(content, "000", 3) == 0);
    TEST(memcmp(content + 30, "010", 3) == 0);
    TEST(memcmp(content + 297, "099", 3) == 0);
    
    string_destroy(&s);
}

// 测试 string_append_view 函数
void test_string_append_view() {
    string_t s;
    string_init(&s);
    
    // 测试向空字符串追加视图
    string_view_t view = string_view_from_parts("Hello", 5);
    bool result = string_append_view(&s, view);
    TEST(result && 
         string_len(&s) == 5 && 
         !string_empty(&s) && 
         strcmp(string_cstr(&s), "Hello") == 0);
    
    // 测试向非空字符串追加视图
    view = string_view_from_parts(" World", 6);
    result = string_append_view(&s, view);
    TEST(result && 
         string_len(&s) == 11 && 
         strcmp(string_cstr(&s), "Hello World") == 0);
    
    // 测试追加空视图
    view = string_view_from_parts("", 0);
    result = string_append_view(&s, view);
    TEST(result && 
         string_len(&s) == 11 && 
         strcmp(string_cstr(&s), "Hello World") == 0); // 长度和内容不变
    
    // 测试追加包含空字符的视图
    view = string_view_from_parts("Hello\0World", 11);
    result = string_append_view(&s, view);
    TEST(result && 
         string_len(&s) == 22 && 
         memcmp(string_cstr(&s), "Hello WorldHello\0World", 22) == 0);
    
    string_destroy(&s);
}

// 测试 string_push_back 函数
void test_string_push_back() {
    string_t s;
    string_init(&s);
    
    // 测试向空字符串添加字符
    bool result = string_push_back(&s, 'A');
    TEST(result && 
         string_len(&s) == 1 && 
         !string_empty(&s) && 
         string_cstr(&s)[0] == 'A');
    
    // 测试多次添加字符
    result = string_push_back(&s, 'B');
    result &= string_push_back(&s, 'C');
    TEST(result && 
         string_len(&s) == 3 && 
         strcmp(string_cstr(&s), "ABC") == 0);
    
    // 测试添加空字符
    result = string_push_back(&s, '\0');
    // 注意：这里行为取决于实现。可能会将空字符作为正常字符添加，也可能视为字符串结束
    // 如果将空字符作为正常字符添加：
    if (string_len(&s) == 4) {
        TEST(result && memcmp(string_cstr(&s), "ABC\0", 4) == 0);
    }
    // 如果将空字符视为字符串结束：
    else {
        TEST(result && string_len(&s) == 3);
    }
    
    // 测试大量连续添加
    string_destroy(&s);
    string_init(&s);
    
    for (char c = 'A'; c <= 'Z'; c++) {
        result = string_push_back(&s, c);
        TEST(result);
    }
    
    TEST(string_len(&s) == 26 && 
         strncmp(string_cstr(&s), "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26) == 0);
    
    string_destroy(&s);
}

// 测试 string_cstr 的边界情况
void test_string_cstr_edge_cases() {
    printf("Testing string_cstr edge cases:\n");
    
    string_t s;
    string_init(&s);
    
    // 空字符串应返回有效指针，指向NUL终止符
    const char* empty_str = string_cstr(&s);
    TEST(empty_str != NULL && empty_str[0] == '\0');
    
    // 包含空字符的字符串
    string_assign(&s, "Hello");
    string_push_back(&s, '\0');
    string_append(&s, "World");
    
    // 检查string_cstr是否正确处理内部空字符
    const char* with_null = string_cstr(&s);
    if (string_len(&s) > 5) {
        // 如果实现保留内部空字符
        TEST(with_null != NULL && 
             memcmp(with_null, "Hello\0World", 11) == 0);
    } else {
        // 如果实现将内部空字符视为终止符
        TEST(with_null != NULL && 
             strcmp(with_null, "Hello") == 0);
    }
    
    string_destroy(&s);
}

// 测试 string_len 和 string_empty 的边界情况
void test_string_len_empty_edge_cases() {
    
    string_t s;
    string_init(&s);
    
    // 空字符串
    TEST(string_len(&s) == 0 && string_empty(&s));
    
    // 单字符字符串
    string_push_back(&s, 'A');
    TEST(string_len(&s) == 1 && !string_empty(&s));
    
    // 清空
    string_clear(&s);
    TEST(string_len(&s) == 0 && string_empty(&s));
    
    // 赋值后清空
    string_assign(&s, "Hello World");
    string_clear(&s);
    TEST(string_len(&s) == 0 && string_empty(&s));
    
    string_destroy(&s);
}

// 测试大字符串操作
void test_large_string_operations() {
    
    string_t s;
    string_init(&s);
    
    // 创建大字符串
    char large_str[10001];
    memset(large_str, 'X', 10000);
    large_str[10000] = '\0';
    
    // 赋值大字符串
    string_assign(&s, large_str);
    TEST(string_len(&s) == 10000 && !string_empty(&s));
    
    // 清空大字符串
    string_clear(&s);
    TEST(string_len(&s) == 0 && string_empty(&s));
    
    string_destroy(&s);
}

// 测试基本字符串比较
void test_string_compare_basic() {
    
    string_t s1, s2;
    string_init(&s1);
    string_init(&s2);
    
    // 比较两个空字符串
    TEST(string_compare(&s1, &s2) == 0);
    
    // 比较空字符串和非空字符串
    string_assign(&s2, "A");
    TEST(string_compare(&s1, &s2) < 0);
    TEST(string_compare(&s2, &s1) > 0);
    
    // 比较相同字符串
    string_assign(&s1, "Hello");
    string_assign(&s2, "Hello");
    TEST(string_compare(&s1, &s2) == 0);
    
    // 比较不同长度但前缀相同的字符串
    string_assign(&s2, "HelloWorld");
    TEST(string_compare(&s1, &s2) < 0);
    TEST(string_compare(&s2, &s1) > 0);
    
    // 比较相同长度但内容不同的字符串
    string_assign(&s2, "Hellx");
    TEST(string_compare(&s1, &s2) < 0); // 'o' < 'x'
    
    string_destroy(&s1);
    string_destroy(&s2);
}

// 测试包含NUL字符的字符串比较
void test_string_compare_with_nulls() {
    
    string_t s1, s2;
    string_init(&s1);
    string_init(&s2);
    
    // 设置带内部NUL的字符串
    string_assign(&s1, "Hello");
    string_assign(&s2, "Hellx");
    
    string_resize(&s1, 10); // "Hello\0\0\0\0\0"
    string_resize(&s2, 10); // "Hellx\0\0\0\0\0"
    TEST(string_compare(&s1, &s2) < 0); // 应该仍然 'o' < 'x'
    
    // 比较完全相同但包含NUL字符的字符串
    string_assign(&s1, "Hello");
    string_resize(&s1, 10); // "Hello\0\0\0\0\0"
    string_assign(&s2, "Hello");
    string_resize(&s2, 10); // "Hello\0\0\0\0\0"
    TEST(string_compare(&s1, &s2) == 0);
    
    string_destroy(&s1);
    string_destroy(&s2);
}

// 测试大字符串比较
void test_large_string_compare() {
    
    string_t s1, s2;
    string_init(&s1);
    string_init(&s2);
    
    // 创建大字符串
    char large_str[10001];
    memset(large_str, 'X', 10000);
    large_str[10000] = '\0';
    
    // 相同大字符串比较
    string_assign(&s1, large_str);
    string_assign(&s2, large_str);
    TEST(string_compare(&s1, &s2) == 0);
    
    // 修改大字符串的最后一个字符
    large_str[9999] = 'Y';
    string_assign(&s2, large_str);
    TEST(string_compare(&s1, &s2) < 0); // 'X' < 'Y'
    
    string_destroy(&s1);
    string_destroy(&s2);
}

// 测试与C字符串比较
void test_string_compare_cstr() {
    
    string_t s;
    string_init(&s);
    
    // 比较空字符串和空C字符串
    TEST(string_compare_cstr(&s, "") == 0);
    
    // 比较空字符串和非空C字符串
    TEST(string_compare_cstr(&s, "A") < 0);
    
    // 比较非空字符串和空C字符串
    string_assign(&s, "Hello");
    TEST(string_compare_cstr(&s, "") > 0);
    
    // 比较相同内容
    TEST(string_compare_cstr(&s, "Hello") == 0);
    
    // 比较不同长度但前缀相同
    TEST(string_compare_cstr(&s, "HelloWorld") < 0);
    
    // 比较相同长度但内容不同
    TEST(string_compare_cstr(&s, "Hellx") < 0); // 'o' < 'x'
    
    string_destroy(&s);
}

// 测试与NULL和特殊C字符串比较
void test_string_compare_cstr_special() {
    
    string_t s;
    string_init(&s);
    string_assign(&s, "Hello");
    
    // 比较与NULL指针
    // 注意：如果实现不允许NULL指针，这个测试可能会导致崩溃
    // 建议在实际使用中单独测试这个情况或添加条件检查
    TEST(string_compare_cstr(&s, NULL) > 0); // 假设实现将NULL视为空字符串
    
    // 比较包含NUL字符的字符串
    string_resize(&s, 10); // "Hello\0\0\0\0\0"
    TEST(string_compare_cstr(&s, "Hello") > 0);
    
    string_destroy(&s);
}

// 在main中添加这两个测试
int main() {
    printf("Running string initialization and ownership tests...\n");
    
    test_string_init();
    test_string_init_from_cstr();
    test_string_init_from_parts();
    test_string_init_from_string_view();
    test_string_destroy();
    test_string_take_ownership();
    test_string_reserve();
    test_string_resize();
	test_string_assign();
	test_string_assign_view();
    test_string_append();
    test_string_append_parts();
    test_string_append_view();
    test_string_push_back();
	test_string_cstr_edge_cases();
	test_string_len_empty_edge_cases();
	test_large_string_operations();
	test_string_compare_basic();
	test_string_compare_with_nulls();
	test_large_string_compare();
	test_string_compare_cstr();
	test_string_compare_cstr_special();
    printf("All tests completed.\n");
    return main_ret;
}

