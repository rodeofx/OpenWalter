
### Resourcer app
add_executable(resourcer resourcer.c)

### Convert all the GLSL shaders and usda schemas to cpp
file(
    GLOB_RECURSE
    RESOURCE_LIST
        "${USD_ROOT}/share/*.glslfx"
        "${USD_ROOT}/share/*.usda"
        "schemas/*.usda")
set(RESOURCE_CPP_LIST "")

FOREACH(RESOURCE ${RESOURCE_LIST})
    string(REPLACE "/" "_" RESOURCE_CPP ${RESOURCE})
    string(CONCAT RESOURCE_CPP ${RESOURCE_CPP} ".cpp")
    add_custom_command(
        OUTPUT ${RESOURCE_CPP}
        COMMAND resourcer ${RESOURCE} -o ${RESOURCE_CPP})
    add_custom_target(
        ${RESOURCE_CPP}
        ALL
        DEPENDS resourcer ${RESOURCE_CPP})

    list(APPEND RESOURCE_CPP_LIST ${RESOURCE_CPP})
ENDFOREACH()

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/walter_plugInfo.json.in"
  "${CMAKE_CURRENT_BINARY_DIR}/walter_plugInfo.json")

# Merge every plugInfo.json from USD into one single one
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/plugInfoMerger.py
    ARGS
        ${USD_ROOT}
        ${CMAKE_CURRENT_BINARY_DIR}/walter_plugInfo.json
        ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/walter_plugInfo.json
    COMMENT "Merging plugInfo.json files from ${USD_ROOT}")

# Include plugInfo.json to the library as an ELF object.
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.o
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/usd
    COMMAND ${CMAKE_OBJCOPY}
    ARGS
        --input binary
        --output elf64-x86-64
        --binary-architecture i386
        plugInfo.json
        ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.o
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    COMMENT "Objcopying plugInfo.json")

# Include plugInfo.json to the library as an ELF object.
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.o
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/usd
    COMMAND ${CMAKE_OBJCOPY}
    ARGS
        --input binary
        --output elf64-x86-64
        --binary-architecture i386
        plugInfo.json
        ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.o
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    COMMENT "Objcopying plugInfo.json")

add_custom_target(
    plugInfo
    ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.o)

### walterUsdExtras library
file(GLOB ALL_SRC 
    "*.cpp" "*h"
    "resolver/*.cpp" "resolver/*.h"
    "schemas/*.cpp" "schemas/*.h")

file(GLOB WALTER_HYDRA_SCHEMAS
    "schemas/walterHdStream/*.cpp" "schemas/walterHdStream/*.h")

if(BUILD_MAYA_PLUGINS)
    set(ALL_SRC 
        ${ALL_SRC}
        ${WALTER_HYDRA_SCHEMAS})
endif()

if("${USD_RESOLVER_NAME}" MATCHES RdoResolver)
    set(SRC "")
    FOREACH(src ${ALL_SRC})
        string(REGEX MATCH "abcCoreLayerResolver" result ${src} )
        if(result STREQUAL "abcCoreLayerResolver")
            message("RDO_RESOLVER_ROOT is defined, skipping default Walter resolver")
        else()
            list(APPEND SRC ${src})
        endif()
    ENDFOREACH()
else()
    set(SRC ${ALL_SRC})
endif()

add_library(walterUsdExtras STATIC ${SRC} ${RESOURCE_CPP_LIST})

# Resources.
target_link_libraries(
    walterUsdExtras
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.o)

# USD requirements.
find_package(TBB REQUIRED)

if("${USD_RESOLVER_NAME}" MATCHES RdoResolver)
    find_package(RdoResolver REQUIRED)
endif()

# Definitions
target_compile_definitions(
    walterUsdExtras
    PRIVATE
    BUILD_COMPONENT_SRC_PREFIX=\"\"
    BUILD_OPTLEVEL_OPT
    MFB_ALT_PACKAGE_NAME=walterUsdExtras
    TF_NO_GNU_EXT)

target_include_directories(walterUsdExtras
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIR})

target_include_directories(walterUsdExtras
    PRIVATE
    ${USD_INCLUDE_DIR}
    ${TBB_INCLUDE_DIRS}
    ${RDO_USD_RESOLVER_INCLUDE_DIR})

target_link_libraries(
    walterUsdExtras
    PUBLIC
    "-Wl,--whole-archive ${RDO_USD_RESOLVER_LIBRARY} -Wl,--no-whole-archive"
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY})

# install(TARGETS walterUsdExtras DESTINATION "/tmp/test")