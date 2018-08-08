# Example usage:
#   find_package(GoogleTest)
#   if(GoogleTest_FOUND)
#     message("GTest found: ${GoogleTest_LIBRARIES}")
#   endif()
#
#=============================================================================

set(GoogleTest_LIBRARIES
       ${GoogleTest_DIR}/lib/libgtest.a
       ${GoogleTest_DIR}/lib/libgtest_main.a
)

set(GoogleTest_INCLUDE_DIR ${GoogleTest_DIR}/include)


include( FindPackageHandleStandardArgs )

find_package_handle_standard_args( "GoogleTest" DEFAULT_MSG
				GoogleTest_LIBRARIES
				GoogleTest_INCLUDE_DIR
				  )
if(NOT GoogleTest_FOUND)
    message(FATAL_ERROR "Try using -D GoogleTest_DIR=/path/to/GTest")
endif()
