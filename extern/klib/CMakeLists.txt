file(GLOB src "*.c")

add_library(klib STATIC ${src})
target_include_directories(klib INTERFACE "include" PRIVATE "include/klib")
