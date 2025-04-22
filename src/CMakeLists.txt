add_subdirectory(util)
add_subdirectory(http)
add_subdirectory(core)

add_executable(app main.c)
set_target_properties(app PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
target_link_libraries(app core)

