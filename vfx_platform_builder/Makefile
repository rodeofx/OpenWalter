####################################################################
# Copyright 2018 Rodeo FX.  All rights reserved.
# Makefile to build external libraries
#
# Usage:
# make
# or
# make MAKE_MODE=debug
#
################################################################################

# Directories
SOURCES_ROOT=/tmp/src
BUILD_ROOT=/tmp/build
PREFIX_ROOT=/tmp/lib

# Versions
ALEMBIC_VERSION := 1.7.1
BLOSC_VERSION := v1.14.0
BOOST_VERSION := 1_55_0
CMAKE_VERSION := 3.7.2
DC_VERSION := v1.1.5
EMBREE_VERSION := v2.17.1
GLEW_VERSION := 2.0.0
GLFW_VERSION := 3.2.1
GLUT_VERSION := 3.0.0
GOOGLETEST_VERSION := release-1.8.0
HDF5_VERSION := 1.8.10
ILMBASE_VERSION := 2.2.0
ISPC_VERSION := v1.9.1
JPEG_VERSION := 1.5.1
JSONCPP_VERSION := 1.8.0
LLVM_VERS := 3.9.0
MERCURIAL_VERSION := 4.1.1
OCIO_VERSION := v1.0.9
OIIO_VERSION := Release-1.8.6
OPENEXR_VERSION := 2.2.0
OPENSUBD_VERSION := v3_2_0
OPENVDB_VERSION := v5.0.0
OSL_VERSION := Release-1.8.12
PNG_VERSION := 1.6.34
PTEX_VERSION := v2.1.28
PYILMBASE_VERSION := v2.2.0
PYOPENGL_VERSION := 3.0.1b1
PYSIDETOOLS_VERSION := 0.2.15
PYSIDE_VERSION := 1.2.4
PYSTRING_VERSION := v1.1.3
PYTHON_VERSION := 2.7.12
QT_VERSION := 4.8.6
QT5BASE_VERSION := v5.9.2
SHIBOKEN_VERSION := 1.2.4
TBB_VERSION := 2017_20161128oss
TIFF_VERSION := 3.8.2
USD_VERSION := v0.8.5
ZLIB_VERSION := v1.2.11

# Set the package name for USD. It also sets the internal namespace for USD.
USD_PACKAGE_NAME := usd

MAKE_MODE := release
ifeq "$(MAKE_MODE)" "debug"
CMAKE_BUILD_TYPE := Debug
else
CMAKE_BUILD_TYPE := Release
endif

# Save the current directory
THIS_DIR := $(shell pwd)

# Stamp files that should indicate that the build is successfull
ALEMBIC_STAMP := $(PREFIX_ROOT)/built_alembic
BLOSC_STAMP := $(PREFIX_ROOT)/built_blosc
BOOST_STAMP := $(PREFIX_ROOT)/built_boost
CMAKE_STAMP := $(PREFIX_ROOT)/built_cmake
DC_STAMP := $(PREFIX_ROOT)/built_dc
EMBREE_STAMP := $(PREFIX_ROOT)/built_embree
GLEW_STAMP := $(PREFIX_ROOT)/built_glew
GLFW_STAMP := $(PREFIX_ROOT)/built_glfw
GLUT_STAMP := $(PREFIX_ROOT)/built_glut
GOOGLETEST_STAMP := $(PREFIX_ROOT)/built_googletest
HDF5_STAMP := $(PREFIX_ROOT)/built_hdf5
ILMBASE_STAMP := $(PREFIX_ROOT)/built_ilmbase
JPEG_STAMP := $(PREFIX_ROOT)/built_jpeg
JSONCPP_STAMP := $(PREFIX_ROOT)/built_jsoncpp
MERCURIAL_STAMP := $(PREFIX_ROOT)/built_mercural
OCIO_STAMP := $(PREFIX_ROOT)/built_ocio
OIIO_STAMP := $(PREFIX_ROOT)/built_oiio
OPENEXR_STAMP := $(PREFIX_ROOT)/built_openexr
OPENSUBD_STAMP := $(PREFIX_ROOT)/built_opensubd
OPENVDB_STAMP := $(PREFIX_ROOT)/built_openvdb
OSL_STAMP := $(PREFIX_ROOT)/built_osl
PNG_STAMP := $(PREFIX_ROOT)/built_png
PTEX_STAMP := $(PREFIX_ROOT)/built_ptex
PYILMBASE_STAMP := $(PREFIX_ROOT)/built_pyilmbase
PYOPENGL_STAMP := $(PREFIX_ROOT)/built_pyopengl
PYSIDE_STAMP := $(PREFIX_ROOT)/built_pyside
PYSIDETOOLS_STAMP := $(PREFIX_ROOT)/built_pysidetools
PYSTRING_STAMP := $(PREFIX_ROOT)/built_pystring
PYTHON_STAMP := $(PREFIX_ROOT)/built_python
QT_STAMP := $(PREFIX_ROOT)/built_qt
QT5BASE_STAMP := $(PREFIX_ROOT)/built_qt5base
SHIBOKEN_STAMP := $(PREFIX_ROOT)/built_shiboken
TBB_STAMP := $(PREFIX_ROOT)/built_tbb
TIFF_STAMP := $(PREFIX_ROOT)/built_tiff
USD_STAMP := $(PREFIX_ROOT)/built_$(USD_PACKAGE_NAME)
ZLIB_STAMP := $(PREFIX_ROOT)/built_zlib

ALEMBIC_SOURCE := git://github.com/alembic/alembic.git
BLOSC_SOURCE := git://github.com/Blosc/c-blosc.git
BOOST_SOURCE := http://sourceforge.net/projects/boost/files/boost/$(subst _,.,$(BOOST_VERSION))/boost_$(BOOST_VERSION).tar.gz
CMAKE_SOURCE := https://cmake.org/files/v$(word 1,$(subst ., ,$(CMAKE_VERSION))).$(word 2,$(subst ., ,$(CMAKE_VERSION)))/cmake-$(CMAKE_VERSION).tar.gz
DC_SOURCE := git://github.com/google/double-conversion.git
EMBREE_SOURCE := git@github.com:embree/embree.git
GLEW_SOURCE := https://sourceforge.net/projects/glew/files/glew/$(GLEW_VERSION)/glew-$(GLEW_VERSION).tgz
GLFW_SOURCE := git://github.com/glfw/glfw.git
GLUT_SOURCE := https://sourceforge.net/projects/freeglut/files/freeglut/$(GLUT_VERSION)/freeglut-$(GLUT_VERSION).tar.gz
GOOGLETEST_SOURCE := git://github.com/google/googletest.git
HDF5_SOURCE := https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-$(word 1,$(subst ., ,$(HDF5_VERSION))).$(word 2,$(subst ., ,$(HDF5_VERSION)))/hdf5-$(HDF5_VERSION)/src/hdf5-$(HDF5_VERSION).tar.gz
ILMBASE_SOURCE := http://download.savannah.nongnu.org/releases/openexr/ilmbase-$(ILMBASE_VERSION).tar.gz
JPEG_SOURCE := git://github.com/libjpeg-turbo/libjpeg-turbo.git
JSONCPP_SOURCE := git://github.com/open-source-parsers/jsoncpp.git
MERCURIAL_SOURCE := https://www.mercurial-scm.org/release/mercurial-$(MERCURIAL_VERSION).tar.gz
OCIO_SOURCE := git://github.com/imageworks/OpenColorIO.git
OIIO_SOURCE := git://github.com/OpenImageIO/oiio.git
OPENEXR_SOURCE := http://download.savannah.nongnu.org/releases/openexr/openexr-$(OPENEXR_VERSION).tar.gz
OPENSUBD_SOURCE := git://github.com/PixarAnimationStudios/OpenSubdiv.git
OPENVDB_SOURCE := git://github.com/dreamworksanimation/openvdb.git
OSL_SOURCE := git://github.com/imageworks/OpenShadingLanguage.git
PNG_SOURCE := https://sourceforge.net/projects/libpng/files/libpng16/$(PNG_VERSION)/libpng-$(PNG_VERSION).tar.gz
PTEX_SOURCE := git://github.com/wdas/ptex.git
PYILMBASE_SOURCE := git://github.com/openexr/openexr.git
PYOPENGL_SOURCE := https://downloads.sourceforge.net/project/pyopengl/PyOpenGL/$(PYOPENGL_VERSION)/PyOpenGL-$(PYOPENGL_VERSION).tar.gz
PYSIDE_SOURCE := git@github.com:pyside/PySide.git
PYSIDETOOLS_SOURCE := git@github.com:pyside/Tools.git
PYSTRING_SOURCE := git://github.com/imageworks/pystring.git
PYTHON_SOURCE := https://www.python.org/ftp/python/$(PYTHON_VERSION)/Python-$(PYTHON_VERSION).tgz
QT_SOURCE := http://mirror.csclub.uwaterloo.ca/qtproject/archive/qt/$(word 1,$(subst ., ,$(QT_VERSION))).$(word 2,$(subst ., ,$(QT_VERSION)))/$(QT_VERSION)/qt-everywhere-opensource-src-$(QT_VERSION).tar.gz
QT5BASE_SOURCE := git@github.com:qt/qtbase.git
SHIBOKEN_SOURCE := git@github.com:pyside/Shiboken.git
TBB_SOURCE := https://www.threadingbuildingblocks.org/sites/default/files/software_releases/source/tbb$(TBB_VERSION)_src.tgz
TIFF_SOURCE := http://dl.maptools.org/dl/libtiff/tiff-$(TIFF_VERSION).tar.gz
USD_SOURCE := git://github.com/PixarAnimationStudios/USD
ZLIB_SOURCE := git://github.com/madler/zlib.git

ALEMBIC_FILE := $(SOURCES_ROOT)/$(notdir $(ALEMBIC_SOURCE))
BLOSC_FILE := $(SOURCES_ROOT)/$(notdir $(BLOSC_SOURCE))
BOOST_FILE := $(SOURCES_ROOT)/$(notdir $(BOOST_SOURCE))
CMAKE_FILE := $(SOURCES_ROOT)/$(notdir $(CMAKE_SOURCE))
DC_FILE := $(SOURCES_ROOT)/$(notdir $(DC_SOURCE))
EMBREE_FILE := $(SOURCES_ROOT)/$(notdir $(EMBREE_SOURCE))
GLEW_FILE := $(SOURCES_ROOT)/$(notdir $(GLEW_SOURCE))
GLFW_FILE := $(SOURCES_ROOT)/$(notdir $(GLFW_SOURCE))
GLUT_FILE := $(SOURCES_ROOT)/$(notdir $(GLUT_SOURCE))
GOOGLETEST_FILE := $(SOURCES_ROOT)/$(notdir $(GOOGLETEST_SOURCE))
HDF5_FILE := $(SOURCES_ROOT)/$(notdir $(HDF5_SOURCE))
ILMBASE_FILE := $(SOURCES_ROOT)/$(notdir $(ILMBASE_SOURCE))
JPEG_FILE := $(SOURCES_ROOT)/$(notdir $(JPEG_SOURCE))
JSONCPP_FILE := $(SOURCES_ROOT)/$(notdir $(JSONCPP_SOURCE))
MERCURIAL_FILE := $(SOURCES_ROOT)/$(notdir $(MERCURIAL_SOURCE))
OCIO_FILE := $(SOURCES_ROOT)/$(notdir $(OCIO_SOURCE))
OIIO_FILE := $(SOURCES_ROOT)/$(notdir $(OIIO_SOURCE))
OPENEXR_FILE := $(SOURCES_ROOT)/$(notdir $(OPENEXR_SOURCE))
OPENSUBD_FILE := $(SOURCES_ROOT)/$(notdir $(OPENSUBD_SOURCE))
OPENVDB_FILE := $(SOURCES_ROOT)/$(notdir $(OPENVDB_SOURCE))
OSL_FILE := $(SOURCES_ROOT)/$(notdir $(OSL_SOURCE))
PNG_FILE := $(SOURCES_ROOT)/$(notdir $(PNG_SOURCE))
PTEX_FILE := $(SOURCES_ROOT)/$(notdir $(PTEX_SOURCE))
PYILMBASE_FILE := $(SOURCES_ROOT)/$(notdir $(PYILMBASE_SOURCE))
PYOPENGL_FILE := $(SOURCES_ROOT)/$(notdir $(PYOPENGL_SOURCE))
PYSIDE_FILE := $(SOURCES_ROOT)/$(notdir $(PYSIDE_SOURCE))
PYSIDETOOLS_FILE := $(SOURCES_ROOT)/$(notdir $(PYSIDETOOLS_SOURCE))
PYSTRING_FILE := $(SOURCES_ROOT)/$(notdir $(PYSTRING_SOURCE))
PYTHON_FILE := $(SOURCES_ROOT)/$(notdir $(PYTHON_SOURCE))
QT_FILE := $(SOURCES_ROOT)/$(notdir $(QT_SOURCE))
QT5BASE_FILE := $(SOURCES_ROOT)/$(notdir $(QT5BASE_SOURCE))
SHIBOKEN_FILE := $(SOURCES_ROOT)/$(notdir $(SHIBOKEN_SOURCE))
TBB_FILE := $(SOURCES_ROOT)/$(notdir $(TBB_SOURCE))
TIFF_FILE := $(SOURCES_ROOT)/$(notdir $(TIFF_SOURCE))
USD_FILE := $(SOURCES_ROOT)/$(notdir $(USD_SOURCE))
ZLIB_FILE := $(SOURCES_ROOT)/$(notdir $(ZLIB_SOURCE))

# Number of processors
ifeq "$(OS)" "Darwin"
JOB_COUNT := $(shell sysctl -n machdep.cpu.thread_count)
endif
ifeq "$(OS)" "linux"
JOB_COUNT := $(shell cat /sys/devices/system/cpu/cpu*/topology/thread_siblings | wc -l)
endif

# DSO extension
DYNAMIC_EXT := .a
ifeq "$(BOOST_LINK)" "shared"
ifeq "$(OS)" "Darwin"
DYNAMIC_EXT := .dylib
endif
ifeq "$(OS)" "linux"
DYNAMIC_EXT := .so
endif
endif

