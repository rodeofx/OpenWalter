set(WALTERSTANDIN walterStandin)

find_package(OpenGL REQUIRED)

file(GLOB SRC "*.cpp" "*h")

add_library(${WALTERSTANDIN} SHARED ${SRC})

# Includes
target_include_directories(
    ${WALTERSTANDIN}
    PRIVATE
    ${ALEMBIC_INCLUDE_PATH}
    ${ARNOLD_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${ILMBASE_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIRS})

find_package(USD REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread chrono date_time atomic)
find_package(JPEG REQUIRED)
find_package(OpenColorIO REQUIRED)
find_package(OpenEXR REQUIRED)
find_package(OpenImageIO REQUIRED)
find_package(OpenSubdiv REQUIRED)
find_package(PNG REQUIRED)
find_package(PTex REQUIRED)
find_package(TBB REQUIRED)
find_package(TIFF REQUIRED)
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)
find_package(Zlib REQUIRED)

# Standard FindGLEW is looking in the system. Fix it.
set(GLEW_INCLUDE_DIR ${GLEW_LOCATION}/include)
file(GLOB GLEW_LIBRARY "${GLEW_LOCATION}/lib/libGLEW*")
find_package(GLEW REQUIRED)

# Definitions
target_compile_definitions(
    ${WALTERSTANDIN}
    PRIVATE
    BUILD_COMPONENT_SRC_PREFIX=\"\"
    BUILD_OPTLEVEL_OPT
    MFB_ALT_PACKAGE_NAME=${WALTERSTANDIN}
    TF_NO_GNU_EXT
    WALTER_USD)

# Includes
target_include_directories(
    ${WALTERSTANDIN}
    PRIVATE
    ${GLEW_INCLUDE_DIR}
    ${ILMBASE_INCLUDE_DIR}/..
    ${OIIO_INCLUDE_DIR}
    ${TBB_INCLUDE_DIRS}
    ${USD_INCLUDE_DIR})

# USD libraries.
set(USD_ALL_LIBS
    ${USD_INCLUDE_DIR}/../lib/hdEmbree.a
    ${HYDRA_LIBRARIES}
    ${USD_LIBRARIES}
    walterUsdExtras)

# Put them as whole-archive.
target_link_libraries(
    ${WALTERSTANDIN}
    PRIVATE
    -Wl,--whole-archive
    ${USD_ALL_LIBS}
    -Wl,--no-whole-archive)

target_link_libraries(
    ${WALTERSTANDIN}
    PRIVATE
    ${USD_INCLUDE_DIR}/../../embree/lib/libembree.a
    ${USD_INCLUDE_DIR}/../../embree/lib/libembree_avx.a
    ${USD_INCLUDE_DIR}/../../embree/lib/libembree_avx2.a
    ${USD_INCLUDE_DIR}/../../embree/lib/libembree_sse42.a
    ${USD_INCLUDE_DIR}/../../embree/lib/libsys.a
    ${USD_INCLUDE_DIR}/../../embree/lib/libsimd.a
    ${USD_INCLUDE_DIR}/../../embree/lib/libtasking.a
    ${USD_INCLUDE_DIR}/../../embree/lib/liblexers.a
    ${OIIO_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${GLEW_LIBRARIES}
    ${JPEG_LIBRARY}
    ${OCIO_LIBRARIES}
    ${OPENEXR_LIBRARIES}
    ${OPENSUBDIV_LIBRARIES}
    ${PNG_LIBRARY}
    ${PTEX_LIBRARY}
    ${TBB_LIBRARIES}
    TIFF::TIFF
    ${Boost_ATOMIC_LIBRARY}
    ${Boost_CHRONO_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    ${X11_X11_LIB}
    ${X11_Xcursor_LIB}
    ${X11_Xext_LIB}
    ${X11_Xi_LIB}
    ${X11_Xinerama_LIB}
    ${X11_Xrandr_LIB}
    ${X11_Xxf86vm_LIB}
    ${ZLIB_LIB})


target_link_libraries(
    ${WALTERSTANDIN}
    PRIVATE
    ${ALEMBIC_LIB}
    ${ALEMBIC_ILMBASE_HALF_LIB}
    ${ALEMBIC_ILMBASE_IEX_LIB}
    ${MAYA_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
    AbcExport
    rt
    walterMayaCommon
    walterAbcExtras
    walterCommon)

if(USE_HDF5)
    target_link_libraries(
        ${WALTERSTANDIN}
        PRIVATE
        ${ALEMBIC_HDF5_LIB} )
endif()

set_target_properties(${WALTERSTANDIN} PROPERTIES PREFIX "")

MAYA_SET_PLUGIN_PROPERTIES(${WALTERSTANDIN})

add_definitions(
    -DMAJOR_VERSION="${MAJOR_VERSION}"
    -DMINOR_VERSION="${MINOR_VERSION}"
    -DPATCH_VERSION="${PATCH_VERSION}")

macro(maya_render_test mayaScene)
    add_test(
        NAME ${mayaScene}
        COMMAND
            ${MAYA_LOCATION}/bin/Render -r arnold -x 256 -y 256 -rd /tmp -im ${mayaScene} -of exr ${mayaScene}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/mayaRender)

    add_test(
        NAME ${mayaScene}-Diff
        COMMAND
            ${OIIO_idiff_BINARY}
            -q
            -warn 0.01
            -fail 0.001
            ${mayaScene}.exr
            /tmp/${mayaScene}.exr
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/mayaRender)
    set_property(
        TEST ${mayaScene}
        PROPERTY ENVIRONMENT
            "MAYA_MODULE_PATH=${CMAKE_INSTALL_PREFIX}/maya:${MTOA_INCLUDE_DIR}/..:${MAYA_MODULE_PATH};"
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/maya:${MAYA_PLUG_IN_PATH};"
            "MAYA_RENDER_DESC_PATH=${MTOA_INCLUDE_DIR}/..:${MAYA_RENDER_DESC_PATH}/..;"
            "LD_LIBRARY_PATH=${MTOA_INCLUDE_DIR}/../bin")

endmacro()

if(BUILD_TESTS)
    add_test(
        NAME Walter-Standin-Integration
        COMMAND
            ${MAYA_EXECUTABLE}
            -batch
            -command "source \"${CMAKE_SOURCE_DIR}/tests/maya/main.mel\"")
    set_property(
        TEST Walter-Standin-Integration
        PROPERTY ENVIRONMENT
            "MAYA_MODULE_PATH=${CMAKE_INSTALL_PREFIX}/maya:${MTOA_INCLUDE_DIR}/..;"
            "LD_LIBRARY_PATH=${MTOA_INCLUDE_DIR}/../bin")

    find_package(OpenImageIO REQUIRED)
    maya_render_test(testPointInstancer.ma)
    maya_render_test(testPrimColor.ma)
    maya_render_test(testPrimVertexColor.ma)

endif()

install(TARGETS ${WALTERSTANDIN} DESTINATION ${CMAKE_INSTALL_PREFIX}/maya/plug-ins)
