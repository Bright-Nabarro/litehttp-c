cmake_minimum_required(VERSION 3.21)

project(c_http_server C)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DEBUG_MODE ON)
option(ENABLE_TEST OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(USR_PREFIX "/usr/local")

find_package(PkgConfig REQUIRED)
set(USER_LOCAL_PKG_PATH "${USR_PREFIX}/lib/pkgconfig")
set(CURRENT_PKG_PATH "$ENV{PKG_CONFIG_PATH}")
if (NOT "${CURRENT_PKG_PATH}" MATCHES ${USER_LOCAL_PKG_PATH})
	set(ENV{PKG_CONFIG_PATH} ${USER_LOCAL_PKG_PATH}:${CURRENT_PKG_PATH})
endif()
message(STATUS "pkgconfig env path $ENV{PKG_CONFIG_PATH}")

include(FetchContent)

if (DEBUG_MODE)
	set(CMAKE_BUILD_TYPE Debug)
else()
	set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(
	-Wall
	-Wextra
	-Wuninitialized
)

add_subdirectory(src)

if (ENABLE_TEST)
	enable_testing()
	add_subdirectory(test)
endif()

