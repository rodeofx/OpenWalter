
set(ARNOLD_BASE_DIR
"${ARNOLD_BASE_DIR}"
CACHE
PATH
"Directory to search for Arnold")


find_library(ARNOLD_LIBRARY
	NAMES
	ai
	PATHS
	"${ARNOLD_BASE_DIR}/lib"
	"${ARNOLD_BASE_DIR}/bin"
	PATH_SUFFIXES
	${_pathsuffixes})


find_path(ARNOLD_INCLUDE_DIR
	NAMES
	ai.h
	PATHS
	"${ARNOLD_BASE_DIR}"
	PATH_SUFFIXES
	include)

find_program(ARNOLD_EXECUTABLE
	NAMES
	kick
	PATHS
	"${ARNOLD_BASE_DIR}/bin")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	"ARNOLDCPP"
	DEFAULT_MSG
	ARNOLD_LIBRARY
	ARNOLD_INCLUDE_DIR)


if(ARNOLD_FOUND)
	set(ARNOLD_LIBRARY "${ARNOLD_LIBRARY}")
	set(ARNOLD_INCLUDE_DIR "${ARNOLD_INCLUDE_DIR}")
	set(ARNOLD_EXECUTABLE "${ARNOLD_EXECUTABLE}")
	mark_as_advanced(ARNOLD_BASE_DIR)
endif()

mark_as_advanced(ARNOLD_INCLUDE_DIR ARNOLD_LIBRARY ARNOLD_EXECUTABLE)
