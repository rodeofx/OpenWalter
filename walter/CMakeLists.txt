project(walter)
cmake_minimum_required(VERSION 3.7)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Get the current version
include(version)

if( "${CMAKE_SYSTEM_NAME}" MATCHES "Linux" )
    add_definitions(-DLINUX -D_LINUX)
endif()

# We use the custom namespace for TBB because the standard namespace conflicts
# with Maya when static linking.
if (NOT TBB_NAMESPACE)
    add_definitions(-Dtbb=rdotbb)
else()
    add_definitions(-Dtbb=${TBB_NAMESPACE})
endif()

add_definitions(
    -DWALTER_MAJOR_VERSION="${WALTER_MAJOR_VERSION}"
    -DWALTER_MINOR_VERSION="${WALTER_MINOR_VERSION}"
    -DWALTER_PATCH_VERSION="${WALTER_PATCH_VERSION}")


set(CMAKE_SKIP_RPATH TRUE)
cmake_policy(SET CMP0012 OLD)

set(LINUX FALSE)
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
    set(LINUX TRUE)
endif()

option(USE_HDF5 "Include HDF5 stuff" OFF)
option(USE_STATIC_BOOST "Build with static Boost libs" ON)
option(USE_ILMBASE_STATIC_LIBS "Build with static ilmBase libs" ON)
option(USE_STATIC_HDF5 "Build with static HDF5 libs" OFF)
option(ALEMBIC_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_ARNOLD_PROCEDURALS "Build Arnold procedurals" ON)
option(BUILD_KATANA_PLUGINS "Build Katana plugins" ON)
option(BUILD_MAYA_PLUGINS "Build Maya plugins" ON)
option(BUILD_HOUDINI_PLUGINS "Build Houdini plugins" ON)
option(BUILD_TESTS "Enable test series" OFF)
option(BUILD_VIEWER "Build the Walter viewer app" OFF)

set(USD_RESOLVER_NAME 
    "AbcCoreLayerResolver"
    CACHE STRING 
    "Name of the Resolver plugin to use with Walter")

if (BUILD_TESTS)
    # Enable tests.
    enable_testing()
endif()

if (LINUX)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    if (COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    elseif (COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    endif()
endif()

# Compiler flags
if (LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

# Set some debug vs opt flags
if ("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    add_definitions(-DDEBUG=1 -UNDEBUG)
    if (LINUX)
        add_definitions(
            -Wall
            -Wextra
            -Wno-deprecated-register
            -Wno-ignored-qualifiers
            -Wno-reorder
            -Wno-unused-local-typedefs
            -Wno-unused-parameter)
    endif()
elseif ("${CMAKE_BUILD_TYPE}" MATCHES "Release")
    add_definitions(-DNDEBUG=1 -UDEBUG)
    if (LINUX)
        add_definitions(-O3 -Wno-deprecated-register)
    endif()
endif()

if (LINUX)
    set(EXTERNAL_MATH_LIBS "-lm")
    add_definitions(-DPLATFORM_LINUX -DPLATFORM=LINUX -D_LINUX)
endif()

# Find USD
find_package(USD REQUIRED)

# Find Boost
set(Boost_USE_STATIC_LIBS ${USE_STATIC_BOOST})
find_package(Boost REQUIRED COMPONENTS filesystem program_options regex system)

# Find ILMBASE
find_package(IlmBase REQUIRED)

# Find alembic
find_package(Alembic REQUIRED)

# Find HDF5
if(USE_HDF5)
    find_package(AlembicHDF5 REQUIRED)
endif()

# Find JSONCPP
find_package(JsonCPP REQUIRED)

# Find pthread (needed by gtest)
find_package(Threads REQUIRED)

# Testing
find_package(GoogleTest)
set(TEST_RESOURCES_DIR ${CMAKE_SOURCE_DIR}/tests/resources)

set(SUBDIRECTORIES
        alembic
        common
        tests/resources
)

if(BUILD_MAYA_PLUGINS)
    # Find maya
    find_package(Maya REQUIRED)
    find_package(Mtoa REQUIRED)
    list(APPEND SUBDIRECTORIES maya)
    list(APPEND SUBDIRECTORIES arnold)
endif()

if(BUILD_KATANA_PLUGINS)
    # Find Katana
    find_package(Katana PATHS ${KATANA_HOME}/plugin_apis/cmake)
    # Find Arnold SDK
    find_package(Arnold REQUIRED)

    if(TARGET Katana::FnAttribute)
        list(APPEND SUBDIRECTORIES usd/fileFormat/usdArnold)
        list(APPEND SUBDIRECTORIES katana/WalterIn)
    else()
        message(WARNING "Katana is not found. Please specify KATANA_HOME.")
    endif()
endif()

if(BUILD_ARNOLD_PROCEDURALS)
    # Find Arnold SDK
    find_package(Arnold REQUIRED)
    # Required because of integration tests using mtoa shaders
    find_package(Mtoa REQUIRED)
    list(APPEND SUBDIRECTORIES arnold)
endif()

if(BUILD_HOUDINI_PLUGINS)
    find_package(Houdini REQUIRED)
    list(APPEND SUBDIRECTORIES houdini)
endif()

list(APPEND SUBDIRECTORIES usd)
if (BUILD_TESTS)
    list(APPEND SUBDIRECTORIES usd/tests)
endif()

if(BUILD_VIEWER)
    list(APPEND SUBDIRECTORIES viewer)
endif()

include_directories(common)

# loop over subdirectories
foreach(SUBDIR ${SUBDIRECTORIES})
    add_subdirectory(${SUBDIR})
endforeach()
