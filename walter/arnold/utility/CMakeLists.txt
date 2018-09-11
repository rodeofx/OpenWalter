file(GLOB SRC "*.cpp")

add_library(arnoldVersionPort STATIC ${SRC})

find_package(Arnold REQUIRED)

message(STATUS ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>")
message(STATUS ${ARNOLD_INCLUDE_DIR})
message(STATUS ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>")

target_include_directories(arnoldVersionPort
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ARNOLD_INCLUDE_DIR})

target_link_libraries(
    arnoldVersionPort
    PUBLIC
    ${ARNOLD_LIBRARY})
