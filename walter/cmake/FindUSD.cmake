# Copyright 2017 Rodeo FX.  All rights reserved.

# Trying to find USD include path.
find_path (
    USD_INCLUDE_DIR
    pxr/pxr.h
    PATHS ${USD_ROOT}/include
    NO_DEFAULT_PATH)

# Trying to get version
file(STRINGS ${USD_INCLUDE_DIR}/pxr/pxr.h TMP REGEX "^#define PXR_VERSION .*$")
string (REGEX MATCHALL "[0-9]+" USD_VERSION ${TMP})

# The list of required libraries for minimal USD.
set (_usd_components
    usdShade
    usdGeom
    usd
    usdUtils
    pcp
    sdf
    plug
    js
    ar
    work
    tf
    trace
    kind
    arch
    vt
    gf
    hf
    cameraUtil
    usdAbc
    usdLux
    usdSkel)

# The list of required libraries for Hydra.
set (_hydra_components
    usdImagingGL
    usdImaging
    usdHydra
    hdx
    hdSt
    hdStream
    hd
    glf
    garch
    pxOsd
    usdRi
    usdSkelImaging
    usdUI)

set (_usd_all_components ${_usd_components} ${_hydra_components})

# Trying to find all the libraries.
foreach (COMPONENT ${_usd_all_components})
    string (TOUPPER ${COMPONENT} UPPERCOMPONENT)

    unset (USD_${UPPERCOMPONENT}_LIBRARY CACHE)

    find_library (
        USD_${UPPERCOMPONENT}_LIBRARY
        NAMES ${COMPONENT} ${COMPONENT}${CMAKE_STATIC_LIBRARY_SUFFIX}
        PATHS ${USD_ROOT}/lib ${USD_ROOT}/plugin/usd
        NO_DEFAULT_PATH)
endforeach ()

set (USD_LIBRARIES "")
set (HYDRA_LIBRARIES "")

foreach (COMPONENT ${_usd_components})
    string (TOUPPER ${COMPONENT} UPPERCOMPONENT)
    list(APPEND USD_LIBRARIES ${USD_${UPPERCOMPONENT}_LIBRARY})
endforeach ()

foreach (COMPONENT ${_hydra_components})
    string (TOUPPER ${COMPONENT} UPPERCOMPONENT)
    list(APPEND HYDRA_LIBRARIES ${USD_${UPPERCOMPONENT}_LIBRARY})
endforeach ()

find_package_handle_standard_args (
    USD
    REQUIRED_VARS USD_INCLUDE_DIR USD_LIBRARIES HYDRA_LIBRARIES
    VERSION_VAR USD_VERSION)
