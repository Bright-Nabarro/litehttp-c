file(GLOB src "*.c")

add_library(http STATIC ${src})
target_include_directories(http PUBLIC "./include")
target_link_libraries(http util)
