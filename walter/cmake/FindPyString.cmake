
# The module defines the following variables:
#	PYSTRING_LIBRARY_DIR - path to PYstring static libs
#       PYSTRING_FOUND 	    - true if the Alembic was found
#
# Example usage:
#   find_package(PYSTRING)
#   if(PYSTRING_FOUND)
#     message("Pystring found: ${PYSTRING_LIBRARY_DIR}")
#   endif()
#
#=============================================================================

message("-- Searching PyString libraries.......")

IF(NOT PYSTRING_DIR AND NOT $ENV{PYSTRING_DIR} STREQUAL "")
  SET(PYSTRING_DIR $ENV{PYSTRING_DIR})
ENDIF()


set(LIBRARY_PATHS
	${PYSTRING_DIR}/lib/
	${PYSTRING_DIR}/lib/static)

# Find PYstring lib

find_library(PYSTRING_LIBRARY
	NAMES pystring libpystring
	PATHS ${LIBRARY_PATHS})

set ( PYSTRING_LIBRARIES
	${PYSTRING_LIBRARY}
	)

get_filename_component( PYSTRING_LIBRARY_DIR ${PYSTRING_LIBRARY} PATH )

find_path ( PYSTRING_INCLUDE_DIR pystring.h
           ${PYSTRING_DIR}/include/pystring
          )

include( FindPackageHandleStandardArgs )

find_package_handle_standard_args( "PYSTRING" DEFAULT_MSG
				PYSTRING_LIBRARIES
                PYSTRING_LIBRARY_DIR
				PYSTRING_INCLUDE_DIR
				  )
if(NOT PYSTRING_FOUND)
    message(FATAL_ERROR "Try using -D PYSTRING_DIR=/path/to/pystring")
endif()

