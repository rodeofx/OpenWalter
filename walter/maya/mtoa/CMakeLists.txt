set(MTOATRANSLATOR walterMtoATranslator)

file(GLOB SRC "*.cpp" "*h")
include_directories(${Boost_INCLUDE_DIR})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${MTOA_INCLUDE_DIR})
include_directories(${ARNOLD_INCLUDE_DIR})

add_library(${MTOATRANSLATOR} SHARED ${SRC})

MAYA_SET_TRANSLATOR_PROPERTIES(${MTOATRANSLATOR})

target_include_directories(${MTOATRANSLATOR}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
    ${MTOATRANSLATOR}
    walterCommon
    walterMayaCommon
    arnoldVersionPort
    ${ARNOLD_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    ${JSONCPP_LIBRARIES}
    ${MAYA_LIBRARIES}
    ${MTOA_LIBRARY} )
set_target_properties(${MTOATRANSLATOR} PROPERTIES PREFIX "")

install(TARGETS ${MTOATRANSLATOR} DESTINATION ${CMAKE_INSTALL_PREFIX}/maya/extensions)
