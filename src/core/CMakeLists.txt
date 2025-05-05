# 通过pkgconfig 导入 toml-c
pkg_check_modules(TOML REQUIRED IMPORTED_TARGET libtoml)
message(STATUS "find package libtoml(https://github.com/arp242/toml-c)")


add_core(core)
