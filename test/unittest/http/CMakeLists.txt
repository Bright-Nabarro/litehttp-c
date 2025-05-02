set(targets
	resource_getter
	http_parse_dfs
)

set(libs
	util
	http
)

add_test_from_targets("${targets}" "${libs}")