# Compiler specific options.
# Try to use the compiler of devtoolset-2 first.
GCC_BIN_PATH := /opt/rh/devtoolset-2/root/usr/bin
ifeq ("$(wildcard $(GCC_BIN_PATH)/gcc)","")
GCC_BIN_PATH := /usr/bin
endif
AR := $(shell which ar)
AUTORECONF := $(shell which autoreconf)
CC := $(GCC_BIN_PATH)/gcc
CXX := $(GCC_BIN_PATH)/g++
LIBTOOL := $(shell which libtool)
MAKE := $(shell which make)
NASM := $(shell which nasm)
FLAGS := -fPIC
BOOST_LINK := static
PYTHON :=
HOUDINI_ROOT := ""
BUILD_HOUDINI_PLUGINS := ON
KATANA_ROOT := ""
BUILD_KATANA_PLUGINS := ON
MAYA_ROOT := ""

CLANG := $(PREFIX_ROOT)/llvm/bin/clang
ISPC := $(PREFIX_ROOT)/ispc/bin/ispc
BISON := $(PREFIX_ROOT)/bison/bin/bison
FLEX := $(PREFIX_ROOT)/flex/bin/flex

# Change tbb namespace. We need it to avoid conflicts with tbb already used in
# Maya.
ifeq "$(TBB_NAMESPACE)" ""
FLAGS += -Dtbb=rdotbb
else
ifneq "$(TBB_NAMESPACE)" "tbb"
FLAGS += -Dtbb=$(TBB_NAMESPACE)
endif
endif

ifeq "$(PYTHON)" ""
PYTHON_BIN := $(PREFIX_ROOT)/python/bin/python
PYTHON_VERSION_SHORT := $(word 1,$(subst ., ,$(PYTHON_VERSION))).$(word 2,$(subst ., ,$(PYTHON_VERSION)))
else
PYTHON_BIN := $(PYTHON)
PYTHON_VERSION_SHORT = 2.7
PYTHON_STAMP =
endif
PYTHON_ROOT := $(realpath $(dir $(PYTHON_BIN))/..)

ifeq "$(MAKE_MODE)" "debug"
FLAGS += -g
else
FLAGS += -O3
endif

ifeq "$(BOOST_VERSION)" "1_55_0"
ifeq "$(OS)" "Darwin"
# Fixed macOS compilation error for boost 1.55.0
# https://svn.boost.org/trac/boost/ticket/9610
FLAGS += -DBOOST_HAS_INT128=1
endif
endif

COMPILER_CONF :=\
	CC="$(CC)" \
	CXX="$(CXX)" \
	CFLAGS="$(FLAGS)" \
	CXXFLAGS="$(FLAGS)"

CMAKE := $(PREFIX_ROOT)/cmake/bin/cmake
COMMON_CMAKE_FLAGS :=\
	-DCMAKE_BUILD_TYPE:STRING=$(CMAKE_BUILD_TYPE) \
	-DCMAKE_CXX_COMPILER:STRING=$(CXX) \
	-DCMAKE_CXX_FLAGS:STRING="$(FLAGS)" \
	-DCMAKE_C_COMPILER:STRING=$(CC) \
	-DCMAKE_C_FLAGS:STRING="$(FLAGS)" \
	-DCMAKE_INSTALL_LIBDIR=lib \
	-DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=ON \
	-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON \
	-DCMAKE_SHARED_LINKER_FLAGS=-Wl,--no-undefined \
	-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON

PYSIDE_CONF := \
	PYTHONPATH=$(PREFIX_ROOT)/pyside/lib/python2.7/site-packages:$(PREFIX_ROOT)/shiboken/lib/python2.7/site-packages:$(PREFIX_ROOT)/pyopengl/lib/python2.7/site-packages

ALL_TARGETS :=\
	$(ALEMBIC_STAMP) \
	$(GOOGLETEST_STAMP) \
	$(JSONCPP_STAMP) \
	$(OIIO_STAMP) \
	$(OPENVDB_STAMP) \
	$(USD_STAMP)

manglevariable = `echo -e "void $(1)() {} " | $(CXX) -x c++ -S - -o- | grep "^_.*:$$" | sed -e 's/v:$$//' | sed -e 's/^_Z//'`

all: $(ALL_TARGETS)

# Shortcuts
alembic: $(ALEMBIC_STAMP)
blosc: $(BLOSC_STAMP)
boost: $(BOOST_STAMP)
cmake: $(CMAKE_STAMP)
doubleconversion: $(DC_STAMP)
embree: $(EMBREE_STAMP)
glew: $(GLEW_STAMP)
glfw: $(GLFW_STAMP)
glut: $(GLUT_STAMP)
googletest: $(GOOGLETEST_STAMP)
hdf5: $(HDF5_STAMP)
ilmbase: $(ILMBASE_STAMP)
ispc: $(ISPC)
jpeg: $(JPEG_STAMP)
jsoncpp: $(JSONCPP_STAMP)
llvm: $(CLANG)
mercurial: $(MERCURIAL_STAMP)
ocio: $(OCIO_STAMP)
oiio: $(OIIO_STAMP)
openexr: $(OPENEXR_STAMP)
opensubdiv: $(OPENSUBD_STAMP)
openvdb: $(OPENVDB_STAMP)
osl: $(OSL_STAMP)
png: $(PNG_STAMP)
ptex: $(PTEX_STAMP)
pyilmbase: $(PYILMBASE_STAMP)
pyopengl: $(PYOPENGL_STAMP)
pyside: $(PYSIDE_STAMP)
pysidetools: $(PYSIDETOOLS_STAMP)
pystring: $(PYSTRING_STAMP)
python: $(PYTHON_STAMP)
qt: $(QT_STAMP)
qt5: $(QT5BASE_STAMP)
shiboken: $(SHIBOKEN_STAMP)
tbb: $(TBB_STAMP)
tiff: $(TIFF_STAMP)
usd: $(USD_STAMP)
zlib: $(ZLIB_STAMP)

download : $(BOOST_FILE) $(CMAKE_FILE) $(ILMBASE_FILE) $(JPEG_FILE)/HEAD $(JSONCPP_FILE)/HEAD $(OCIO_FILE)/HEAD $(OIIO_FILE)/HEAD $(OPENEXR_FILE) $(PNG_FILE) $(PYSTRING_FILE)/HEAD $(PYTHON_FILE) $(TIFF_FILE) $(ZLIB_FILE)/HEAD

help:
	@printf 'Automatic building system for external tools\n\n' ; \
	printf 'Usage:\n\tmake\nor\n\tmake MAKE_MODE=debug\n\n' ; \
	printf 'Options:\n' ; \
	printf '\tCC\t\t: C copiler path\t: $(CC)\n' ; \
	printf '\tCXX\t\t: C++ copiler path\t: $(CXX)\n' ; \
	printf '\tMAKE_MODE\t: debug|release\t\t: $(MAKE_MODE)\n' ; \
	printf '\tSOURCES_ROOT\t: Source directory\t: $(SOURCES_ROOT)\n' ; \
	printf '\tBUILD_ROOT\t: Building directory\t: $(BUILD_ROOT)\n' ; \
	printf '\tPREFIX_ROOT\t: Installation dir\t: $(PREFIX_ROOT)\n' ; \
	printf '\nTo build Alembic or OIIO:\n\tmake\n\n' ; \
	printf '\nTo build USD:\n\tmake BOOST_LINK=shared usd\n\n' ; \
	printf 'CentOS packages to install:\n' ; \
	printf 'nasm libXxf86vm-devel\n' ; \
	printf '\nBuild everything with clang:\n' ; \
	printf 'make PREFIX_ROOT=/tmp/tmplib llvm\n' ; \
	printf 'make CC=/tmp/tmplib/llvm/bin/clang CXX=/tmp/tmplib/llvm/bin/clang++ llvm\n' ; \
	printf 'make CC=/tmp/lib/llvm/bin/clang CXX=/tmp/lib/llvm/bin/clang++\n\n' ; \
	printf '\nBuild PyAlembic:\n' ; \
	printf 'make BOOST_NAMESPACE=rdoBoostAlembic BOOST_LINK=shared alembic\n\n' ; \
	printf '\nBuild USD with custom namespace:\n' ; \
	printf 'make USD_PACKAGE_NAME=usdProcedural\n\n' ;


