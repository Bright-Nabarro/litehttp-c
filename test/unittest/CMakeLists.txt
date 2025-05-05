add_library(test_header INTERFACE)
target_include_directories(test_header INTERFACE "include")
set(unit_outdir "${CMAKE_SOURCE_DIR}/test/unittest/bin")

add_util(ut_util)
add_http(ut_http)
add_core(ut_core)
set(ut_libs ut_util ut_http ut_core)

foreach(ut_lib IN LISTS ut_libs)
	message(STATUS "ad UNIT_TEST macro to ${ut_lib}")
	target_compile_definitions(${ut_lib} PRIVATE UNIT_TEST)
endforeach()

function(add_test_from_targets targets libs)
    foreach(target IN LISTS targets)
        message(STATUS "add unit test ${target} to ${unit_outdir} link ${libs}")
        add_executable(${target} "${target}_test.c")
        target_link_libraries(${target} ${libs} test_header)
        set_target_properties(${target}
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${unit_outdir}"
        )
        add_test(${target} "${unit_outdir}/${target}")
		target_compile_definitions(${target} PUBLIC UNIT_TEST)
    endforeach()
endfunction()


add_subdirectory(util)
add_subdirectory(http)
