# Copyright 2017 Rodeo FX.  All rights reserved.

set(PROC walterProcedural)
cmake_policy(SET CMP0074 NEW)

# The procedural.
file(GLOB SRC "*.cpp" "*h")

add_library(${PROC} SHARED ${SRC} )

set_target_properties(${PROC} PROPERTIES PREFIX "")

# USD requirements.
find_package(USD REQUIRED)
find_package(TBB REQUIRED)
find_package(Threads REQUIRED)

# Definitions
target_compile_definitions(
    ${PROC}
    PRIVATE
    BUILD_COMPONENT_SRC_PREFIX=\"\"
    BUILD_OPTLEVEL_OPT
    MFB_ALT_PACKAGE_NAME=${PROC}
    TF_NO_GNU_EXT)

# Includes
target_include_directories(
    ${PROC}
    PRIVATE
    ${ARNOLD_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${TBB_INCLUDE_DIRS}
    ${USD_INCLUDE_DIR})

# USD libraries.
set(USD_ALL_LIBS
    ${USD_LIBRARIES}
    walterUsdExtras)

# Put them as whole-archive.
target_link_libraries(
    ${PROC}
    PRIVATE
    -Wl,--whole-archive
    ${USD_ALL_LIBS}
    -Wl,--no-whole-archive)

# Arnold library.
target_link_libraries(
    ${PROC}
    PRIVATE
    ${ARNOLD_LIBRARY})

target_link_libraries(
    ${PROC}
    PRIVATE
    ${ALEMBIC_LIB}
    ${ALEMBIC_ILMBASE_HALF_LIB}
    ${ALEMBIC_ILMBASE_IEX_LIB}
    ${Boost_REGEX_LIBRARY}
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES}
    util
    walterCommon)

if(USE_HDF5)
    target_link_libraries(
        ${PROC}
        PRIVATE
        ${ALEMBIC_HDF5_LIB} )
endif()

macro(arnold_test assScene)
    add_test(
        NAME ${assScene}
        COMMAND
            ${ARNOLD_EXECUTABLE}
            -v 0
            -dp
            -dw
            -i ${assScene}
            -o /tmp/${assScene}.exr
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/arnold)
    add_test(
        NAME ${assScene}-Diff
        COMMAND
            ${OIIO_idiff_BINARY}
            -q
            -warn 0.01
            -fail 0.001
            ${assScene}.exr
            /tmp/${assScene}.exr
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/arnold)
    set_property(
        TEST ${assScene}
        PROPERTY ENVIRONMENT
            "ARNOLD_PLUGIN_PATH=${CMAKE_INSTALL_PREFIX}/arnold:${MTOA_INCLUDE_DIR}/../shaders")
endmacro()

if(BUILD_TESTS)
    find_package(OpenImageIO REQUIRED)
    arnold_test(00-displacement-from-maya.ass)
    arnold_test(01-displacement-from-maya.ass)
    arnold_test(02-point-instancer-from-maya.ass)
    arnold_test(03-motion-blur-from-maya.ass)
    arnold_test(04-attribute-inheritance.ass)
    arnold_test(05-arnold-visibility-bug.ass)
    arnold_test(06-uvs-subdivision.ass)
    arnold_test(07-arnold-visibilities.ass)
    arnold_test(08-triplanar-shader.ass)
    arnold_test(09-b365c58f35b1-broken-regex.ass)
    arnold_test(10-6722c1d-aiComposite.ass)
    arnold_test(11-matrix-param_support.ass)
    arnold_test(12-point-instancer.ass)
    arnold_test(13-point-instancer-colors.ass)
endif()

install(TARGETS ${PROC} DESTINATION ${CMAKE_INSTALL_PREFIX}/arnold)
