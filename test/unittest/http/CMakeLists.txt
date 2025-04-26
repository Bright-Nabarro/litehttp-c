set(prefix_path ${CMAKE_SOURCE_DIR}/src/http)

# 单独测试http_parser的依赖变量
set(dep_src 
	${prefix_path}/http_response.c
	${prefix_path}/http_types.c
)
# add http_parser
message(STATUS "add static unit test http_parser to ${dep_src}")
add_executable(http_parser_test "http_parser_test.c" ${dep_src})
target_link_libraries(http_parser_test test_header util)
target_include_directories(http_parser_test PRIVATE
	"${prefix_path}"
	"${prefix_path}/include"
)
set_target_properties(http_parser_test
	PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${unit_outdir}
)
add_test(http_parser_test "${unit_outdir}/http_parser_test")
# http_parser end

set(res 
	"${CMAKE_SOURCE_DIR}/res/index.html")

message(STATUS "add http test resource_getter to ${unit_outdir}")
add_executable(resource_getter "resource_getter_test.c")
target_link_libraries(resource_getter test_header util http)
set_target_properties(resource_getter 
	PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${unit_outdir}
)
add_test(NAME resource_getter COMMAND "${unit_outdir}/resource_getter" ${res})
