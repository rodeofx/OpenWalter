file(GLOB SRC "PathUtil.cpp")

add_library(walterCommon STATIC ${SRC})

target_include_directories(walterCommon
    PUBLIC
    ${Boost_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIRS})

target_include_directories(walterCommon
    PRIVATE
    ${MAYA_INCLUDE_DIRS})

target_link_libraries(
    walterCommon
    ${Boost_REGEX_LIBRARY}
    ${JSONCPP_LIBRARIES})
