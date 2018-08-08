# Copyright 2017 Rodeo FX.  All rights reserved.

find_path (
    RDO_USD_RESOLVER_INCLUDE_DIR
    rdoResolver.h
    PATHS ${RDO_RESOLVER_ROOT}/include
    NO_DEFAULT_PATH)

find_library (
    RDO_USD_RESOLVER_LIBRARY
    NAMES librdoResolver${CMAKE_STATIC_LIBRARY_SUFFIX}
    PATHS ${RDO_RESOLVER_ROOT}/lib
    NO_DEFAULT_PATH)

find_package_handle_standard_args (
    RdoResolver
    REQUIRED_VARS RDO_USD_RESOLVER_INCLUDE_DIR RDO_USD_RESOLVER_LIBRARY
    VERSION_VAR RdoResolver_VERSION)
