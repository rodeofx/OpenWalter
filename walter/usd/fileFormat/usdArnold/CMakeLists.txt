file(GLOB ALL_SRC "*.cpp" "*.h")

add_library(usdArnold STATIC ${ALL_SRC})

find_package(USD REQUIRED)
find_package(TBB REQUIRED)

# Definitions
target_compile_definitions(
    usdArnold
    PRIVATE
    BUILD_COMPONENT_SRC_PREFIX=\"\"
    BUILD_OPTLEVEL_OPT
    MFB_ALT_PACKAGE_NAME=usdArnold
    TF_NO_GNU_EXT)

target_include_directories(
    usdArnold
    PRIVATE
    ${ARNOLD_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${TBB_INCLUDE_DIRS}
    ${USD_INCLUDE_DIR})

target_link_libraries(
    usdArnold
    PRIVATE
    walterUsdExtras)
