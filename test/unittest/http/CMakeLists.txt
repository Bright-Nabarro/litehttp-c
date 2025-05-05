set(targets
	resource_getter
	http_parse_dfs
)

set(libs
	ut_util
	ut_http
)

add_test_from_targets("${targets}" "${libs}")

target_include_directories(http_parse_dfs PRIVATE "${CMAKE_SOURCE_DIR}/src/http")
