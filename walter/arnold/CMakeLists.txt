if(BUILD_ARNOLD_PROCEDURALS)
	add_subdirectory(procedural)
endif()

add_subdirectory(utility)

message (STATUS ${ARNOLD_INCLUDE_DIR})