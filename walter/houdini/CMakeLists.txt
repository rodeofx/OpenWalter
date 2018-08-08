set(OPWALTER OP_walter)

find_package(Boost REQUIRED COMPONENTS thread date_time chrono filesystem regex system)
find_package(JPEG REQUIRED)
find_package(OpenColorIO REQUIRED)
find_package(OpenEXR REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenImageIO REQUIRED)
find_package(OpenSubdiv REQUIRED)
find_package(PNG REQUIRED)
find_package(PTex REQUIRED)
find_package(TBB REQUIRED)
find_package(TIFF REQUIRED)
find_package(Threads REQUIRED)
find_package(USD REQUIRED)
find_package(X11 REQUIRED)
find_package(Zlib REQUIRED)

# Standard FindGLEW is looking in the system. Fix it.
set(GLEW_INCLUDE_DIR ${GLEW_LOCATION}/include)
file(GLOB GLEW_LIBRARY "${GLEW_LOCATION}/lib/libGLEW*")
find_package(GLEW REQUIRED)

file(GLOB SRC "src/*.cpp" "src/*h")

add_library(${OPWALTER} SHARED ${SRC})

set_target_properties(${OPWALTER} PROPERTIES PREFIX "")

# Houdini definitions
target_compile_definitions(
    ${OPWALTER}
    PRIVATE
    AMD64
    ENABLE_THREADS
    HOUDINI_API_VERSION=${HOUDINI_MAJOR_VERSION}${HOUDINI_MINOR_VERSION}
    HOUDINI_DOT_VERSION=${HOUDINI_BUILD_VERSION}
    MAKING_DSO
    SESI_LITTLE_ENDIAN
    SIZEOF_VOID_P=8
    USE_PTHREADS
    VERSION="${HOUDINI_VERSION}"
    _FILE_OFFSET_BITS=64)

# USD definitions
target_compile_definitions(
    ${OPWALTER}
    PRIVATE
    BUILD_COMPONENT_SRC_PREFIX=\"\"
    BUILD_OPTLEVEL_OPT
    MFB_ALT_PACKAGE_NAME=${OPWALTER}
    TF_NO_GNU_EXT)

# Includes
target_include_directories(
    ${OPWALTER}
    PRIVATE
    src
    ${ALEMBIC_INCLUDE_PATH}
    ${Boost_INCLUDE_DIR}
    ${HDF5_INCLUDE_DIRS}
    ${ILMBASE_INCLUDE_DIR}
    ${USD_INCLUDE_DIR}
    ${USD_INCLUDE_DIR}/../third_party/houdini/include
    ${USD_INCLUDE_DIR}/../third_party/houdini/include/hinclude
    ${USD_INCLUDE_DIR}/../../glfw/include
    ${USD_INCLUDE_DIR}/../../glew/include
	${HOUDINI_INCLUDE_DIRS})

# Put USD libraries as whole-archive.
target_link_libraries(
    ${OPWALTER}
    PRIVATE
    -Wl,--whole-archive
    ${USD_INCLUDE_DIR}/../lib/hdEmbree.a
    ${HYDRA_LIBRARIES}
    ${USD_LIBRARIES}
    walterUsdExtras
    -Wl,--no-whole-archive
    ${USD_ROOT}/third_party/houdini/lib/libgusd.a)

# Other libraries
target_link_libraries(
    ${OPWALTER}
    PRIVATE
    ${USD_INCLUDE_DIR}/../../embree/lib/libembree.a
    ${USD_INCLUDE_DIR}/../../embree/lib/libembree_avx.a
    ${USD_INCLUDE_DIR}/../../embree/lib/libembree_avx2.a
    ${USD_INCLUDE_DIR}/../../embree/lib/libembree_sse42.a
    ${USD_INCLUDE_DIR}/../../embree/lib/libsys.a
    ${USD_INCLUDE_DIR}/../../embree/lib/libsimd.a
    ${USD_INCLUDE_DIR}/../../embree/lib/libtasking.a
    ${USD_INCLUDE_DIR}/../../embree/lib/liblexers.a    
    walterCommon
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
    ${Boost_REGEX_LIBRARY}
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
    ${ALEMBIC_LIB}
    ${ALEMBIC_ILMBASE_HALF_LIB}
    ${ALEMBIC_ILMBASE_IEX_LIB}
    ${OPENGL_gl_LIBRARY}
    ${ALEMBIC_HDF5_LIB}
    rt
    ${USD_INCLUDE_DIR}/../../ocio/lib/libtinyxml.a
    ${USD_INCLUDE_DIR}/../../ocio/lib/libyaml-cpp.a
    ${USD_INCLUDE_DIR}/../../glfw/lib/libglfw3.a
    ${USD_INCLUDE_DIR}/../../glew/lib/libGLEW.a)

# Since Houdini uses boost in his API, we need to redirect the functions that
# use rdoBoost to the functions that use boost. We use wrapper functions for
# unresolved symbols. Any undefined reference to `symbol` will be resolved to
# `__wrap_symbol`. Any undefined reference to `__real_symbol` will be resolved
# to `symbol`. We need to replace 5boost to 8rdoBoost
string(LENGTH ${Boost_NAMESPACE} BOOSTLEN)
set(BOOSTVAR "${BOOSTLEN}${Boost_NAMESPACE}")

# Houdini libraries.
target_link_libraries(
    ${OPWALTER}
    PRIVATE
    ${HOUDINI_LIB_DIRS}/libHoudiniAPPS1.so
    ${HOUDINI_LIB_DIRS}/libHoudiniAPPS2.so
    ${HOUDINI_LIB_DIRS}/libHoudiniAPPS3.so
    ${HOUDINI_LIB_DIRS}/libHoudiniGEO.so
    ${HOUDINI_LIB_DIRS}/libHoudiniOP1.so
    ${HOUDINI_LIB_DIRS}/libHoudiniOP2.so
    ${HOUDINI_LIB_DIRS}/libHoudiniOP3.so
    ${HOUDINI_LIB_DIRS}/libHoudiniOPZ.so
    ${HOUDINI_LIB_DIRS}/libHoudiniUT.so
    ${HOUDINI_LIB_DIRS}/libHoudiniUI.so
    ${HOUDINI_LIB_DIRS}/libHoudiniPRM.so
    ${HOUDINI_LIB_DIRS}/libHoudiniRAY.so
    ${HOUDINI_LIB_DIRS}/libhboost_system.so)

install(
    FILES ${CMAKE_SOURCE_DIR}/maya/icons/walterwhite.png
    DESTINATION houdini/config/Icons)
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/otls/walter_procedural.hda
    DESTINATION houdini/otls)
install(
    FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/python/hou_walter/procedural.py
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/python/hou_walter/__init__.py
    DESTINATION houdini/scripts/python/hou_walter)

install(TARGETS ${OPWALTER} DESTINATION houdini/dso)