# Download
$(ALEMBIC_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(ALEMBIC_FILE)... && \
	git clone -q --bare $(ALEMBIC_SOURCE) $(ALEMBIC_FILE)
$(BLOSC_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(BLOSC_FILE)... && \
	git clone -q --bare $(BLOSC_SOURCE) $(BLOSC_FILE)
$(BOOST_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(BOOST_FILE)... && \
	curl -s -o $@ -L $(BOOST_SOURCE)
$(CMAKE_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(CMAKE_FILE)... && \
	curl -s -o $@ -L $(CMAKE_SOURCE)
$(DC_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(DC_FILE)... && \
	git clone -q --bare $(DC_SOURCE) $(DC_FILE)
$(EMBREE_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(EMBREE_FILE)... && \
	git clone -q --bare $(EMBREE_SOURCE) $(EMBREE_FILE)
$(GLEW_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(GLEW_FILE)... && \
	curl -s -o $@ -L $(GLEW_SOURCE)
$(GLFW_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(GLFW_FILE)... && \
	git clone -q --bare $(GLFW_SOURCE) $(GLFW_FILE)
$(GLUT_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(GLUT_FILE)... && \
	curl -s -o $@ -L $(GLUT_SOURCE)
$(GOOGLETEST_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(GOOGLETEST_FILE)... && \
	git clone -q --bare $(GOOGLETEST_SOURCE) $(GOOGLETEST_FILE)
$(HDF5_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(HDF5_FILE)... && \
	curl --tlsv1.2 -s -o $@ -L $(HDF5_SOURCE)
$(ILMBASE_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(ILMBASE_FILE)... && \
	curl -s -o $@ -L $(ILMBASE_SOURCE)
$(JPEG_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(JPEG_FILE)... && \
	git clone -q --bare $(JPEG_SOURCE) $(JPEG_FILE)
$(JSONCPP_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(JSONCPP_FILE)... && \
	git clone -q --bare $(JSONCPP_SOURCE) $(JSONCPP_FILE)
$(MERCURIAL_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(MERCURIAL_FILE)... && \
	curl -s -o $@ -L $(MERCURIAL_SOURCE)
$(OCIO_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(OCIO_FILE)... && \
	git clone -q --bare $(OCIO_SOURCE) $(OCIO_FILE)
$(OIIO_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(OIIO_FILE)... && \
	git clone -q --bare $(OIIO_SOURCE) $(OIIO_FILE)
$(OPENEXR_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(OPENEXR_FILE)... && \
	curl -s -o $@ -L $(OPENEXR_SOURCE)
$(OPENSUBD_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(OPENSUBD_FILE)... && \
	git clone -q --bare $(OPENSUBD_SOURCE) $(OPENSUBD_FILE)
$(OPENVDB_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(OPENVDB_FILE)... && \
	git clone -q --bare $(OPENVDB_SOURCE) $(OPENVDB_FILE)
$(OSL_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(OSL_FILE)... && \
	git clone -q --bare $(OSL_SOURCE) $(OSL_FILE)
$(PNG_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(PNG_FILE)... && \
	curl -s -o $@ -L $(PNG_SOURCE)
$(PTEX_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(PTEX_FILE)... && \
	git clone -q --bare $(PTEX_SOURCE) $(PTEX_FILE)
$(PYILMBASE_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(PYILMBASE_FILE)... && \
	git clone -q --bare $(PYILMBASE_SOURCE) $(PYILMBASE_FILE)
$(PYOPENGL_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(PYOPENGL_FILE)... && \
	curl --tlsv1.2 -s -o $@ -L $(PYOPENGL_SOURCE)
$(PYSIDE_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(PYSIDE_FILE)... && \
	git clone -q --bare $(PYSIDE_SOURCE) $(PYSIDE_FILE)
$(PYSIDETOOLS_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(PYSIDETOOLS_FILE)... && \
	git clone -q --bare $(PYSIDETOOLS_SOURCE) $(PYSIDETOOLS_FILE)
$(PYSTRING_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(PYSTRING_FILE)... && \
	git clone -q --bare $(PYSTRING_SOURCE) $(PYSTRING_FILE)
$(PYTHON_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(PYTHON_FILE)... && \
	curl -s -o $@ -L $(PYTHON_SOURCE)
$(QT_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(QT_FILE)... && \
	curl -s -o $@ -L $(QT_SOURCE)
$(QT5BASE_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(QT5BASE_FILE)... && \
	git clone -q --bare $(QT5BASE_SOURCE) $(QT5BASE_FILE)
$(SHIBOKEN_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(SHIBOKEN_FILE)... && \
	git clone -q --bare $(SHIBOKEN_SOURCE) $(SHIBOKEN_FILE)
$(TBB_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(TBB_FILE)... && \
	curl -s -o $@ -L $(TBB_SOURCE)
$(TIFF_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(TIFF_FILE)... && \
	curl -s -o $@ -L $(TIFF_SOURCE)
$(ZLIB_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(ZLIB_FILE)... && \
	git clone -q --bare $(ZLIB_SOURCE) $(ZLIB_FILE)
$(USD_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(USD_FILE)... && \
	git clone -q --bare $(USD_SOURCE) $(USD_FILE)


# Boost
ifeq "$(BOOST_VERSION)" "1_55_0"
BOOST_USERCONFIG := tools/build/v2/user-config.jam
else
BOOST_USERCONFIG := tools/build/src/user-config.jam
endif
BOOST_NAMESPACE := rdoBoost
ifeq "$(BOOST_LINK)" "shared"
USE_STATIC_BOOST := OFF
USE_SHARED_BOOST := ON
else
USE_STATIC_BOOST := ON
USE_SHARED_BOOST := OFF
PYILMBASE_STAMP :=
PYSIDETOOLS_STAMP :=
PYSIDE_STAMP :=
endif
$(BOOST_STAMP) : $(PYTHON_STAMP) $(BOOST_FILE)
	@echo Building boost $(BOOST_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf boost_$(BOOST_VERSION) && \
	rm -rf $(BOOST_NAMESPACE) && \
	tar -xf $(SOURCES_ROOT)/boost_$(BOOST_VERSION).tar.gz && \
	cd boost_$(BOOST_VERSION) && \
	echo ">>>" Patching Boost. Regex crashed when defining the second time... && \
	( printf "/static object_data/s/static object_data/object_data/\nw\nq" | ed -s boost/regex/pending/object_cache.hpp ) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(COMPILER_CONF) TOOLSET=cc \
	./bootstrap.sh > $(PREFIX_ROOT)/log_boost.txt 2>&1 && \
	echo 'using gcc : 4.8 : "$(CXX)" ;' >> $(BOOST_USERCONFIG) && \
	./b2 \
		-j $(JOB_COUNT) \
		tools/bcp >> $(PREFIX_ROOT)/log_boost.txt 2>&1 && \
	mkdir ../$(BOOST_NAMESPACE) && \
	dist/bin/bcp \
		--namespace=$(BOOST_NAMESPACE) \
		--namespace-alias \
		build libs boost tools bootstrap.sh bootstrap.bat boostcpp.jam boost-build.jam \
		$(BUILD_ROOT)/$(BOOST_NAMESPACE) >> $(PREFIX_ROOT)/log_boost.txt 2>&1 && \
	cd ../$(BOOST_NAMESPACE) && \
	$(COMPILER_CONF) TOOLSET=cc \
	./bootstrap.sh \
		--with-python-version=$(PYTHON_VERSION_SHORT) \
		--with-python-root=$(PYTHON_ROOT) \
		--with-python=$(PYTHON_BIN) \
		--prefix=$(PREFIX_ROOT)/boost >> $(PREFIX_ROOT)/log_boost.txt 2>&1 && \
	./b2 \
		--layout=system \
		--prefix=$(PREFIX_ROOT)/boost \
		-j $(JOB_COUNT) \
		-s NO_BZIP2=1 \
		link=$(BOOST_LINK) \
		threading=multi \
		toolset=gcc-4.8 \
		cxxflags="$(FLAGS)" cflags="$(FLAGS)" \
		include=$(PYTHON_ROOT)/include/python$(PYTHON_VERSION_SHORT) \
		$(MAKE_MODE) \
		stage \
		install >> $(PREFIX_ROOT)/log_boost.txt 2>&1 && \
	cd .. && \
	rm -rf boost_$(BOOST_VERSION) && \
	rm -rf $(BOOST_NAMESPACE) && \
	cd $(THIS_DIR) && \
	echo $(BOOST_VERSION) > $@


# Alembic
# Edits:
# - Remove Werror, it fails the build.
$(ALEMBIC_STAMP) : $(BOOST_STAMP) $(CMAKE_STAMP) $(HDF5_STAMP) $(ILMBASE_STAMP) $(OPENEXR_STAMP) $(PYILMBASE_STAMP) $(ZLIB_STAMP) $(ALEMBIC_FILE)/HEAD
	@echo Building Alembic $(ALEMBIC_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf alembic && \
	git clone -q --no-checkout $(SOURCES_ROOT)/alembic.git alembic && \
	cd alembic && \
	git checkout -q $(ALEMBIC_VERSION) && \
	git apply $(THIS_DIR)/patches/Alembic/0001-RodeoFX-modification-of-AbcConvert.patch && \
	git apply $(THIS_DIR)/patches/Alembic/0002-RodeoFX-modification-of-AbcConvert.patch && \
	git apply $(THIS_DIR)/patches/Alembic/0003-AbcExport-the-ability-to-set-metadata.patch && \
	git apply $(THIS_DIR)/patches/Alembic/0004-AbcCoreLayer-merging-objects-with-different-names.patch && \
	git apply $(THIS_DIR)/patches/Alembic/0005-AbcGeom-ability-to-add-replace-metadata-to-UVs.patch && \
	git apply $(THIS_DIR)/patches/Alembic/0006-RodeoFX-modification-of-AbcConvert.patch && \
	git apply $(THIS_DIR)/patches/Alembic/0007-RodeoFX-AbcConvert-replace-string-properties.patch && \
	git apply $(THIS_DIR)/patches/Alembic/0008-RodeoFX-ability-to-change-property-name.patch && \
	git apply $(THIS_DIR)/patches/Alembic/0009-RodeoFX-better-help-string.patch && \
	git apply $(THIS_DIR)/patches/Alembic/0010-RodeoFX-ability-to-change-the-names-of-compounds.patch && \
	( printf '/Werror/d\nw\nq' | ed -s CMakeLists.txt ) && \
	( printf "/INSTALL/a\nFoundation.h\n.\nw\nq" | ed -s lib/Alembic/AbcCoreLayer/CMakeLists.txt ) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DALEMBIC_ILMBASE_LINK_STATIC:BOOL=ON \
		-DALEMBIC_LIB_USES_BOOST:BOOL=ON \
		-DALEMBIC_SHARED_LIBS:BOOL=OFF \
		-DBOOST_ROOT:STRING=$(PREFIX_ROOT)/boost \
		-DBUILD_SHARED_LIBS:BOOL=OFF \
		-DBoost_NAMESPACE=$(BOOST_NAMESPACE) \
		-DCMAKE_CXX_FLAGS="$(FLAGS) -fPIC -std=c++11" \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/alembic \
		-DHDF5_ROOT=$(PREFIX_ROOT)/hdf5 \
		-DILMBASE_ROOT=$(PREFIX_ROOT)/ilmbase \
		-DMAYA_ROOT=$(MAYA_ROOT) \
		-DPYILMBASE_ROOT:PATH=$(PREFIX_ROOT)/pyilmbase \
		-DPYTHON_EXECUTABLE:FILEPATH=$(PYTHON_BIN) \
		-DUSE_BOOSTREGEX:BOOL=ON \
		-DUSE_HDF5:BOOL=ON \
		-DUSE_MAYA:BOOL=ON \
		-DUSE_PYALEMBIC:BOOL=$(USE_SHARED_BOOST) \
		-DUSE_STATIC_BOOST:BOOL=$(USE_STATIC_BOOST) \
		-DUSE_STATIC_HDF5:BOOL=ON \
		-DUSE_TESTS:BOOL=OFF \
		-DZLIB_ROOT:PATH=$(PREFIX_ROOT)/zlib \
		. > $(PREFIX_ROOT)/log_alembic.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_alembic.txt 2>&1 && \
	cd .. && \
	rm -rf alembic && \
	cd $(THIS_DIR) && \
	echo $(ALEMBIC_VERSION) > $@


# BLOSC
$(BLOSC_STAMP) : $(CMAKE_STAMP) $(BLOSC_FILE)/HEAD
	@echo Building Blosc $(BLOSC_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(BLOSC_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(BLOSC_FILE)) $(notdir $(basename $(BLOSC_FILE))) && \
	cd $(notdir $(basename $(BLOSC_FILE))) && \
	git checkout -q $(BLOSC_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DBUILD_BENCHMARKS:BOOL=OFF \
		-DBUILD_SHARED:BOOL=OFF \
		-DBUILD_STATIC:BOOL=ON \
		-DBUILD_TESTS:BOOL=OFF \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/blosc \
		-DPREFER_EXTERNAL_ZLIB:BOOL=ON \
		-DZLIB_ROOT:PATH=$(PREFIX_ROOT)/zlib \
		. > $(PREFIX_ROOT)/log_blosc.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_blosc.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(BLOSC_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(BLOSC_VERSION) > $@


# Bison
$(BISON) :
	@echo Building Bison && \
	$(MAKE) $(MAKE_OPTS) -C ./system \
		BUILD_ROOT=$(BUILD_ROOT) \
		CLANG=$(CLANG) \
		CMAKE=$(CMAKE) \
		CMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		COMMON_CMAKE_FLAGS='$(COMMON_CMAKE_FLAGS)' \
		PREFIX_ROOT=$(PREFIX_ROOT) \
		SOURCES_ROOT=$(SOURCES_ROOT) \
		bison -j$(JOB_COUNT) > $(PREFIX_ROOT)/log_bison.txt 2>&1


# CMake
$(CMAKE_STAMP) : $(CMAKE_FILE)
	@echo Building cmake $(CMAKE_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf cmake-$(CMAKE_VERSION) && \
	tar -xf $(SOURCES_ROOT)/cmake-$(CMAKE_VERSION).tar.gz && \
	cd cmake-$(CMAKE_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(COMPILER_CONF) \
	./configure \
		--prefix="$(PREFIX_ROOT)/cmake" \
		--parallel=$(JOB_COUNT) \
		--no-qt-gui \
		--no-server \
		--verbose > $(PREFIX_ROOT)/log_cmake.txt 2>&1 && \
	$(MAKE) -j$(JOB_COUNT) \
		MAKE_MODE=$(MAKE_MODE) \
		install >> $(PREFIX_ROOT)/log_cmake.txt 2>&1 && \
	cd .. && \
	rm -rf cmake-$(CMAKE_VERSION) && \
	cd $(THIS_DIR) && \
	echo $(CMAKE_VERSION) > $@


# DoubleConversion
$(DC_STAMP) : $(CMAKE_STAMP) $(DC_FILE)/HEAD
	@echo Building double-conversion $(DC_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(DC_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(DC_FILE)) $(notdir $(basename $(DC_FILE))) && \
	cd $(notdir $(basename $(DC_FILE))) && \
	git checkout -q $(DC_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/dc \
		. > $(PREFIX_ROOT)/log_dc.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_dc.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(DC_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(DC_VERSION) > $@


# DoubleConversion
$(EMBREE_STAMP) : $(CMAKE_STAMP) $(ISPC) $(GLUT_STAMP) $(EMBREE_FILE)/HEAD $(TBB_STAMP)
	@echo Building embree $(EMBREE_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(EMBREE_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(EMBREE_FILE)) $(notdir $(basename $(EMBREE_FILE))) && \
	cd $(notdir $(basename $(EMBREE_FILE))) && \
	git checkout -q $(EMBREE_VERSION) && \
	( printf '/ADD_EXECUTABLE/a\nfind_package(X11 REQUIRED)\n.\nw\nq' | ed -s common/cmake/tutorial.cmake ) && \
	( printf '/TARGET_LINK_LIBRARIES/s/)/ \044{X11_LIBRARIES} Xxf86vm Xrandr)/\nw\nq' | ed -s common/cmake/tutorial.cmake ) && \
	( printf '/TARGET_LINK_LIBRARIES.*ispc/s/)/ \044{X11_LIBRARIES} Xxf86vm Xrandr)/\nw\nq' | ed -s common/cmake/tutorial.cmake ) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/embree \
		-DEMBREE_IGNORE_CMAKE_CXX_FLAGS:BOOL=OFF \
		-DEMBREE_STATIC_LIB:BOOL=ON \
		-DEMBREE_TUTORIALS:BOOL=ON \
		-DGLUT_Xmu_LIBRARY= \
		-DISPC_DIR_HINT=$(dir $(ISPC)) \
		-DTBB_INCLUDE_DIR=$(PREFIX_ROOT)/tbb/include \
		-DTBB_LIBRARY=$(PREFIX_ROOT)/tbb/lib/libtbb.a \
		-DTBB_LIBRARY_MALLOC=$(PREFIX_ROOT)/tbb/lib/libtbb.a \
		-D_GLUT_INC_DIR:PATH=$(PREFIX_ROOT)/glut/include \
		-D_GLUT_glut_LIB_DIR:PATH=$(PREFIX_ROOT)/glut/lib \
		. > $(PREFIX_ROOT)/log_embree.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_embree.txt 2>&1 && \
	cp -rf *.a $(PREFIX_ROOT)/embree/lib && \
	cd .. && \
	rm -rf $(notdir $(basename $(EMBREE_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(DC_VERSION) > $@


# Flex
$(FLEX) :
	@echo Building Flex && \
	$(MAKE) $(MAKE_OPTS) -C ./system \
		BUILD_ROOT=$(BUILD_ROOT) \
		CLANG=$(CLANG) \
		CMAKE=$(CMAKE) \
		CMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		COMMON_CMAKE_FLAGS='$(COMMON_CMAKE_FLAGS)' \
		PREFIX_ROOT=$(PREFIX_ROOT) \
		SOURCES_ROOT=$(SOURCES_ROOT) \
		flex -j$(JOB_COUNT) > $(PREFIX_ROOT)/log_flex.txt 2>&1

# glew
# Edits:
# - define GLEW_STATIC
# link glewinfo and visualinfo statically
$(GLEW_STAMP) : $(CMAKE_STAMP) $(GLEW_FILE)
	@echo Building glew $(GLEW_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(GLEW_FILE))) && \
	tar zxf $(SOURCES_ROOT)/$(notdir $(GLEW_FILE)) && \
	cd $(notdir $(basename $(GLEW_FILE))) && \
	( printf "0a\n#define GLEW_STATIC\n.\nw\nq\n" | ed -s include/GL/glew.h ) && \
	( printf "0a\n#define GLEW_STATIC\n.\nw\nq\n" | ed -s include/GL/wglew.h ) && \
	( printf "/target_link_libraries.*glewinfo/s/glew)/glew_s)/\nw\nq" | ed -s build/cmake/CMakeLists.txt ) && \
	( printf "/target_link_libraries.*visualinfo/s/glew)/glew_s)/\nw\nq" | ed -s build/cmake/CMakeLists.txt ) && \
	( printf "/CMAKE_DEBUG_POSTFIX/d\nw\nq" | ed -s build/cmake/CMakeLists.txt ) && \
	cd build && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/glew \
		./cmake > $(PREFIX_ROOT)/log_glew.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_glew.txt 2>&1 && \
	( ls -d -1 $(PREFIX_ROOT)/glew/lib/*GLEW* | grep -v -F .a | xargs rm ) && \
	cd ../.. && \
	rm -rf $(notdir $(basename $(GLEW_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(GLEW_VERSION) > $@


# glfw
$(GLFW_STAMP) : $(CMAKE_STAMP) $(GLFW_FILE)/HEAD
	@echo Building glfw $(GLFW_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(GLFW_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(GLFW_FILE)) $(notdir $(basename $(GLFW_FILE))) && \
	cd $(notdir $(basename $(GLFW_FILE))) && \
	git checkout -q $(GLFW_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DGLFW_BUILD_DOCS:BOOL=OFF \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/glfw \
		-DZLIB_ROOT:PATH=$(PREFIX_ROOT)/zlib \
		. > $(PREFIX_ROOT)/log_glfw.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_glfw.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(GLFW_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(GLFW_VERSION) > $@


# glut
$(GLUT_STAMP) : $(CMAKE_STAMP) $(GLUT_FILE)
	@echo Building glut $(GLUT_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(basename $(GLUT_FILE)))) && \
	tar -xf $(SOURCES_ROOT)/$(notdir $(GLUT_FILE)) && \
	cd $(notdir $(basename $(basename $(GLUT_FILE)))) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DFREEGLUT_BUILD_SHARED_LIBS:BOOL=OFF \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/glut \
		. > $(PREFIX_ROOT)/log_glut.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_glut.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(basename $(GLUT_FILE)))) && \
	cd $(THIS_DIR) && \
	echo $(GLUT_VERSION) > $@


# Google Test
$(GOOGLETEST_STAMP) : $(CMAKE_STAMP) $(GOOGLETEST_FILE)/HEAD
	@echo Building Google Test $(GOOGLETEST_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(GOOGLETEST_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(GOOGLETEST_FILE)) $(notdir $(basename $(GOOGLETEST_FILE))) && \
	cd $(notdir $(basename $(GOOGLETEST_FILE))) && \
	git checkout -q $(GOOGLETEST_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/googletest \
		. > $(PREFIX_ROOT)/log_googletest.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_googletest.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(GOOGLETEST_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(GOOGLETEST_VERSION) > $@


# HDF5
$(HDF5_STAMP) : $(CMAKE_STAMP) $(ZLIB_STAMP) $(HDF5_FILE)
	@echo Building HDF5 $(HDF5_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf hdf5-$(HDF5_VERSION) && \
	tar -xf $(SOURCES_ROOT)/hdf5-$(HDF5_VERSION).tar.gz && \
	cd hdf5-$(HDF5_VERSION) && \
	( test $$OS != linux || if [ -f release_docs/USING_CMake.txt ] ; then cp release_docs/USING_CMake.txt release_docs/Using_CMake.txt ; fi ) && \
	( if [ ! -f release_docs/USING_CMake.txt ] ; then touch release_docs/USING_CMake.txt ; fi ) && \
	( if [ ! -f release_docs/Using_CMake.txt ] ; then touch release_docs/Using_CMake.txt ; fi ) && \
	mkdir build && cd build && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DBUILD_SHARED_LIBS:BOOL=OFF \
		-DBUILD_SHARED_LIBS:BOOL=OFF \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/hdf5 \
		-DZLIB_ROOT:PATH=$(PREFIX_ROOT)/zlib \
		-DZLIB_USE_EXTERNAL:BOOL=ON \
		.. > $(PREFIX_ROOT)/log_hdf5.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_hdf5.txt 2>&1 && \
	cd ../.. && \
	rm -rf hdf5-$(HDF5_VERSION) && \
	cd $(THIS_DIR) && \
	echo $(HDF5_VERSION) > $@


# IlmBase
# Edits:
# - using the custom namespaces instead of Imath_2_2, Iex_2_2, IlmThread_2_2
$(ILMBASE_STAMP) : $(CMAKE_STAMP) $(ILMBASE_FILE)
	@echo Building IlmBase $(ILMBASE_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf ilmbase-$(ILMBASE_VERSION) && \
	tar -xf $(SOURCES_ROOT)/ilmbase-$(ILMBASE_VERSION).tar.gz && \
	cd ilmbase-$(ILMBASE_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DBUILD_SHARED_LIBS:BOOL=OFF \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/ilmbase \
		-DNAMESPACE_VERSIONING:BOOL=ON \
		. > $(PREFIX_ROOT)/log_ilmbase.txt 2>&1 && \
	( printf '/IMATH_INTERNAL_NAMESPACE/s/Imath_2_2/rdoImath/\nw\nq' | ed -s config/IlmBaseConfig.h ) && \
	( printf '/IEX_INTERNAL_NAMESPACE/s/Iex_2_2/rdoIex/\nw\nq' | ed -s config/IlmBaseConfig.h ) && \
	( printf '/ILMTHREAD_INTERNAL_NAMESPACE/s/IlmThread_2_2/rdoIlmThread/\nw\nq' | ed -s config/IlmBaseConfig.h ) && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_ilmbase.txt 2>&1 && \
	cd .. && \
	rm -rf ilmbase-$(ILMBASE_VERSION) && \
	cd $(THIS_DIR) && \
	echo $(ILMBASE_VERSION) > $@


# ISPC
$(ISPC) : $(CMAKE_STAMP)
	@echo Building ISPC $(ISPC_VERSION) && \
	$(MAKE) $(MAKE_OPTS) -C ./system \
		BUILD_ROOT=$(BUILD_ROOT) \
		CLANG=$(CLANG) \
		CMAKE=$(CMAKE) \
		CMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		COMMON_CMAKE_FLAGS='$(COMMON_CMAKE_FLAGS)' \
		ISPC=$(ISPC) \
		ISPC_VERSION=$(ISPC_VERSION) \
		LLVM_VERS=$(LLVM_VERS) \
		PREFIX_ROOT=$(PREFIX_ROOT) \
		PYTHON_BIN=$(PYTHON_BIN) \
		SOURCES_ROOT=$(SOURCES_ROOT) \
		ispc -j$(JOB_COUNT) > $(PREFIX_ROOT)/log_ispc.txt 2>&1


# jpeg
# Edits:
# - removed m4_argn which is available in the latest m4
$(JPEG_STAMP) : $(JPEG_FILE)/HEAD
	@echo Building jpeg $(JPEG_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf jpeg && \
	git clone -q --no-checkout $(SOURCES_ROOT)/libjpeg-turbo.git jpeg && \
	cd jpeg && \
	git checkout -q $(JPEG_VERSION) && \
	( printf '/m4_define.*version_major/s/m4_argn(1,version_triplet)/1/\nw\nq' | ed -s configure.ac ) && \
	( printf '/m4_define.*version_minor/s/m4_argn(2,version_triplet)/5/\nw\nq' | ed -s configure.ac ) && \
	( printf '/m4_define.*version_revision/s/m4_argn(3,version_triplet)/1/\nw\nq' | ed -s configure.ac ) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(AUTORECONF) -fiv > $(PREFIX_ROOT)/log_jpeg.txt 2>&1 && \
	$(COMPILER_CONF) \
	./configure \
		--prefix="$(PREFIX_ROOT)/jpeg" \
		--enable-shared=no >> $(PREFIX_ROOT)/log_jpeg.txt 2>&1 && \
	$(MAKE) -j$(JOB_COUNT) \
		MAKE_MODE=$(MAKE_MODE) \
		install >> $(PREFIX_ROOT)/log_jpeg.txt 2>&1 && \
	cd .. && \
	rm -rf jpeg && \
	cd $(THIS_DIR) && \
	echo $(JPEG_VERSION) > $@


# jsoncpp
$(JSONCPP_STAMP) : $(CMAKE_STAMP) $(ZLIB_STAMP) $(JSONCPP_FILE)/HEAD
	@echo Building jsoncpp $(JSONCPP_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf jsoncpp && \
	git clone -q --no-checkout $(SOURCES_ROOT)/jsoncpp.git jsoncpp && \
	cd jsoncpp && \
	git checkout -q $(JSONCPP_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/jsoncpp \
		-DZLIB_ROOT:PATH=$(PREFIX_ROOT)/zlib \
		. > $(PREFIX_ROOT)/log_jsoncpp.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_jsoncpp.txt 2>&1 && \
	cd .. && \
	rm -rf jsoncpp && \
	cd $(THIS_DIR) && \
	echo $(JSONCPP_VERSION) > $@


# LLVM, clang
$(CLANG) : $(CMAKE_STAMP) $(PYTHON_STAMP)
	@echo Building clang $(LLVM_VERS) && \
	$(MAKE) $(MAKE_OPTS) -C ./system \
		$(COMPILER_CONF) \
		BUILD_ROOT=$(BUILD_ROOT) \
		CLANG=$(CLANG) \
		CMAKE=$(CMAKE) \
		CMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		COMMON_CMAKE_FLAGS='$(COMMON_CMAKE_FLAGS)' \
		LLVM_VERS=$(LLVM_VERS) \
		PREFIX_ROOT=$(PREFIX_ROOT) \
		PYTHON_BIN=$(PYTHON_BIN) \
		SOURCES_ROOT=$(SOURCES_ROOT) \
		llvm > $(PREFIX_ROOT)/log_llvm.txt 2>&1


# Mercurial
$(MERCURIAL_STAMP) : $(MERCURIAL_FILE) $(PYTHON_STAMP)
	@echo Building mercurial $(MERCURIAL_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(basename $(MERCURIAL_FILE)))) && \
	tar zxf $(SOURCES_ROOT)/$(notdir $(MERCURIAL_FILE)) && \
	cd $(notdir $(basename $(basename $(MERCURIAL_FILE)))) && \
	( printf "0c\n#!$(PYTHON_BIN)\n.\nw\nq" | ed -s hg ) && \
	$(MAKE) $(COMPILER_CONF) -j$(JOB_COUNT) \
		MAKE_MODE=$(MAKE_MODE) \
		PYTHON=$(PYTHON_BIN) \
		PREFIX=$(PREFIX_ROOT)/mercurial \
		install-bin >> $(PREFIX_ROOT)/log_mercurial.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(basename $(MERCURIAL_FILE)))) && \
	cd $(THIS_DIR) && \
	echo $(MERCURIAL_VERSION) > $@


# OpenColorIO
$(OCIO_STAMP) : $(BOOST_STAMP) $(CMAKE_STAMP) $(OCIO_FILE)/HEAD
	@echo Building OpenColorIO $(OCIO_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf ocio && \
	git clone -q --no-checkout $(SOURCES_ROOT)/OpenColorIO.git ocio && \
	cd ocio && \
	git checkout -q $(OCIO_VERSION) && \
	mkdir build && cd build && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DBOOST_ROOT=$(PREFIX_ROOT)/boost \
		-DBoost_NAMESPACE=$(BOOST_NAMESPACE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/ocio \
		-DOCIO_BUILD_APPS:BOOL=OFF \
		-DOCIO_BUILD_PYGLUE:BOOL=OFF \
		-DOCIO_BUILD_SHARED:BOOL=OFF \
		-DOCIO_USE_BOOST_PTR:BOOL=ON \
		.. > $(PREFIX_ROOT)/log_ocio.txt 2>&1 && \
	PATH=$(dir $(CXX)):$(PATH) \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_ocio.txt 2>&1 && \
	cp ext/dist/lib/lib* $(PREFIX_ROOT)/ocio/lib && \
	cd ../.. && \
	rm -rf ocio && \
	cd $(THIS_DIR) && \
	echo $(OCIO_VERSION) > $@


# OpenImageIO
# Edits:
# - Defining OIIO_STATIC_BUILD to avoid specifying it everywhere
# - Linking with tinyxml and yaml-cpp
# - std::locale segfault fix
# - Python module
$(OIIO_STAMP) : $(BOOST_STAMP) $(CMAKE_STAMP) $(GLEW_STAMP) $(ILMBASE_STAMP) $(JPEG_STAMP) $(OCIO_STAMP) $(OPENEXR_STAMP) $(PNG_STAMP) $(PTEX_STAMP) $(QT5BASE_STAMP) $(TIFF_STAMP) $(ZLIB_STAMP) $(OIIO_FILE)/HEAD
	@echo Building OpenImageIO $(OIIO_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf oiio && \
	git clone -q --no-checkout $(SOURCES_ROOT)/oiio.git oiio && \
	cd oiio && \
	git checkout -q $(OIIO_VERSION) && \
	echo Patch to avoid defininig OIIO_STATIC_BUILD in all the packages... && \
	( printf '\n#ifndef OIIO_STATIC_BUILD\n# define OIIO_STATIC_BUILD\n#endif\n' >> src/include/OpenImageIO/export.h ) && \
	echo Patch to get tinyxml and yaml from OCIO... && \
	( printf '/find_library.*tinyxml/s/)/ PATHS $$\{OCIO_PATH\}\/lib)/\nw\nq' | ed -s src/cmake/externalpackages.cmake ) && \
	( printf '/find_library.*yaml-cpp/s/)/ PATHS $$\{OCIO_PATH\}\/lib)/\nw\nq' | ed -s src/cmake/externalpackages.cmake ) && \
	( printf '/USE_PYTHON OFF/d\nw\nq' | ed -s src/cmake/compiler.cmake ) && \
	( printf '/Boost_USE_STATIC_LIBS/d\nw\nq' | ed -s src/cmake/compiler.cmake ) && \
	( printf '/Boost_USE_STATIC_LIBS/d\nw\nq' | ed -s src/cmake/compiler.cmake ) && \
	( printf '/Boost_USE_STATIC_LIBS/d\nw\nq' | ed -s src/cmake/externalpackages.cmake ) && \
	( printf '/CMAKE_FIND_LIBRARY_SUFFIXES .a/d\nw\nq' | ed -s src/cmake/compiler.cmake ) && \
	echo Patch to link iv with static qt5... && \
	( printf '/Qt5::Core/a\nlibxcb.so\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\nlibxcb-glx.so\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\nlibX11-xcb.so\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\n\044{X11_Xi_LIB}\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\n\044{X11_LIBRARIES}\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\n$(PREFIX_ROOT)/qt5base/lib/libxcb-static.a\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\n$(PREFIX_ROOT)/qt5base/lib/libqtpcre2.a\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\n$(PREFIX_ROOT)/qt5base/lib/libqtharfbuzz.a\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\n$(PREFIX_ROOT)/qt5base/lib/libqtfreetype.a\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\n$(PREFIX_ROOT)/qt5base/lib/libQt5GlxSupport.a\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\n$(PREFIX_ROOT)/qt5base/lib/libQt5FontDatabaseSupport.a\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\n$(PREFIX_ROOT)/qt5base/lib/libQt5ServiceSupport.a\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\n$(PREFIX_ROOT)/qt5base/lib/libQt5ThemeSupport.a\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\n$(PREFIX_ROOT)/qt5base/lib/libQt5EventDispatcherSupport.a\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\n$(PREFIX_ROOT)/qt5base/lib/libQt5XcbQpa.a\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\nQt5::QXcbGlxIntegrationPlugin\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5::Core/a\nQt5::QXcbIntegrationPlugin\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '/Qt5_FOUND/a\nfind_package(X11 REQUIRED)\n.\nw\nq' | ed -s src/iv/CMakeLists.txt ) && \
	( printf '\n#include <QtPlugin>\nQ_IMPORT_PLUGIN(QXcbIntegrationPlugin)' >> src/iv/ivmain.cpp ) && \
	( printf '\nQ_IMPORT_PLUGIN(QXcbGlxIntegrationPlugin)' >> src/iv/ivmain.cpp ) && \
	mkdir build && cd build && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DBOOST_ROOT=$(PREFIX_ROOT)/boost \
		-DBUILDSTATIC:BOOL=ON \
		-DBoost_NAMESPACE=$(BOOST_NAMESPACE) \
		-DBoost_USE_STATIC_LIBS:BOOL=$(USE_STATIC_BOOST) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/oiio \
		-DGLEW_INCLUDES:PATH=$(PREFIX_ROOT)/glew/include/GL \
		-DGLEW_LIBRARIES:PATH=$(PREFIX_ROOT)/glew/lib/libGLEW.a \
		-DILMBASE_HOME=$(PREFIX_ROOT)/ilmbase \
		-DJPEGTURBO_PATH=$(PREFIX_ROOT)/jpeg \
		-DLINKSTATIC:BOOL=ON \
		-DOCIO_PATH=$(PREFIX_ROOT)/ocio \
		-DOIIO_BUILD_TESTS:BOOL=OFF \
		-DOPENEXR_HOME=$(PREFIX_ROOT)/openexr \
		-DPNG_LIBRARY=$(PREFIX_ROOT)/png/lib/libpng.a \
		-DPNG_PNG_INCLUDE_DIR=$(PREFIX_ROOT)/png/include \
		-DPTEX_LOCATION:PATH=$(PREFIX_ROOT)/ptex \
		-DPYTHON_EXECUTABLE=$(PYTHON_BIN) \
		-DTIFF_INCLUDE_DIR=$(PREFIX_ROOT)/tiff/include \
		-DTIFF_LIBRARY=$(PREFIX_ROOT)/tiff/lib/libtiff.a \
		-DUSE_FFMPEG:BOOL=OFF \
		-DUSE_FREETYPE:BOOL=OFF \
		-DUSE_GIF:BOOL=OFF \
		-DQt5_DIR:PATH=$(PREFIX_ROOT)/qt5base/lib/cmake/Qt5 \
		-DVERBOSE:BOOL=ON \
		-DZLIB_ROOT=$(PREFIX_ROOT)/zlib \
		.. > $(PREFIX_ROOT)/log_oiio.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_oiio.txt 2>&1 && \
	cd ../.. && \
	rm -rf oiio && \
	cd $(THIS_DIR) && \
	echo $(OIIO_VERSION) > $@


# OpenEXR
# Edits:
# - using the custom namespaces instead of Imf_2_2
# Patch:
# - Fixed linking problem of dwaLookups
$(OPENEXR_STAMP) : $(CMAKE_STAMP) $(ILMBASE_STAMP) $(ZLIB_STAMP) $(OPENEXR_FILE)
	@echo Building OpenEXR $(OPENEXR_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf openexr-$(OPENEXR_VERSION) && \
	tar -xf $(SOURCES_ROOT)/openexr-$(OPENEXR_VERSION).tar.gz && \
	cd openexr-$(OPENEXR_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DBUILD_SHARED_LIBS:BOOL=OFF \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/openexr \
		-DILMBASE_PACKAGE_PREFIX:PATH=$(PREFIX_ROOT)/ilmbase \
		-DNAMESPACE_VERSIONING:BOOL=ON \
		-DZLIB_ROOT:PATH=$(PREFIX_ROOT)/zlib \
		. > $(PREFIX_ROOT)/log_openexr.txt 2>&1 && \
	( printf '/OPENEXR_IMF_INTERNAL_NAMESPACE Imf_2_2/s/Imf_2_2/rdoImf/\nw\nq' | ed -s config/OpenEXRConfig.h ) && \
	( printf '/define.*INCLUDED_IMF_DWA_COMRESSOR_H/a\n#include <zlib.h>\n.\nw\nq' | ed -s IlmImf/ImfDwaCompressor.h ) && \
	( printf '/define.*INCLUDED_IMF_ZIP_COMPRESSOR_H/a\n#include <zlib.h>\n.\nw\nq' | ed -s IlmImf/ImfZipCompressor.h ) && \
	( printf '/define.*INCLUDED_IMF_COMPRESSION_H/a\n#include <zlib.h>\n.\nw\nq' | ed -s IlmImf/ImfCompression.h ) && \
	( printf '/define.*INCLUDED_IMF_ZIP_H/a\n#include <zlib.h>\n.\nw\nq' | ed -s IlmImf/ImfZip.h ) && \
	patch -N --no-backup-if-mismatch IlmImf/CMakeLists.txt $(THIS_DIR)/patches/OpenEXR/patch_openexr_cmakelists.diff && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_openexr.txt 2>&1 && \
	cd .. && \
	rm -rf openexr-$(OPENEXR_VERSION) && \
	cp $(PREFIX_ROOT)/ilmbase/lib/*.a $(PREFIX_ROOT)/openexr/lib && \
	cd $(THIS_DIR) && \
	echo $(OPENEXR_VERSION) > $@


# OpenSubdiv
$(OPENSUBD_STAMP) : $(CMAKE_STAMP) $(GLEW_STAMP) $(GLFW_STAMP) $(PTEX_STAMP) $(PYTHON_STAMP) $(TBB_STAMP) $(ZLIB_STAMP) $(OPENSUBD_FILE)/HEAD
	@echo Building OpenSubdiv $(OPENSUBD_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(OPENSUBD_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(OPENSUBD_FILE)) $(notdir $(basename $(OPENSUBD_FILE))) && \
	cd $(notdir $(basename $(OPENSUBD_FILE))) && \
	git checkout -q $(OPENSUBD_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	( printf "/osd_dynamic_cpu/s/osd_dynamic_cpu/osd_static_gpu/\nw\nq" | ed -s CMakeLists.txt ) && \
	( printf "/osd_dynamic_gpu/s/osd_dynamic_gpu/osd_static_cpu/\nw\nq" | ed -s CMakeLists.txt ) && \
	( printf "/if.*NOT.*NOT/s/(/( 0 AND /\nw\nq" | ed -s opensubdiv/CMakeLists.txt ) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/opensubdiv \
		-DGLFW_LOCATION:PATH=$(PREFIX_ROOT)/glfw \
		-DGLEW_LOCATION:PATH=$(PREFIX_ROOT)/glew \
		-DNO_GLTESTS:BOOL=ON \
		-DNO_TESTS:BOOL=ON \
		-DNO_TUTORIALS:BOOL=ON \
		-DPTEX_LOCATION:PATH=$(PREFIX_ROOT)/ptex \
		-DPYTHON_EXECUTABLE=$(PYTHON_BIN) \
		-DTBB_LOCATION:PATH=$(PREFIX_ROOT)/tbb \
		-DZLIB_ROOT:PATH=$(PREFIX_ROOT)/zlib \
		-DNO_OMP=1 \
		. > $(PREFIX_ROOT)/log_opensubdiv.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_opensubdiv.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(OPENSUBD_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(OPENSUBD_VERSION) > $@


# OpenVDB
$(OPENVDB_STAMP) : $(BLOSC_STAMP) $(BOOST_STAMP) $(CMAKE_STAMP) $(GLFW_STAMP) $(OPENEXR_STAMP) $(TBB_STAMP) $(OPENVDB_FILE)/HEAD
	@echo Building OpenVDB $(OPENVDB_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(OPENVDB_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(OPENVDB_FILE)) $(notdir $(basename $(OPENVDB_FILE))) && \
	cd $(notdir $(basename $(OPENVDB_FILE))) && \
	git checkout -q $(OPENVDB_VERSION) && \
	( printf "/define/a\n#define OPENVDB_STATICLIB\n.\nw\n" | ed -s openvdb/PlatformConfig.h ) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DBLOSC_LOCATION=$(PREFIX_ROOT)/blosc \
		-DBOOST_ROOT=$(PREFIX_ROOT)/boost \
		-DBlosc_USE_STATIC_LIBS=ON \
		-DBoost_NAMESPACE=$(BOOST_NAMESPACE) \
		-DBoost_USE_STATIC_LIBS:BOOL=$(USE_STATIC_BOOST) \
		-DCMAKE_CXX_FLAGS="$(FLAGS) -fPIC -std=c++11" \
		-DCMAKE_EXE_LINKER_FLAGS="-ldl -pthread" \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/openvdb \
		-DCMAKE_SHARED_LINKER_FLAGS="-Wl,--no-undefined -ldl -pthread" \
		-DGLFW3_LOCATION=$(PREFIX_ROOT)/glfw \
		-DGLFW3_USE_STATIC_LIBS:BOOL=ON \
		-DILMBASE_LOCATION:PATH=$(PREFIX_ROOT)/ilmbase \
		-DOPENEXR_LOCATION:PATH=$(PREFIX_ROOT)/openexr \
		-DOPENVDB_BUILD_PYTHON_MODULE:BOOL=OFF \
		-DOPENVDB_BUILD_UNITTESTS:BOOL=OFF \
		-DOPENVDB_ENABLE_3_ABI_COMPATIBLE:BOOL=OFF \
		-DTBB_LOCATION:PATH=$(PREFIX_ROOT)/tbb \
		-DUSE_GLFW3:BOOL=ON \
		-DZLIB_ROOT:PATH=$(PREFIX_ROOT)/zlib \
		. > $(PREFIX_ROOT)/log_openvdb.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_openvdb.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(OPENVDB_FILE))) && \
	rm $(PREFIX_ROOT)/openvdb/lib/*.so* && \
	cd $(THIS_DIR) && \
	echo $(OPENVDB_VERSION) > $@


# Open Shading Language
OSL_ADDITIONAL_LIBS := \
	$(PREFIX_ROOT)/boost/lib/lib$(BOOST_NAMESPACE)_filesystem$(DYNAMIC_EXT) \
	$(PREFIX_ROOT)/openexr/lib/libIlmImf-2_2.a \
	$(PREFIX_ROOT)/openexr/lib/libImath-2_2.a \
	$(PREFIX_ROOT)/openexr/lib/libIex-2_2.a \
	$(PREFIX_ROOT)/openexr/lib/libHalf.a \
	$(PREFIX_ROOT)/openexr/lib/libIlmThread-2_2.a \
	$(PREFIX_ROOT)/png/lib/libpng.a \
	$(PREFIX_ROOT)/jpeg/lib/libjpeg.a \
	$(PREFIX_ROOT)/ptex/lib/libPtex.a \
	$(PREFIX_ROOT)/tiff/lib/libtiff.a \
	$(PREFIX_ROOT)/zlib/lib/libz.a


$(OSL_STAMP) : $(BISON) $(BOOST_STAMP) $(CLANG) $(CMAKE_STAMP) $(FLEX) $(OSL_FILE)/HEAD
	@echo Building OSL $(OSL_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(OSL_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(OSL_FILE)) $(notdir $(basename $(OSL_FILE))) && \
	cd $(notdir $(basename $(OSL_FILE))) && \
	git checkout -q $(OSL_VERSION) && \
	( printf '/VERSION_LESS/s/VERSION_LESS.*)/VERSION_LESS 3.8.0)/\nw\nq' | ed -s src/cmake/modules/FindLLVM.cmake ) && \
	( printf '/LLVM.*REQUIRED/s/LLVM.*REQUIRED/LLVM REQUIRED/\nw\nq' | ed -s src/cmake/externalpackages.cmake ) && \
	( printf '/oslexec/s/oslexec/oslexec oslcomp/\nw\nq' | ed -s src/osl.imageio/CMakeLists.txt ) && \
	( for f in $(OSL_ADDITIONAL_LIBS); do ( printf "/OPENIMAGEIO_LIBRARY_DIRS.*OPENIMAGEIO_LIBRARY/a\nlist(APPEND OPENIMAGEIO_LIBRARY $$f)\n.\nw\nq" | ed -s src/cmake/modules/FindOpenImageIO.cmake ); done ) && \
	mkdir -p build && cd build && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DBISON_EXECUTABLE=$(BISON) \
		-DBOOST_ROOT=$(PREFIX_ROOT)/boost \
		-DBUILDSTATIC:BOOL=ON \
		-DBoost_NAMESPACE=$(BOOST_NAMESPACE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/osl \
		-DFLEX_EXECUTABLE=$(FLEX) \
		-DILMBASE_HOME=$(PREFIX_ROOT)/ilmbase \
		-DLLVM_DIRECTORY=$(PREFIX_ROOT)/llvm \
		-DLLVM_STATIC:BOOL=ON \
		-DUSE_SIMD:BOOL=sse4.2 \
		-DOPENEXR_HOME=$(PREFIX_ROOT)/openexr \
		-DOPENIMAGEIOHOME=$(PREFIX_ROOT)/oiio \
		-DOSL_BUILD_TESTS:BOOL=OFF \
		.. > $(PREFIX_ROOT)/log_osl.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_osl.txt 2>&1 && \
	cd ../.. && \
	rm -rf $(notdir $(basename $(OSL_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(OSL_VERSION) > $@


# png
$(PNG_STAMP) : $(CMAKE_STAMP) $(ZLIB_STAMP) $(PNG_FILE)
	@echo Building png $(PNG_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(basename $(PNG_FILE)))) && \
	tar zxf $(SOURCES_ROOT)/$(notdir $(PNG_FILE)) && \
	cd $(notdir $(basename $(basename $(PNG_FILE)))) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DPNG_SHARED:BOOL=OFF \
		-DPNG_PREFIX=rdoPNG \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/png \
		-DZLIB_ROOT:PATH=$(PREFIX_ROOT)/zlib \
		. > $(PREFIX_ROOT)/log_png.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_png.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(basename $(PNG_FILE)))) && \
	cd $(THIS_DIR) && \
	echo $(PNG_VERSION) > $@


# Ptex
$(PTEX_STAMP) : $(CMAKE_STAMP) $(ZLIB_STAMP) $(PTEX_FILE)/HEAD
	@echo Building Ptex $(PTEX_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(PTEX_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(PTEX_FILE)) $(notdir $(basename $(PTEX_FILE))) && \
	cd $(notdir $(basename $(PTEX_FILE))) && \
	git checkout -q $(PTEX_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/ptex \
		-DZLIB_ROOT:PATH=$(PREFIX_ROOT)/zlib \
		. > $(PREFIX_ROOT)/log_ptex.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_ptex.txt 2>&1 && \
	( ls -d -1 $(PREFIX_ROOT)/ptex/lib/* | grep -v -F .a | xargs rm ) && \
	cd .. && \
	rm -rf $(notdir $(basename $(PTEX_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(PTEX_VERSION) > $@


# pyilmbase
$(PYILMBASE_STAMP) : $(BOOST_STAMP) $(CMAKE_STAMP) $(ILMBASE_STAMP) $(PYTHON_STAMP) $(PYILMBASE_FILE)/HEAD
	@echo Building pyilmbase $(PYILMBASE_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(PYILMBASE_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(PYILMBASE_FILE)) $(notdir $(basename $(PYILMBASE_FILE))) && \
	cd $(notdir $(basename $(PYILMBASE_FILE))) && \
	git checkout -q $(PYILMBASE_VERSION) && \
	cd PyIlmBase && \
	( printf "/INCLUDE_DIRECTORIES/a\n\044{Boost_INCLUDE_DIR}\n.\nw\nq" | ed -s CMakeLists.txt ) && \
	( printf "/FIND_PACKAGE.*Boost/a\nREQUIRED COMPONENTS python\n.\nw\nq" | ed -s CMakeLists.txt ) && \
	( printf "\044a\nfile(GLOB MYHEADERS *.h)\ninstall(FILES \044{MYHEADERS} DESTINATION include/OpenEXR)\n.\nw\nq" | ed -s PyImath/CMakeLists.txt ) && \
	( for i in `grep --include=\*.{cpp,h} -rl . -e "^namespace boost "`; do echo $$i; ( printf "/^namespace boost/s/namespace boost/namespace $(BOOST_NAMESPACE)/\nw\nq" | ed -s $$i ); done ) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DBOOST_ROOT:PATH=$(PREFIX_ROOT)/boost \
		-DBoost_NAMESPACE=$(BOOST_NAMESPACE) \
		-DBoost_USE_STATIC_LIBS:BOOL=$(USE_STATIC_BOOST) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/pyilmbase \
		-DCMAKE_INSTALL_RPATH=$(PREFIX_ROOT)/pyilmbase/lib \
		-DILMBASE_PACKAGE_PREFIX=$(PREFIX_ROOT)/ilmbase \
		-DPYTHON_EXECUTABLE=$(PYTHON_BIN) \
		. > $(PREFIX_ROOT)/log_pyilmbase.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_pyilmbase.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(PYILMBASE_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(PYILMBASE_VERSION) > $@


# PyOpenGL
$(PYOPENGL_STAMP) : $(PYTHON_STAMP) $(PYOPENGL_FILE)
	@echo Building PyOpenGL $(PYOPENGL_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(basename $(PYOPENGL_FILE)))) && \
	tar -xf $(SOURCES_ROOT)/$(notdir $(PYOPENGL_FILE)) && \
	cd $(notdir $(basename $(basename $(PYOPENGL_FILE)))) && \
	$(PYTHON_BIN) setup.py install --prefix=$(PREFIX_ROOT)/pyopengl > $(PREFIX_ROOT)/log_pyopengl.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(basename $(PYOPENGL_FILE)))) && \
	cd $(THIS_DIR) && \
	echo $(PYOPENGL_VERSION) > $@


# PySide
$(PYSIDE_STAMP) : $(CMAKE_STAMP) $(PYTHON_STAMP) $(QT_STAMP) $(SHIBOKEN_STAMP) $(PYSIDE_FILE)/HEAD
	@echo Building PySide $(PYSIDE_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(PYSIDE_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(PYSIDE_FILE)) $(notdir $(basename $(PYSIDE_FILE))) && \
	cd $(notdir $(basename $(PYSIDE_FILE))) && \
	git checkout -q $(PYSIDE_VERSION) && \
	mkdir -p build && cd build && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DBUILD_TESTS:BOOL=OFF \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/pyside \
		-DCMAKE_INSTALL_RPATH=$(PREFIX_ROOT)/pyside/lib \
		-DPYTHON_EXECUTABLE=$(PYTHON_BIN) \
		-DQT_QMAKE_EXECUTABLE:PATH=$(PREFIX_ROOT)/qt/bin/qmake \
		-DShiboken_DIR=$(PREFIX_ROOT)/shiboken/lib/cmake/Shiboken-$(SHIBOKEN_VERSION) \
		.. > $(PREFIX_ROOT)/log_pyside.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_pyside.txt 2>&1 && \
	cd ../.. && \
	rm -rf $(notdir $(basename $(PYSIDE_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(PYSIDE_VERSION) > $@


# PySide Tools
$(PYSIDETOOLS_STAMP) : $(CMAKE_STAMP) $(PYSIDE_STAMP) $(PYTHON_STAMP) $(QT_STAMP) $(SHIBOKEN_STAMP) $(PYSIDETOOLS_FILE)/HEAD
	@echo Building PySide Tools $(PYSIDETOOLS_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(PYSIDETOOLS_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(PYSIDETOOLS_FILE)) $(notdir $(basename $(PYSIDETOOLS_FILE))) && \
	cd $(notdir $(basename $(PYSIDETOOLS_FILE))) && \
	git checkout -q $(PYSIDETOOLS_VERSION) && \
	mkdir -p build && cd build && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DBUILD_TESTS:BOOL=OFF \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/pyside \
		-DPySide_DIR=$(PREFIX_ROOT)/pyside/lib/cmake/PySide-$(PYSIDE_VERSION) \
		-DQT_QMAKE_EXECUTABLE:PATH=$(PREFIX_ROOT)/qt/bin/qmake \
		-DShiboken_DIR=$(PREFIX_ROOT)/shiboken/lib/cmake/Shiboken-$(SHIBOKEN_VERSION) \
		.. > $(PREFIX_ROOT)/log_pysidetools.txt 2>&1 && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_pysidetools.txt 2>&1 && \
	cd ../.. && \
	rm -rf $(notdir $(basename $(PYSIDETOOLS_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(PYSIDETOOLS_VERSION) > $@


# pystring
$(PYSTRING_STAMP) : $(PYSTRING_FILE)/HEAD
	@echo Building pystring $(PYSTRING_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf pystring && \
	git clone -q --no-checkout $(SOURCES_ROOT)/pystring.git pystring && \
	cd pystring && \
	git checkout -q $(PYSTRING_VERSION) && \
	mkdir -p $(PREFIX_ROOT)/pystring/lib && \
	mkdir -p $(PREFIX_ROOT)/pystring/include && \
	PATH=$(dir $(CXX)):$(PATH) \
	$(MAKE) $(COMPILER_CONF) -j$(JOB_COUNT) \
		MAKE_MODE=$(MAKE_MODE) \
		LIBDIR=$(PREFIX_ROOT)/pystring/lib \
		LIBTOOL=$(LIBTOOL) \
		install > $(PREFIX_ROOT)/log_pystring.txt 2>&1 && \
	cp pystring.h $(PREFIX_ROOT)/pystring/include && \
	( ls -d -1 $(PREFIX_ROOT)/pystring/lib/* | grep -v -F .a | xargs rm ) && \
	cd .. && \
	rm -rf pystring && \
	cd $(THIS_DIR) && \
	echo $(PYSTRING_VERSION) > $@


# Python
# We have to run make && make install because target install fails in
# multithreaded mode in Python 2.6.6
# Edits:
# - Added RPATH, it allows to run python without setting LD_LIBRARY_PATH
ifeq "$(MAKE_MODE)" "debug"
PYTHON_EXTRA_OPT := --with-pydebug
endif
ifeq "$(OS)" "Darwin"
PYTHON_EXTRA_OPT += --enable-unicode=ucs2
endif
ifeq "$(OS)" "linux"
PYTHON_EXTRA_OPT += --enable-unicode=ucs4 --enable-shared
endif
$(PYTHON_STAMP) : $(PYTHON_FILE)
	@echo Building python $(PYTHON_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf Python-$(PYTHON_VERSION) && \
	tar zxf $(SOURCES_ROOT)/Python-$(PYTHON_VERSION).tgz && \
	cd Python-$(PYTHON_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(COMPILER_CONF) \
	./configure \
		LDFLAGS=-Wl,-rpath,\''$$''$$'\'ORIGIN/../lib \
		--prefix="$(PREFIX_ROOT)/python" \
		$(PYTHON_EXTRA_OPT) > $(PREFIX_ROOT)/log_python.txt 2>&1 && \
	$(MAKE) -j$(JOB_COUNT) \
		MAKE_MODE=$(MAKE_MODE) >> $(PREFIX_ROOT)/log_python.txt 2>&1 && \
	$(MAKE) \
		MAKE_MODE=$(MAKE_MODE) \
		install >> $(PREFIX_ROOT)/log_python.txt 2>&1 && \
	cd .. && \
	rm -rf Python-$(PYTHON_VERSION) && \
	cd $(THIS_DIR) && \
	echo $(PYTHON_VERSION) > $@


# Qt
$(QT_STAMP) : $(QT_FILE)
	@echo Building Qt $(QT_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(basename $(QT_FILE)))) && \
	tar zxf $(SOURCES_ROOT)/$(notdir $(QT_FILE)) && \
	cd $(notdir $(basename $(basename $(QT_FILE)))) && \
	echo QMAKE_CXX = $(CXX) >> mkspecs/linux-g++/qmake.conf && \
	echo QMAKE_LINK = $(CXX) >> mkspecs/linux-g++/qmake.conf && \
	echo QMAKE_CC = $(CC) >> mkspecs/linux-g++/qmake.conf && \
	echo QMAKE_LINK_C = $(CC) >> mkspecs/linux-g++/qmake.conf && \
	mkdir -p $(PREFIX_ROOT) && \
	$(COMPILER_CONF) \
	./configure \
		-confirm-license \
		-no-multimedia \
		-no-webkit \
		-nomake demos \
		-nomake docs \
		-nomake examples \
		-nomake tests \
		-opensource \
		-prefix $(PREFIX_ROOT)/qt \
		-shared > $(PREFIX_ROOT)/log_qt.txt 2>&1 && \
	$(MAKE) -j$(JOB_COUNT) \
		MAKE_MODE=$(MAKE_MODE) \
		install >> $(PREFIX_ROOT)/log_qt.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(basename $(QT_FILE)))) && \
	cd $(THIS_DIR) && \
	echo $(QT_VERSION) > $@


# Qt 5
$(QT5BASE_STAMP) : $(QT5BASE_FILE)/HEAD
	@echo Building Qt $(QT5BASE_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(QT5BASE_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(QT5BASE_FILE)) $(notdir $(basename $(QT5BASE_FILE))) && \
	cd $(notdir $(basename $(QT5BASE_FILE))) && \
	git checkout -q $(QT5BASE_VERSION) && \
	echo QMAKE_CXX = $(CXX) >> mkspecs/linux-g++/qmake.conf && \
	echo QMAKE_LINK = $(CXX) >> mkspecs/linux-g++/qmake.conf && \
	echo QMAKE_CC = $(CC) >> mkspecs/linux-g++/qmake.conf && \
	echo QMAKE_LINK_C = $(CC) >> mkspecs/linux-g++/qmake.conf && \
	echo Patching Qt FreeType to don\'t use png... && \
	( printf '/FT_CONFIG_OPTION_USE_PNG/d\nw\n' | ed -s src/3rdparty/freetype/freetype.pro ) && \
	( printf '/libpng/d\nw\n' | ed -s src/3rdparty/freetype/freetype.pro ) && \
	echo Patching Qt Platform Font Database to use fonts in /usr/share... && \
	( printf '/location.*LibrariesPath.*fonts/c\nfontpath = QLatin1String("/usr/share/fonts");\n.\nw\n' | ed -s src/gui/text/qplatformfontdatabase.cpp ) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(COMPILER_CONF) \
	./configure \
		-confirm-license \
		-no-dbus \
		-no-directfb \
		-no-eglfs \
		-no-gif \
		-no-glib \
		-no-icu \
		-no-libjpeg \
		-no-libpng \
		-no-libproxy \
		-no-linuxfb \
		-no-openssl \
		-no-securetransport \
		-no-sql-mysql \
		-no-sql-sqlite \
		-no-zlib \
		-nomake examples \
		-nomake tests \
		-opengl desktop \
		-opensource \
		-prefix $(PREFIX_ROOT)/qt5base \
		-qt-freetype \
		-qt-harfbuzz \
		-qt-xcb \
		-release \
		-static > $(PREFIX_ROOT)/log_qt5base.txt 2>&1 && \
	$(MAKE) -j$(JOB_COUNT) \
		MAKE_MODE=$(MAKE_MODE) \
		install >> $(PREFIX_ROOT)/log_qt5base.txt 2>&1 && \
	cd .. && \
	rm -rf $(notdir $(basename $(QT5BASE_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(QT5BASE_VERSION) > $@


# Shiboken
$(SHIBOKEN_STAMP) : $(CMAKE_STAMP) $(PYTHON_STAMP) $(QT_STAMP) $(SHIBOKEN_FILE)/HEAD
	@echo Building Shiboken $(SHIBOKEN_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(SHIBOKEN_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(SHIBOKEN_FILE)) $(notdir $(basename $(SHIBOKEN_FILE))) && \
	cd $(notdir $(basename $(SHIBOKEN_FILE))) && \
	git checkout -q $(SHIBOKEN_VERSION) && \
	mkdir -p build && cd build && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DBUILD_TESTS:BOOL=OFF \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/shiboken \
		-DCMAKE_INSTALL_RPATH=$(PREFIX_ROOT)/shiboken/lib \
		-DPYTHON_EXECUTABLE=$(PYTHON_BIN) \
		-DQT_QMAKE_EXECUTABLE:PATH=$(PREFIX_ROOT)/qt/bin/qmake \
		.. > $(PREFIX_ROOT)/log_shiboken.txt 2>&1 && \
	LD_LIBRARY_PATH=$(PREFIX_ROOT)/qt/lib \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_shiboken.txt 2>&1 && \
	cd ../.. && \
	rm -rf $(notdir $(basename $(SHIBOKEN_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(SHIBOKEN_VERSION) > $@


# tbb
# Edits:
# - Ability to use custom compiler, custom flags
# - Fixed bug with recognizing linux
$(TBB_STAMP) : $(TBB_FILE)
	@echo Building tbb $(TBB_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf tbb$(TBB_VERSION) && \
	tar zxf $(SOURCES_ROOT)/$(notdir $(TBB_FILE)) && \
	cd tbb$(TBB_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	( printf "/CPLUS/s/g++/$(subst /,\/,$(CXX))/\nw\nq" | ed -s build/linux.gcc.inc ) && \
	( printf "/CPLUS/s/g++/$(subst /,\/,$(CXX))/\nw\nq" | ed -s build/macos.gcc.inc ) && \
	( printf "/CONLY/s/gcc/$(subst /,\/,$(CC))/\nw\nq" | ed -s build/linux.gcc.inc ) && \
	( printf "/CONLY/s/gcc/$(subst /,\/,$(CC))/\nw\nq" | ed -s build/macos.gcc.inc ) && \
	( printf "\044a\nCPLUS_FLAGS += $(FLAGS)\n.\nw\nq" | ed -s build/linux.gcc.inc ) && \
	( printf "\044a\nCPLUS_FLAGS += $(FLAGS)\n.\nw\nq" | ed -s build/macos.gcc.inc ) && \
	( printf "/ifeq.*OS.*Linux/i\nifeq (\044(OS), linux)\nexport tbb_os=linux\nendif\n.\nw\nq\n" | ed -s build/common.inc ) && \
	$(MAKE) \
		-C src \
		tbb_$(MAKE_MODE) \
		tbbmalloc_$(MAKE_MODE) \
		compiler=gcc \
		-j $(JOB_COUNT) > $(PREFIX_ROOT)/log_tbb.txt 2>&1 && \
	mkdir -p $(PREFIX_ROOT)/tbb/include && \
	cp -R include/tbb $(PREFIX_ROOT)/tbb/include && \
	mkdir -p $(PREFIX_ROOT)/tbb/lib && \
	$(AR) \
		-rsc $(PREFIX_ROOT)/tbb/lib/libtbb.a \
		build/*_$(MAKE_MODE)/*.o >> $(PREFIX_ROOT)/log_tbb.txt 2>&1 && \
	cd .. && \
	rm -rf tbb$(TBB_VERSION) && \
	cd $(THIS_DIR) && \
	echo $(TBB_VERSION) > $@


# libtiff
$(TIFF_STAMP) : $(ZLIB_STAMP) $(TIFF_FILE)
	@echo Building tiff $(TIFF_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf tiff-$(TIFF_VERSION) && \
	tar -xf $(SOURCES_ROOT)/tiff-$(TIFF_VERSION).tar.gz && \
	cd tiff-$(TIFF_VERSION) && \
	( printf "/inflateEnd.*()/d\nw\nq" | ed -s configure ) && \
	( printf "/inflateEnd.*()/d\nw\nq" | ed -s configure ) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(COMPILER_CONF) \
	./configure \
		--prefix=$(PREFIX_ROOT)/tiff \
		--with-zlib-include-dir=$(PREFIX_ROOT)/zlib/include \
		--with-zlib-lib-dir=$(PREFIX_ROOT)/zlib/lib \
		--enable-shared=no > $(PREFIX_ROOT)/log_tiff.txt 2>&1 && \
	$(MAKE) -j$(JOB_COUNT) \
		MAKE_MODE=$(MAKE_MODE) \
		install >> $(PREFIX_ROOT)/log_tiff.txt 2>&1 && \
	cd .. && \
	rm -rf tiff-$(TIFF_VERSION) && \
	cd $(THIS_DIR) && \
	echo $(TIFF_VERSION) > $@


# USD
# Edits:
# - FindOpenEXR: looking for static libHalf, use "-2_2" suffix to find other
#   libraries.
# - headers: use rdoBoost namespace insted of boost.
# - FindOpenImageIO: link with static libraries.
# - Static linking of the usd libraries.
# - Set _ReadPlugInfoObject as a weak function. It allows to override it.
# - Suppress a message that load filed. We shouldn't load anything with static
#   linking.
# - Change a key to store the plugins. It's necessary to use somthing unique
#   since every plugin is in a single application.
# Right now we build dynamic USD because the static linking is not supported.
# Python is executed during building, so we need to provide proper
# LD_LIBRARY_PATH in linux. We don't have to do it on macOS because mac's python
# is linked statically.
OIIO_LIBS := \
	$(PREFIX_ROOT)/openexr/lib/libIlmImf-2_2.a \
	$(PREFIX_ROOT)/openexr/lib/libImath-2_2.a \
	$(PREFIX_ROOT)/openexr/lib/libIex-2_2.a \
	$(PREFIX_ROOT)/openexr/lib/libHalf.a \
	$(PREFIX_ROOT)/openexr/lib/libIlmThread-2_2.a \
	$(PREFIX_ROOT)/png/lib/libpng.a \
	$(PREFIX_ROOT)/jpeg/lib/libjpeg.a \
	$(PREFIX_ROOT)/ptex/lib/libPtex.a \
	$(PREFIX_ROOT)/zlib/lib/libz.a \
	$(PREFIX_ROOT)/tiff/lib/libtiff.a \
	$(PREFIX_ROOT)/boost/lib/lib$(BOOST_NAMESPACE)_filesystem$(DYNAMIC_EXT) \
	$(PREFIX_ROOT)/boost/lib/lib$(BOOST_NAMESPACE)_regex$(DYNAMIC_EXT) \
	$(PREFIX_ROOT)/boost/lib/lib$(BOOST_NAMESPACE)_system$(DYNAMIC_EXT) \
	$(PREFIX_ROOT)/boost/lib/lib$(BOOST_NAMESPACE)_thread$(DYNAMIC_EXT) \
	$(PREFIX_ROOT)/boost/lib/lib$(BOOST_NAMESPACE)_chrono$(DYNAMIC_EXT) \
	$(PREFIX_ROOT)/boost/lib/lib$(BOOST_NAMESPACE)_date_time$(DYNAMIC_EXT) \
	$(PREFIX_ROOT)/boost/lib/lib$(BOOST_NAMESPACE)_atomic$(DYNAMIC_EXT) \
	$(PREFIX_ROOT)/ocio/lib/libOpenColorIO.a \
	$(PREFIX_ROOT)/ocio/lib/libtinyxml.a \
	$(PREFIX_ROOT)/ocio/lib/libyaml-cpp.a

# Additional linux libs.
ifeq "$(OS)" "linux"
OIIO_LIBS += rt Xxf86vm Xrandr
endif

TBB_LIBRARY := $(PREFIX_ROOT)/tbb/lib
TBB_ROOT_DIR := $(PREFIX_ROOT)/tbb/include
ifeq "$(USE_STATIC_BOOST)" "ON"
	BUILD_SHARED_LIBS := OFF
else
	# If BUILD_SHARED_LIBS=ON, we force USD to build a single shared library (PXR_BUILD_MONOLITHIC=ON)
	# This is needed because we link with a static TBB library even in shared mode as including static tbb
	# in multiple usd shared libraries won't work!
	# Using static tbb in the USD plugin is to avoid conflict with other tbb libraries from Maya, Houdini, etc...
	BUILD_SHARED_LIBS := ON
endif

$(USD_STAMP) : $(ALEMBIC_STAMP) $(BOOST_STAMP) $(CMAKE_STAMP) $(DC_STAMP) $(EMBREE_STAMP) $(GLUT_STAMP) $(ILMBASE_STAMP) $(OIIO_STAMP) $(OPENEXR_STAMP) $(OPENSUBD_STAMP) $(PTEX_STAMP) $(PYOPENGL_STAMP) $(PYSIDE_STAMP) $(PYSIDETOOLS_STAMP) $(PYTHON_STAMP) $(TBB_STAMP) $(USD_FILE)/HEAD
	@echo Building usd $(USD_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(USD_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(USD_FILE)) $(notdir $(basename $(USD_FILE))) && \
	cd $(notdir $(basename $(USD_FILE))) && \
	git checkout -q $(USD_VERSION) && \
	echo ">>>" Patching Alembic Reader for the materials support... && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0001-RDO-Added-ability-to-convert-Alembic-materials.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0002-RDO-Added-ability-to-output-Alembic-expressions-to-U.patch ) && \
	( git am $(THIS_DIR)/patches/USD/0003-RDO-Don-t-collapse-the-hierarchy-of-Alembic.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0004-RDO-Output-all-the-material-parameters.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0005-RDO-USD-Alembic-add-userData-namespace.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0006-RDO-Houdini-Ability-to-import-all-the-objects.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0007-RDO-produce-doubleSided-property-in-Alembics.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0008-RDO-always-produce-xforms-in-alembics.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0009-RDO-flip-textures-to-display-in-Hydra-as-in-Arnold.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0010-RDO-Hydra-half-OpenEXR-support.patch ) && \
	( git am $(THIS_DIR)/patches/USD/0011-RDO-Default-Resolver-Abc-Core-Layer-support.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0012-RDO-USD-Alembic-no-time-samples.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0013-RDO-USD-Alembic-Support-for-lights.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0014-RDO-USD-Alembic-using-indexed-UV.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0015-RDO-Integrate-embree-to-OpenGL.patch ) && \
	( git am $(THIS_DIR)/patches/USD/0016-RDO-Added-virtual-method-ArResolver-Clear.patch ) && \
	( test ! $(USE_STATIC_BOOST) == ON || git am $(THIS_DIR)/patches/USD/0017-RDO-Support-for-Arnold-components-in-shaders.patch ) && \
	( git am $(THIS_DIR)/patches/USD/0018-RDO-the-ability-to-reference-a-root.patch ) && \
	( git am $(THIS_DIR)/patches/USD/0019-RDO-USD-AlembicReader-Suppress-warnings.patch ) && \
	( git am $(THIS_DIR)/patches/USD/0020-RDO-hdEngine-v0.8.5-broke-our-hdEmbree-plugin-when-used-.patch ) && \
	( printf "/find_library.*OPENEXR_.*_LIBRARY/a\nNAMES\n\044{OPENEXR_LIB}-2_2\n.\nw\nq" | ed -s cmake/modules/FindOpenEXR.cmake ) && \
	( printf "/PATH_SUFFIXES/-a\n\"\044{OPENEXR_BASE_DIR}\"\n.\nw\nq" | ed -s cmake/modules/FindOpenEXR.cmake ) && \
	( printf "/^namespace boost/s/namespace boost/namespace ${BOOST_NAMESPACE}/\nw\nq" | ed -s pxr/base/lib/tf/weakPtrFacade.h ) && \
	( printf "/namespace boost/s/namespace boost/namespace ${BOOST_NAMESPACE}/\nw\nq" | ed -s third_party/houdini/lib/gusd/UT_Gf.h ) && \
	( printf "/^namespace boost/s/namespace boost/namespace ${BOOST_NAMESPACE}/\nw\nq" | ed -s pxr/usd/lib/sdf/pySpec.h ) && \
	( for i in `grep --include=\*.{cpp,h} -rl . -e "^namespace boost "`; do echo $$i; ( printf "/^namespace boost/s/namespace boost/namespace $(BOOST_NAMESPACE)/\nw\nq" | ed -s $$i ); done ) && \
	( printf "/MAYA_LIBRARIES/a\n\044{X11_LIBRARIES}\n.\nw\nq" | ed -s third_party/maya/lib/pxrUsdMayaGL/CMakeLists.txt ) && \
	( printf "/MAYA_INCLUDE_DIRS/a\n\044{GLUT_INCLUDE_DIR}\n.\nw\nq" | ed -s third_party/maya/lib/pxrUsdMayaGL/CMakeLists.txt ) && \
	( printf "/OPENGL_gl_LIBRARY/a\n\044{X11_LIBRARIES}\n.\nw\nq" | ed -s pxr/imaging/lib/garch/CMakeLists.txt ) && \
	( printf "/Unresolved_external_symbol_error_is_expected_Please_ignore/d\ni\nint Unresolved_external_symbol_error_is_expected_Please_ignore()\n{return 0;}\n.\nw\nq" | ed -s pxr/base/lib/plug/testenv/TestPlugDsoUnloadable.cpp ) && \
	echo ">>>" Patching HDF5 support... && \
	( printf "/HDF5/\n/COMPONENTS/\nd\nd\nd\nw\nq" | ed -s cmake/defaults/Packages.cmake ) && \
	echo ">>>" Patching Maya finder... && \
	( printf "/UNIX/\n/HINTS/\n-a\nPATH_SUFFIXES devkit\n.\nw\nq" | ed -s cmake/modules/FindMaya.cmake ) && \
	echo ">>>" Adding static libraries to OIIO... && \
	( for f in $(OIIO_LIBS); do ( printf "\044a\nlist(APPEND OIIO_LIBRARIES $$f)\n.\nw\nq" | ed -s cmake/modules/FindOpenImageIO.cmake ); done ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/pxr_shared_library/\n/SHARED/s/SHARED/STATIC/\nw\nq" | ed -s cmake/macros/Public.cmake ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/pxr_plugin/\n/SHARED/s/SHARED/STATIC/\nw\nq" | ed -s cmake/macros/Public.cmake ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/_ReadPlugInfoObject/-2a\nextern \"C\" __attribute__((weak))\n.\nw\nq" | ed -s pxr/base/lib/plug/info.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/PlugPlugin::_Load/\n/_handle/+1a\nif(0)\n.\nw\nq" | ed -s pxr/base/lib/plug/plugin.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/AppendPathList.*buildLocation/d\nw\nq" | ed -s pxr/base/lib/plug/initConfig.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/AppendPathList.*pluginBuildLocation/d\nw\nq" | ed -s pxr/base/lib/plug/initConfig.cpp ) && \
	echo ">>>" Patching Alembic Reader for the AbcCoreLayer support... && \
	( printf "/PXR_NAMESPACE_OPEN_SCOPE/-a\n#include <Alembic/AbcCoreFactory/IFactory.h>\n#include <boost/algorithm/string.hpp>\n.\nw\nq" | ed -s pxr/usd/plugin/usdAbc/alembicReader.cpp ) && \
	( printf "/_ReaderContext::_OpenOgawa/\n/{/a\nstd::vector<std::string> archives;\nboost::split(archives, filePath, boost::is_any_of(\":\"));\n.\nw\nq" | ed -s pxr/usd/plugin/usdAbc/alembicReader.cpp ) && \
	( printf "/_ReaderContext::_OpenOgawa/\n/\"Ogawa\"/a\nif (archives.size()>1) {\nAlembic::AbcCoreFactory::IFactory factory;\n*result = factory.getArchive(archives);\n} else\n.\nw\nq" | ed -s pxr/usd/plugin/usdAbc/alembicReader.cpp ) && \
	echo ">>>" Patching Thread Limits for comatibility with MTOA... && \
	( printf "/Work_GetConcurrencyLimitSetting/\n/{/\na\nreturn Work_NormalizeThreadCount(0);\n.\nw\nq" | ed -s pxr/base/lib/work/threadLimits.cpp ) && \
	echo ">>>" Patching glf, hd, hdx to load built-in shaders... && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/GlfGLSLFX::_ProcessFile/--a\nextern \"C\" __attribute__((weak)) std::istream* _GetFileStream(const char* filename)\n.\nw\nq" | ed -s pxr/imaging/lib/glf/glslfx.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/GlfGLSLFX::_ProcessFile/--a\n{ return new std::ifstream(filename); }\n.\nw\nq" | ed -s pxr/imaging/lib/glf/glslfx.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/GlfGLSLFX::_ProcessFile/++d\nd\nd\nd\nd\nw\nq" | ed -s pxr/imaging/lib/glf/glslfx.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/_seenFiles.insert/a\nboost::scoped_ptr<istream> inputPtr(_GetFileStream(filePath.c_str()));\nreturn _ProcessInput(inputPtr.get(), context);\n.\nw\nq" | ed -s pxr/imaging/lib/glf/glslfx.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/_GetShaderPath/-d\n-a\nextern \"C\" __attribute__((weak)) TfToken\n.\nw\nq" | ed -s pxr/imaging/lib/glf/package.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/_GetShaderPath/-d\n-a\nextern \"C\" __attribute__((weak)) TfToken\n.\nw\nq" | ed -s pxr/imaging/lib/hdSt/package.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/_GetShaderPath/-d\n-a\nextern \"C\" __attribute__((weak)) TfToken\n.\nw\nq" | ed -s pxr/imaging/lib/hdx/package.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/_GetShaderPath/-d\n-a\nextern \"C\" __attribute__((weak)) TfToken\n.\nw\nq" | ed -s pxr/usdImaging/lib/usdImagingGL/package.cpp ) && \
	echo ">>>" Patching schemaRegistry for ability to keep schemas in the binary... && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/_GetGeneratedSchema/-d\n-a\nextern \"C\" __attribute__((weak)) SdfLayerRefPtr\n.\nw\nq" | ed -s pxr/usd/lib/usd/schemaRegistry.cpp ) && \
	echo ">>>" Turning off hdx conservative rasterization... && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/GL_NV_conservative_raster/a\nconvRstr = false;\n.\nw\nq" | ed -s pxr/imaging/lib/hdx/intersector.cpp ) && \
	echo ">>>" Catmull-Clark is default subdivision scheme for all the alembics. It's temporary, while Hydra doesn't consider normals... && \
	( printf "/UsdGeomTokens->subdivisionScheme/+2\ns/none/catmullClark/\nw\nq" | ed -s pxr/usd/plugin/usdAbc/alembicReader.cpp ) && \
	echo ">>>" Patching bug in Katana linking... && \
	( printf "/katanaAttrfncApi/a\nkatanaOpApi\nkatanaPluginApi\n.\nw\nq" | ed -s third_party/katana/lib/usdKatana/CMakeLists.txt ) && \
	echo ">>>" Dont skip plugins when building static libraries... && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/Skipping plugin/\nd\nd\na\nset(args_TYPE \"STATIC\")\n.\nw\nq" | ed -s cmake/macros/Public.cmake ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/CMAKE_SHARED_LIBRARY_SUFFIX/s/CMAKE_SHARED_LIBRARY_SUFFIX/CMAKE_STATIC_LIBRARY_SUFFIX/\nw\nq" | ed -s cmake/macros/Public.cmake ) && \
	echo ">>>" Patching Houdini plugin... && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/if(.*PXR_ENABLE_PYTHON_SUPPORT.*)/s/(.*)/(0)/\nw\nq" | ed -s CMakeLists.txt ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/if(.*PXR_ENABLE_PYTHON_SUPPORT.*)/s/(.*)/(0)/\nw\nq" | ed -s CMakeLists.txt ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/if(.*PXR_ENABLE_PYTHON_SUPPORT.*)/s/(.*)/(0)/\nw\nq" | ed -s CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/a\n$(PREFIX_ROOT)/boost/include\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/a\n$(PREFIX_ROOT)/boost/include\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniAPPS1.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniAPPS2.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniAPPS3.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniGEO.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniOP1.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniOP2.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniOP3.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniOPZ.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniPRM.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniRAY.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniUI.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniUT.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniAPPS1.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniAPPS2.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniAPPS3.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniGEO.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniOP1.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniOP2.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniOP3.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniOPZ.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniPRM.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniRAY.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniUI.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libHoudiniUT.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( test ! $(USE_STATIC_BOOST) == OFF || printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libhboost_system.so\n.\nw\nq" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	(  test ! $(USE_STATIC_BOOST) == OFF || printf "/INCLUDE_DIRS/-a\n\044{HOUDINI_LIB_DIRS}/libhboost_system.so\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "/INCLUDE_DIRS/-a\n$(PREFIX_ROOT)/boost/lib/lib$(BOOST_NAMESPACE)_filesystem$(DYNAMIC_EXT)\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "\044a\nfile(GLOB MYHEADERS *.h)\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( printf "\044a\ninstall(FILES \044{MYHEADERS} DESTINATION \044{PXR_INSTALL_SUBDIR}/include/OP_gusd)\n.\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/CMakeLists.txt ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/newDriverOperator/s/newDriverOperator/originalNewDriverOperator/\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/plugin.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/newSopOperator/s/newSopOperator/originalNewSopOperator/\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/plugin.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/newObjectOperator/s/newObjectOperator/originalNewObjectOperator/\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/plugin.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/newGeometryPrim/s/newGeometryPrim/originalNewGeometryPrim/\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/plugin.cpp ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/newGeometryIO/s/newGeometryIO/originalNewGeometryIO/\nw\nq" | ed -s third_party/houdini/plugin/OP_gusd/plugin.cpp ) && \
	echo ">>>" Walter layers support in usdview... && \
	( printf "/os.path.isfile.*usdFilePath/s/if /if 0 and /\nw\nq" | ed -s pxr/usdImaging/lib/usdviewq/appController.py ) && \
	echo ">>>" Patching embree plugin... && \
	( printf "/libembree.so/s/libembree.so/embree/\nw\nq" | ed -s cmake/modules/FindEmbree.cmake ) && \
	( printf "/find_package_handle_standard_args/-a\nlist(APPEND EMBREE_LIBRARY $(PREFIX_ROOT)/embree/lib/libembree_avx.a)\n.\nw\nq" | ed -s cmake/modules/FindEmbree.cmake ) && \
	( printf "/find_package_handle_standard_args/-a\nlist(APPEND EMBREE_LIBRARY $(PREFIX_ROOT)/embree/lib/libembree_avx2.a)\n.\nw\nq" | ed -s cmake/modules/FindEmbree.cmake ) && \
	( printf "/find_package_handle_standard_args/-a\nlist(APPEND EMBREE_LIBRARY $(PREFIX_ROOT)/embree/lib/libembree_sse42.a)\n.\nw\nq" | ed -s cmake/modules/FindEmbree.cmake ) && \
	( printf "/find_package_handle_standard_args/-a\nlist(APPEND EMBREE_LIBRARY $(PREFIX_ROOT)/embree/lib/liblexers.a)\n.\nw\nq" | ed -s cmake/modules/FindEmbree.cmake ) && \
	( printf "/find_package_handle_standard_args/-a\nlist(APPEND EMBREE_LIBRARY $(PREFIX_ROOT)/embree/lib/libsimd.a)\n.\nw\nq" | ed -s cmake/modules/FindEmbree.cmake ) && \
	( printf "/find_package_handle_standard_args/-a\nlist(APPEND EMBREE_LIBRARY $(PREFIX_ROOT)/embree/lib/libsys.a)\n.\nw\nq" | ed -s cmake/modules/FindEmbree.cmake ) && \
	( printf "/find_package_handle_standard_args/-a\nlist(APPEND EMBREE_LIBRARY $(PREFIX_ROOT)/embree/lib/libtasking.a)\n.\nw\nq" | ed -s cmake/modules/FindEmbree.cmake ) && \
	echo ">>>" Fixing bug with No module named PySide when opening Tree View in Houdini USD Import node... && \
	( printf "/QtWidgets/d\na\ntry:\n\tfrom qt import QtWidgets\nexcept ImportError:\n\tpass\n.\nw\nq" | ed -s pxr/usdImaging/lib/usdviewq/__init__.py ) && \
	( test ! $(USE_STATIC_BOOST) == ON || echo ">>>" Patching Houdini Mesh Wrapper to skip normals... ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/PUBLIC_CLASSES/a\nmeshWrapper\n.\nw\n" | ed -s third_party/houdini/lib/gusd/CMakeLists.txt ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/if.*(.*normalsAttr.HasAuthoredValueOpinion.*)/s/if.*(.*)/if(0)/\nw\n" | ed -s third_party/houdini/lib/gusd/meshWrapper.cpp ) && \
	echo ">>>" Fixing bug with Katana plugin unresolved symbols... && \
	( printf "/PYMODULE_CPPFILES/a\nwrapCache.cpp\n.\nw\n" | ed -s third_party/katana/lib/usdKatana/CMakeLists.txt ) && \
	( test ! $(USE_STATIC_BOOST) == ON || echo ">>>" Patching hydra to not select children... ) && \
	( test ! $(USE_STATIC_BOOST) == ON || printf "/UsdImagingDelegate::PopulateSelection/\n/} else {/s/else/else if(0)/\nw\n" | ed -s pxr/usdImaging/lib/usdImaging/delegate.cpp ) && \
	mkdir -p build && cd build && \
	mkdir -p $(PREFIX_ROOT) && \
	$(PYSIDE_CONF) \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DALEMBIC_DIR=$(PREFIX_ROOT)/alembic \
		-DBOOST_ROOT:STRING=$(PREFIX_ROOT)/boost \
		-DBUILD_SHARED_LIBS:BOOL=$(BUILD_SHARED_LIBS) \
		-DBoost_NAMESPACE=$(BOOST_NAMESPACE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/$(USD_PACKAGE_NAME) \
		-DDOUBLE_CONVERSION_INCLUDE_DIR:PATH=$(PREFIX_ROOT)/dc/include \
		-DDOUBLE_CONVERSION_LIBRARY:PATH=$(PREFIX_ROOT)/dc/lib/libdouble-conversion.a \
		-DEMBREE_LOCATION:PATH=$(PREFIX_ROOT)/embree \
		-DGLEW_LOCATION:PATH=$(PREFIX_ROOT)/glew \
		-DGLFW_LOCATION:PATH=$(PREFIX_ROOT)/glfw \
		-DGLUT_Xmu_LIBRARY= \
		-DHDF5_ROOT=$(PREFIX_ROOT)/hdf5 \
		-DHOUDINI_ROOT:PATH=$(HOUDINI_ROOT) \
		-DKATANA_API_LOCATION:PATH=$(KATANA_ROOT) \
		-DMAYA_LOCATION:PATH=$(MAYA_ROOT) \
		-DOIIO_LOCATION:PATH=$(PREFIX_ROOT)/oiio \
		-DOPENEXR_BASE_DIR:PATH=$(PREFIX_ROOT)/ilmbase \
		-DOPENEXR_LOCATION:PATH=$(PREFIX_ROOT)/openexr \
		-DOPENSUBDIV_ROOT_DIR:PATH=$(PREFIX_ROOT)/opensubdiv \
		-DPTEX_LOCATION:PATH=$(PREFIX_ROOT)/ptex \
		-DPXR_BUILD_ALEMBIC_PLUGIN:BOOL=ON \
		-DPXR_BUILD_EMBREE_PLUGIN:BOOL=ON \
		-DPXR_BUILD_HOUDINI_PLUGIN:BOOL=$(BUILD_HOUDINI_PLUGINS) \
		-DPXR_BUILD_IMAGING:BOOL=ON \
		-DPXR_BUILD_KATANA_PLUGIN:BOOL=$(BUILD_KATANA_PLUGINS) \
		-DPXR_INSTALL_LOCATION=$(PREFIX_ROOT)/$(USD_PACKAGE_NAME)/share/usd/plugins \
		-DPXR_BUILD_MAYA_PLUGIN:BOOL=$(BUILD_SHARED_LIBS) \
		-DPXR_BUILD_MONOLITHIC:BOOL=$(BUILD_SHARED_LIBS) \
		-DPXR_BUILD_TESTS:BOOL=OFF \
		-DPXR_ENABLE_PYTHON_SUPPORT:BOOL=$(BUILD_SHARED_LIBS) \
		-DPXR_BUILD_USD_IMAGING:BOOL=ON \
		-DPXR_ENABLE_HDF5_SUPPORT:BOOL=ON \
		-DPXR_SET_INTERNAL_NAMESPACE=rdo$(USD_PACKAGE_NAME) \
		-DPYSIDE_BIN_DIR=$(PREFIX_ROOT)/pyside/bin \
		-DPYTHON_EXECUTABLE=$(PYTHON_BIN) \
		-DTBB_LIBRARY=$(TBB_LIBRARY) \
		-DTBB_ROOT_DIR=$(TBB_ROOT_DIR) \
		-DZLIB_ROOT:PATH=$(PREFIX_ROOT)/zlib \
		-D_GLUT_INC_DIR:PATH=$(PREFIX_ROOT)/glut/include \
		-D_GLUT_glut_LIB_DIR:PATH=$(PREFIX_ROOT)/glut/lib \
		.. > $(PREFIX_ROOT)/log_usd.txt 2>&1 && \
	$(PYSIDE_CONF) \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) >> $(PREFIX_ROOT)/log_usd.txt 2>&1 && \
		echo "USD source path: " $(USD_FILE) && \
	cd ../.. && \
	cd $(THIS_DIR) && \
	echo $(USD_VERSION) > $@


# libz
$(ZLIB_STAMP) : $(ZLIB_FILE)/HEAD
	@echo Building zlib $(ZLIB_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf zlib && \
	git clone -q --no-checkout $(SOURCES_ROOT)/zlib.git zlib && \
	cd zlib && \
	git checkout -q $(ZLIB_VERSION) && \
	mkdir -p $(PREFIX_ROOT) && \
	$(COMPILER_CONF) \
	./configure \
		--const \
		--zprefix \
		--prefix=$(PREFIX_ROOT)/zlib \
		--static > $(PREFIX_ROOT)/log_zlib.txt 2>&1 && \
	$(MAKE) -j$(JOB_COUNT) \
		MAKE_MODE=$(MAKE_MODE) \
		install >> $(PREFIX_ROOT)/log_zlib.txt 2>&1 && \
	cd .. && \
	rm -rf zlib && \
	cd $(THIS_DIR) && \
	echo $(ZLIB_VERSION) > $@
