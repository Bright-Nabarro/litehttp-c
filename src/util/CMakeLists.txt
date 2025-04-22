file(GLOB src "*.c")

# 导入 c_thread_pool
FetchContent_Declare(
	ctp
	GIT_REPOSITORY https://github.com/Bright-Nabarro/c_thread_pool.git
	GIT_TAG main
)
FetchContent_MakeAvailable(ctp)
message(STATUS "find package ctp(https://github.com/Bright-Nabarro/c_thread_pool.git)")

add_library(util STATIC ${src})
target_include_directories(util PUBLIC "./include")
target_link_libraries(util ctp)

