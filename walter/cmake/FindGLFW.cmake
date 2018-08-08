
set(GLFW_STATIC_LIBRARIES ${GLFW_DIR}/lib/libglfw3.a)
set(GLFW_INCLUDE_DIR ${GLFW_DIR}/include)

include( FindPackageHandleStandardArgs )

find_package_handle_standard_args( "GLFW" DEFAULT_MSG
				GLFW_STATIC_LIBRARIES
				GLFW_INCLUDE_DIR
				  )
if(NOT GLFW_FOUND)
    message(FATAL_ERROR "Try using -D GLFW_DIR=/path/to/glfw")
endif()
