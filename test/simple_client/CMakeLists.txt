set(targets 
	recv_client
	send_client
	chat_client
	http_client
)

foreach(target ${targets})
	add_executable(${target} "${target}.c")
	set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/test/bin")
endforeach()


