
IF(NOT MTOA_BASE_DIR AND NOT $ENV{MTOA_BASE_DIR} STREQUAL "")
  SET(MTOA_BASE_DIR $ENV{MTOA_BASE_DIR})
ENDIF()

set(MTOA_BASE_DIR
"${MTOA_BASE_DIR}"
CACHE
PATH
"Directory to search for MTOA")
 

find_library(MTOA_LIBRARY
	NAMES
	mtoa_api
	PATHS
	"${MTOA_BASE_DIR}/lib"
	"${MTOA_BASE_DIR}/bin"
	PATH_SUFFIXES
	${_pathsuffixes})


find_path(MTOA_INCLUDE_DIR
	NAMES
	utils/Version.h
	PATHS
	"${MTOA_BASE_DIR}"
	PATH_SUFFIXES
	include)
	
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MTOACPP
	DEFAULT_MSG
	MTOA_LIBRARY
	MTOA_INCLUDE_DIR)	
	

if(MTOA_FOUND)
	set(MTOA_LIBRARY "${MTOA_LIBRARY}")
	set(MTOA_INCLUDE_DIR "${MTOA_INCLUDE_DIR}")
	mark_as_advanced(MTOA_BASE_DIR)
endif()

mark_as_advanced(MTOA_INCLUDE_DIR MTOA_LIBRARY)

