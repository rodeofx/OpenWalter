find_package(Git REQUIRED)

# Get the current git commit
execute_process(
    COMMAND
    ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE CURRENT_COMMIT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Get the latest git commit that contains a tag
execute_process(
    COMMAND
    ${GIT_EXECUTABLE} rev-list --tags --max-count=1
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE LATEST_TAG_COMMIT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if(LATEST_TAG_COMMIT)
    # Get the latest git tag
    execute_process(
        COMMAND
        ${GIT_EXECUTABLE} describe --tags ${LATEST_TAG_COMMIT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE WALTER_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(WALTER_VERSION "0.0.0")
    message(WARNING "No git tag found, using version ${WALTER_VERSION}")
endif()

# Split it with '.' symbol
string(REPLACE "-" ";" VERSION_LIST ${WALTER_VERSION})
string(REPLACE "." ";" VERSION_LIST ${VERSION_LIST})

# Get the version list
list(GET VERSION_LIST 0 WALTER_MAJOR_VERSION)
list(GET VERSION_LIST 1 WALTER_MINOR_VERSION)
list(GET VERSION_LIST 2 WALTER_PATCH_VERSION)

# Increase the patch version if current commit is not tagged.
if( NOT "${CURRENT_COMMIT}" MATCHES "${LATEST_TAG_COMMIT}" )
    math(EXPR WALTER_PATCH_VERSION "${WALTER_PATCH_VERSION}+1")
endif()

message( "Walter version is ${WALTER_MAJOR_VERSION}.${WALTER_MINOR_VERSION}.${WALTER_PATCH_VERSION}" )


