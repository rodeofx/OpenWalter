set(WALTERIN walterIn)

file(GLOB SRC "*.cpp" "*h")

add_library(${WALTERIN} SHARED ${SRC})

set_target_properties(${WALTERIN} PROPERTIES PREFIX "")

# USD requirements.
find_package(USD REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread chrono date_time atomic)
find_package(JPEG REQUIRED)
find_package(OpenColorIO REQUIRED)
find_package(OpenEXR REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenImageIO REQUIRED)
find_package(OpenSubdiv REQUIRED)
find_package(PNG REQUIRED)
find_package(PTex REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(TBB REQUIRED)
find_package(TIFF REQUIRED)
find_package(Threads REQUIRED)
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)
find_package(Zlib REQUIRED)
find_package(OpenVDB REQUIRED)
find_package(Blosc REQUIRED)
find_package(Boost REQUIRED COMPONENTS iostreams)

# Standard FindGLEW is looking in the system. Fix it.
set(GLEW_INCLUDE_DIR ${GLEW_LOCATION}/include)
file(GLOB GLEW_LIBRARY "${GLEW_LOCATION}/lib/libGLEW*")
find_package(GLEW REQUIRED)

# Definitions
target_compile_definitions(
    ${WALTERIN}
    PRIVATE
    BUILD_COMPONENT_SRC_PREFIX=\"\"
    BUILD_OPTLEVEL_OPT
    MFB_ALT_PACKAGE_NAME=${WALTERIN}
    TF_NO_GNU_EXT)

# OpenVDB definitions
target_compile_definitions(
    ${WALTERIN}
    PRIVATE
    USE_OPENVDB)

# OpenVDB includes
target_include_directories(
    ${WALTERIN}
    PRIVATE
    ${OpenVDB_INCLUDE_DIR})

# OpenVDB libraries
target_link_libraries(
    ${WALTERIN}
    PRIVATE
    ${OpenVDB_OPENVDB_LIBRARY}
    ${BLOSC_blosc_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY})

# Includes
target_include_directories(${WALTERIN}
    PRIVATE
    ${ALEMBIC_INCLUDE_PATH}
    ${Boost_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${HDF5_INCLUDE_DIRS}
    ${ILMBASE_INCLUDE_DIR}
    "${ILMBASE_INCLUDE_DIR}/.." # Fix for RND-575.
    # The problem is that OPENEXR_INCLUDE_DIR points to
    # 'WALTER_LIB_DIR/ilmbase/include/OpenEXR' and OpenVDB
    # needs to point to 'WALTER_LIB_DIR/ilmbase/include'
    # (in WALTER_LIB_DIR/openvdb/include/openvdb/Types.h) 
    ${OPENEXR_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${TBB_INCLUDE_DIRS}
    ${USD_INCLUDE_DIR}
    ${USD_INCLUDE_DIR}/../third_party/katana/include)

# USD libraries.
set(USD_ALL_LIBS
    ${USD_LIBRARIES}
    ${HYDRA_LIBRARIES})

# Put them as whole-archive.
target_link_libraries(
    ${WALTERIN}
    PRIVATE
    ${USD_INCLUDE_DIR}/../third_party/katana/lib/libusdKatana.a
    -Wl,--whole-archive
    usdArnold
    walterUsdExtras
    ${USD_ALL_LIBS}
    -Wl,--no-whole-archive)

# Katana libraries
target_link_libraries(
    ${WALTERIN}
    PRIVATE
    Katana::FnAttribute
    Katana::FnGeolibOpPlugin
    Katana::FnGeolibServices
    Katana::FnLogging)

target_link_libraries(
    ${WALTERIN}
    PRIVATE
    walterAbcExtras
    ${OIIO_LIBRARIES}
    ${ALEMBIC_LIB}
    ${ALEMBIC_HDF5_LIB}
    ${ALEMBIC_ILMBASE_HALF_LIB}
    ${ALEMBIC_ILMBASE_IEX_LIB}
    ${Boost_ATOMIC_LIBRARY}
    ${Boost_CHRONO_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PYTHON_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    ${GLEW_LIBRARIES}
    ${JPEG_LIBRARY}
    ${OCIO_LIBRARIES}
    ${OPENEXR_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    ${OPENSUBDIV_LIBRARIES}
    ${PNG_LIBRARY}
    ${PTEX_LIBRARY}
    ${PYTHON_LIBRARIES}
    ${TBB_LIBRARIES}
    ${X11_X11_LIB}
    ${X11_Xcursor_LIB}
    ${X11_Xext_LIB}
    ${X11_Xi_LIB}
    ${X11_Xinerama_LIB}
    ${X11_Xrandr_LIB}
    ${X11_Xxf86vm_LIB}
    ${ZLIB_LIB}
    TIFF::TIFF
    rt
    util
    walterCommon)

macro(katana_test katanaScene)
    add_test(
        NAME ${katanaScene}
        COMMAND
            ${KATANA_PLUGINAPIS_DIR}/../katana
            --script=${CMAKE_SOURCE_DIR}/tests/katana/katana-test-render.py
            ${katanaScene}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/katana)
    add_test(
        NAME ${katanaScene}-Diff
        COMMAND
            ${OIIO_idiff_BINARY}
            -q
            -warn 0.01
            -fail 0.001
            ${katanaScene}.exr
            /tmp/${katanaScene}.exr
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/katana)
    set_property(
        TEST ${katanaScene}
        PROPERTY ENVIRONMENT
            "KATANA_RESOURCES=${CMAKE_INSTALL_PREFIX}/katana:${KTOA_HOME};"
            "ARNOLD_PLUGIN_PATH=${CMAKE_INSTALL_PREFIX}/arnold:${KTOA_HOME}/shaders:${MTOA_BASE_DIR}/shaders")
endmacro()

if(BUILD_TESTS)
    find_package(OpenImageIO REQUIRED)
    katana_test(testAlembicAttributes.katana)
    katana_test(testInstances.katana)
    katana_test(testLightList.katana)
    katana_test(testVariant.katana)
    katana_test(testWalterAssign.katana)
    katana_test(testAlembicPointCloudMotionBlur.katana)
    katana_test(testInstancesAssignation.katana)
    katana_test(testPointInstancer.katana)
    katana_test(testUSDLights.katana)
    katana_test(testUSDPurposes.katana)
endif()

set(KATANA_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/katana)
install(DIRECTORY ../../common/walterWidgets DESTINATION ${KATANA_INSTALL_DIR}/Plugins)
install(FILES "Walter_In.py" DESTINATION ${KATANA_INSTALL_DIR}/Plugins)
install(FILES "Walter_Assign.py" DESTINATION ${KATANA_INSTALL_DIR}/Plugins)
install(FILES "walterLayersWidget.py" DESTINATION ${KATANA_INSTALL_DIR}/Plugins)
install(FILES "walterUSDExtern.py" DESTINATION ${KATANA_INSTALL_DIR}/Plugins)
install(FILES "walterVariantsMenu.py" DESTINATION ${KATANA_INSTALL_DIR}/Plugins)
install(TARGETS ${WALTERIN} DESTINATION katana/Ops)
