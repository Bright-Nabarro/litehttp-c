add_library(test_header INTERFACE)
target_include_directories(test_header INTERFACE "include")
set(unit_outdir "${CMAKE_SOURCE_DIR}/test/unittest/bin")

function(add_test_from_targets targets libs)
    foreach(target IN LISTS targets)
        message(STATUS "add unit test ${target} to ${unit_outdir} link ${libs}")
        add_executable(${target} "${target}_test.c")
        target_link_libraries(${target} ${libs} test_header)
        set_target_properties(${target}
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${unit_outdir}"
        )
        add_test(${target} "${unit_outdir}/${target}")
    endforeach()
endfunction()


add_subdirectory(util)
add_subdirectory(http)
