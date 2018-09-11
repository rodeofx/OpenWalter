set(MAYAPLUGIN walterMtoaConnection)

file(GLOB SRC "*.cpp" "*h")

add_library(${MAYAPLUGIN} SHARED ${SRC})

MAYA_SET_PLUGIN_PROPERTIES(${MAYAPLUGIN})

include_directories(${Boost_INCLUDE_DIR})

include_directories(${MTOA_INCLUDE_DIR})
include_directories(${ARNOLD_INCLUDE_DIR})
include_directories(${ALEMBIC_INCLUDE_PATH})
include_directories(${ILMBASE_INCLUDE_DIR})

target_link_libraries(
    ${MAYAPLUGIN}
    walterCommon
    walterMayaCommon
    walterMtoATranslator
    ${MTOA_LIBRARY}
    ${ALEMBIC_LIB}
    ${ALEMBIC_ILMBASE_HALF_LIB}
    ${ALEMBIC_ILMBASE_IEX_LIB}
    ${Boost_REGEX_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_CHRONO_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${MAYA_LIBRARIES}
    ${ARNOLD_LIBRARY}
    ${CMAKE_DL_LIBS})

if(USE_HDF5)
    target_link_libraries(
        ${MAYAPLUGIN}
        ${ALEMBIC_HDF5_LIB} )
endif()

set_target_properties(${MAYAPLUGIN} PROPERTIES PREFIX "")

install(TARGETS ${MAYAPLUGIN} DESTINATION ${CMAKE_INSTALL_PREFIX}/maya/plug-ins)


