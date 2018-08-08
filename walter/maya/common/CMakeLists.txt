file(GLOB SRC "mayaUtils.cpp")

add_library(walterMayaCommon STATIC ${SRC})

target_include_directories(walterMayaCommon
    PUBLIC
    ${Boost_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
    ${MAYA_INCLUDE_DIRS})

target_link_libraries(
    walterMayaCommon
    ${Boost_REGEX_LIBRARY})


# configure maya mod file with version numbers
configure_file (
"${CMAKE_CURRENT_SOURCE_DIR}/walter.mod.in"
"${CMAKE_BINARY_DIR}/maya/walter.mod")

# Install files
install(
    FILES "${CMAKE_BINARY_DIR}/maya/walter.mod"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/maya)

# add scripts & icons
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../icons DESTINATION ${CMAKE_INSTALL_PREFIX}/maya)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../scripts DESTINATION ${CMAKE_INSTALL_PREFIX}/maya)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../common/walterWidgets DESTINATION ${CMAKE_INSTALL_PREFIX}/maya/scripts)

