set(targets
	resource_getter
)

set(libs
	util
	http
)

add_test_from_targets("${targets}" "${libs}")
