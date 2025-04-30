set(targets
	string_t
	string_view_t
)

set(libs
	util
)

add_test_from_targets("${targets}" "${libs}")
