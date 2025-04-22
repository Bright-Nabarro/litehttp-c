# 通过pkgconfig 导入 toml-c
pkg_check_modules(TOML REQUIRED IMPORTED_TARGET libtoml)
message(STATUS "find package libtoml(https://github.com/arp242/toml-c)")


file(GLOB src "*.c")

add_library(core STATIC ${src})
target_include_directories(core PUBLIC "./include")
target_link_libraries(core http toml)

