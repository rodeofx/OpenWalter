set(WALTER_VIEWER walterViewer)

# find_package(PkgConfig REQUIRED PATHS "/softwareLocal/walter/lib/glfw/lib/pkgconfig/")
# pkg_search_module(GLFW REQUIRED glfw3)

find_package(GLFW REQUIRED)

file(GLOB SRC "*.cpp" "*h")

# USD requirements.
find_package(OpenGL REQUIRED)
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

add_executable(${WALTER_VIEWER} ${SRC})

# Standard FindGLEW is looking in the system. Fix it.
set(GLEW_INCLUDE_DIR ${GLEW_LOCATION}/include)
file(GLOB GLEW_LIBRARY "${GLEW_LOCATION}/lib/libGLEW*")
find_package(GLEW REQUIRED)

# Definitions
target_compile_definitions(
    ${WALTER_VIEWER}
    PRIVATE
    BUILD_COMPONENT_SRC_PREFIX=\"\"
    BUILD_OPTLEVEL_OPT
    MFB_ALT_PACKAGE_NAME=${WALTER_VIEWER}
    TF_NO_GNU_EXT
    WALTER_USD)

# Includes
target_include_directories(
    ${WALTER_VIEWER}
    PRIVATE
    ${GoogleTest_INCLUDE_DIR}
    ${ALEMBIC_INCLUDE_PATH}
    ${Boost_INCLUDE_DIR}
    ${ILMBASE_INCLUDE_DIR}
    ${ILMBASE_INCLUDE_DIR}/..
    ${GLEW_INCLUDE_DIR}
    ${OIIO_INCLUDE_DIR}
    ${TBB_INCLUDE_DIRS}
    ${USD_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/deps)

# USD libraries.
set(USD_ALL_LIBS
    ${USD_INCLUDE_DIR}/../lib/hdEmbree.a
    ${HYDRA_LIBRARIES}
    ${USD_LIBRARIES}
    walterUsdExtras)

# Put them as whole-archive.
target_link_libraries(
    ${WALTER_VIEWER}
    PRIVATE
    -Wl,--whole-archive
    ${USD_ALL_LIBS}
    -Wl,--no-whole-archive)

target_link_libraries(
    ${WALTER_VIEWER}
    PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    ${GoogleTest_LIBRARIES}    
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
    ${ZLIB_LIB}
    ${GLFW_STATIC_LIBRARIES})

target_link_libraries(
    ${WALTER_VIEWER}
    PRIVATE
    ${ALEMBIC_LIB}
    ${ALEMBIC_ILMBASE_HALF_LIB}
    ${ALEMBIC_ILMBASE_IEX_LIB}
    ${OPENGL_gl_LIBRARY}
    rt
    walterAbcExtras
    walterCommon)

if(USE_HDF5)
    target_link_libraries(
        ${WALTER_VIEWER}
        PRIVATE
        ${ALEMBIC_HDF5_LIB} )
endif()

install(TARGETS ${WALTER_VIEWER} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
