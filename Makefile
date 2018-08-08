################################################################################
# Copyright 2018 Rodeo FX.  All rights reserved.
#
# Master Makefile to build Walter and all its dependencies
# 
# VFX platform libraries and Walter are built using this file.
# Usd and Walter are built in two flavors. A 'dcc' one the for Katana, Houdini
# and Maya plugins and a 'procedural' one for the Arnold.
#
# Run 'make help' to list helpful targets.
#
################################################################################

#===============================================================================
# Set environment variables for the build.

# 'Release' or 'Debug'
BUILD_CONFIG := Release
VERBOSE := 1
BUILD_TESTS := ON
BUILD_HOUDINI_PLUGINS := ON
BUILD_KATANA_PLUGINS := ON
BUILD_MAYA_PLUGINS := ON
BUILD_VIEWER := ON

THIS_DIR = $(shell pwd)
JOB_COUNT := $(shell cat /sys/devices/system/cpu/cpu*/topology/thread_siblings | wc -l)

SOURCES_ROOT := ${THIS_DIR}/build/src
BUILD_ROOT := ${THIS_DIR}/build/build
PREFIX_ROOT := ${THIS_DIR}/build/lib

ifeq "$(ARNOLD_ROOT)" ""
	ARNOLD_ROOT := (empty)
endif
ifeq "$(KATANA_ROOT)" ""
	KATANA_ROOT := (empty)
endif
ifeq "$(KTOA_ROOT)" ""
	KTOA_ROOT := (empty)
endif
ifeq "$(HOUDINI_ROOT)" ""
	HOUDINI_ROOT := (empty)
endif
ifeq "$(MAYA_ROOT)" ""
	MAYA_ROOT := (empty)
endif
ifeq "$(MTOA_ROOT)" ""
	MTOA_ROOT := (empty)
endif

USD_DCC_PACKAGE_NAME := usdDCC
USD_DCC_STAMP := $(PREFIX_ROOT)/built_$(USD_DCC_PACKAGE_NAME)
USD_PROCEDURAL_PACKAGE_NAME := usdProcedural
USD_PROCEDURAL_STAMP := $(PREFIX_ROOT)/built_$(USD_PROCEDURAL_PACKAGE_NAME)

JSONCPP_STAMP := $(PREFIX_ROOT)/built_jsoncpp
GOOGLETEST_STAMP := $(PREFIX_ROOT)/built_googletest
OPENVDB_STAMP := $(PREFIX_ROOT)/built_openvdb

WALTER_DCC_STAMP := $(PREFIX_ROOT)/built_walterDCC
WALTER_PROCEDURAL_STAMP := $(PREFIX_ROOT)/built_walterProcedural

GCC_BIN_PATH := /usr/bin
CMAKE=$(PREFIX_ROOT)/cmake/bin/cmake
CTEST=$(PREFIX_ROOT)/cmake/bin/ctest

ALL_TARGETS :=\
	$(USD_DCC_STAMP) \
	$(USD_PROCEDURAL_STAMP) \
	$(WALTER_DCC_STAMP) \
	$(WALTER_PROCEDURAL_STAMP)

BOOST_NAMESPACE := rdoBoostWalter
TBB_NAMESPACE := rdoTbbWalter
USD_RESOLVER_NAME := AbcCoreLayerResolver

WALTER_VERSION = 1.0.0
WALTER_MAJOR_VERSION := $(word 1, $(subst ., ,$(WALTER_VERSION)))
WALTER_MINOR_VERSION := $(word 2, $(subst ., ,$(WALTER_VERSION)))
WALTER_PATCH_VERSION := $(word 3, $(subst ., ,$(WALTER_VERSION)))

#=============================================================================
# Shortcuts
all : $(ALL_TARGETS)
usd_dcc : $(USD_DCC_STAMP)
usd_procedural : $(USD_PROCEDURAL_STAMP)
walter_dcc: $(WALTER_DCC_STAMP)
walter_procedural: $(WALTER_PROCEDURAL_STAMP)

#=============================================================================
# Clean target for Walter DCC.
clean_dcc:
	rm -f $(WALTER_DCC_STAMP)
.PHONY : clean_dcc

#=============================================================================
# Clean target for Walter DCC.
clean_procedural:
	rm -f $(WALTER_PROCEDURAL_STAMP)
.PHONY : clean_procedural

#=============================================================================
# Clean target for Walter (DCC and Procedural).
clean: clean_dcc clean_procedural
.PHONY : clean

#=============================================================================
# Clean target for USD (DCC and Procedural).
clean_usd:
	rm -f $(USD_DCC_STAMP) && \
	rm -f $(USD_PROCEDURAL_STAMP)
.PHONY : clean_usd

#=============================================================================
# Clean Walter and USD
clean_all: clean_usd clean
.PHONY : clean_all

#=============================================================================
# Target to list tests.
ls_tests :
	@echo "DCC plugins integration tests:" && \
	test -d $(BUILD_ROOT)/walter && cd $(BUILD_ROOT)/walter && $(CTEST) -N
	@echo "Procedural plugins integration tests:" && \
	test -d $(BUILD_ROOT)/walter_procedural && cd $(BUILD_ROOT)/walter_procedural && $(CTEST) -N

.PHONY: ls_tests

#=============================================================================
# Target to for TEST in DCC plugins test.
test :
	@cd $(BUILD_ROOT)/walter && \
	$(CTEST) -V -R $(TEST)
.PHONY: test

# Target to for TEST in Procedural tests.
test_p :
	@cd $(BUILD_ROOT)/walter_procedural && \
	$(CTEST) -V -R $(TEST)
.PHONY: test

#=============================================================================
# Target for dcc tests.
test_dcc :
	@cd $(BUILD_ROOT)/walter && \
	$(CTEST) -V
.PHONY: test_dcc

#=============================================================================
# Target for Arnold procedural tests.
test_procedural :
	@cd $(BUILD_ROOT)/walter_procedural && \
	$(CTEST) -V
.PHONY: test_procedural

#=============================================================================
# Target for all tests.
tests : test_dcc test_procedural
.PHONY: tests

#=============================================================================
# Target rules for usd (used by Walter DCC plugins)
$(USD_DCC_STAMP) :
	@$(call vfx_builder,"USD for DCC plugins",$(USD_DCC_PACKAGE_NAME),$(BOOST_NAMESPACE),$(TBB_NAMESPACE),usd)

#=============================================================================
# Target rules for usd  (used by Walter for Arnold Procedural)
$(USD_PROCEDURAL_STAMP) :
	@$(call vfx_builder,"USD for Arnold procedural",$(USD_PROCEDURAL_PACKAGE_NAME),$(BOOST_NAMESPACE),$(TBB_NAMESPACE),usd)

#=============================================================================
# Target rules for Walter dependencies not needed by USD

$(JSONCPP_STAMP) :
	@$(call vfx_builder,"Json CPP","",$(BOOST_NAMESPACE),$(TBB_NAMESPACE),jsoncpp)

$(GOOGLETEST_STAMP) :
	@$(call vfx_builder,"Google Test","",$(BOOST_NAMESPACE),$(TBB_NAMESPACE),googletest)

$(OPENVDB_STAMP) :
	@$(call vfx_builder,"Open VDB","",$(BOOST_NAMESPACE),$(TBB_NAMESPACE),openvdb)

################################################################################
# Target rules for walter DCC plugins

# Build rule for target.
$(WALTER_DCC_STAMP) : $(USD_DCC_STAMP) $(JSONCPP_STAMP) $(GOOGLETEST_STAMP) $(OPENVDB_STAMP)
	@echo Building Walter
	rm -rf $(BUILD_ROOT)/walter && \
	mkdir $(BUILD_ROOT)/walter && cd $(BUILD_ROOT)/walter && \
	$(CMAKE) \
	-DWALTER_MAJOR_VERSION=$(WALTER_MAJOR_VERSION) \
	-DWALTER_MINOR_VERSION=$(WALTER_MINOR_VERSION) \
	-DWALTER_PATCH_VERSION=$(WALTER_PATCH_VERSION) \
	-DUSD_RESOLVER_NAME=$(USD_RESOLVER_NAME) \
	-DRDO_RESOLVER_ROOT=$(PREFIX_ROOT)/RdoResolver \
	-DALEMBIC_ROOT=$(PREFIX_ROOT)/alembic \
	-DARNOLD_BASE_DIR=$(ARNOLD_ROOT) \
	-DBOOST_ROOT=$(PREFIX_ROOT)/boost \
	-DBUILD_ARNOLD_PROCEDURALS=OFF \
	-DBUILD_HOUDINI_PLUGINS=$(BUILD_HOUDINI_PLUGINS) \
	-DBUILD_KATANA_PLUGINS=$(BUILD_KATANA_PLUGINS) \
	-DBUILD_MAYA_PLUGINS=$(BUILD_MAYA_PLUGINS) \
	-DBUILD_TESTS=$(BUILD_TESTS) \
	-DBUILD_VIEWER=$(BUILD_VIEWER) \
	-DBoost_NAMESPACE=$(BOOST_NAMESPACE) \
	-DCMAKE_BUILD_TYPE=$(BUILD_CONFIG) \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/walter \
	-DCMAKE_SHARED_LINKER_FLAGS=-Wl,--no-undefined \
	-DCMAKE_VERBOSE_MAKEFILE=$(VERBOSE) \
	-DGLEW_INCLUDE_DIR=$(PREFIX_ROOT)/glew/include \
	-DGLEW_LOCATION=$(PREFIX_ROOT)/glew \
	-DGLFW_DIR=$(PREFIX_ROOT)/glfw \
	-DGoogleTest_DIR=$(PREFIX_ROOT)/googletest \
	-DHDF5_ROOT=$(PREFIX_ROOT)/hdf5 \
	-DHOUDINI_ROOT=$(HOUDINI_ROOT) \
	-DILMBASE_HOME=$(PREFIX_ROOT)/ilmbase \
	-DILMBASE_ROOT=$(PREFIX_ROOT)/ilmbase \
	-DJPEG_PATH=$(PREFIX_ROOT)/jpeg \
	-DJSONCPP_ROOT_DIR=$(PREFIX_ROOT)/jsoncpp \
	-DKATANA_HOME=$(KATANA_ROOT) \
	-DKTOA_HOME=$(KTOA_ROOT) \
	-DMAYA_LOCATION=$(MAYA_ROOT) \
	-DMTOA_BASE_DIR=$(MTOA_ROOT) \
	-DOCIO_PATH=$(PREFIX_ROOT)/ocio \
	-DOIIO_LOCATION=$(PREFIX_ROOT)/oiio \
	-DOPENEXR_HOME=$(PREFIX_ROOT)/openexr \
	-DOPENSUBDIV_ROOT_DIR=$(PREFIX_ROOT)/opensubdiv \
	-DOPENVDB_LOCATION=$(PREFIX_ROOT)/openvdb \
	-DBLOSC_LOCATION=$(PREFIX_ROOT)/blosc \
	-DPNG_LIBRARY=$(PREFIX_ROOT)/png/lib/libpng.a \
	-DPNG_PNG_INCLUDE_DIR=$(PREFIX_ROOT)/png/include \
	-DPTEX_LOCATION=$(PREFIX_ROOT)/ptex \
	-DPYSTRING_DIR=$(PREFIX_ROOT)/pystring \
	-DPYSTRING_INCLUDE_DIR=$(PREFIX_ROOT)/pystring/include \
	-DPYTHON_EXECUTABLE=$(HOUDINI_ROOT)/python/bin/python \
	-DTBB_LIBRARIES=$(PREFIX_ROOT)/tbb/lib \
	-DTBB_LIBRARY=$(PREFIX_ROOT)/tbb/lib \
	-DTBB_NAMESPACE=$(TBB_NAMESPACE) \
	-DTBB_ROOT_DIR=$(PREFIX_ROOT)/tbb/include \
	-DTIFF_INCLUDE_DIR=$(PREFIX_ROOT)/tiff/include \
	-DTIFF_LIBRARY=$(PREFIX_ROOT)/tiff/lib/libtiff.a \
	-DUSD_ROOT=$(PREFIX_ROOT)/$(USD_DCC_PACKAGE_NAME) \
	-DUSE_HDF5=ON \
	-DUSE_STATIC_BOOST=OFF \
	-DUSE_STATIC_HDF5=ON \
	-DZLIB_ROOT=$(PREFIX_ROOT)/zlib \
	$(THIS_DIR)/walter && \
	$(CMAKE) --build . --target install --config $(BUILD_CONFIG) -- -j$(JOB_COUNT) && \
	echo timestamp > $(WALTER_DCC_STAMP)

# fast build rule for target.
walter_dcc_fast :
	@cd $(BUILD_ROOT)/walter && \
	make install

################################################################################
# Target rules for walter for Arnold

# Build rule for target.
$(WALTER_PROCEDURAL_STAMP) : $(USD_PROCEDURAL_STAMP) $(JSONCPP_STAMP) $(OPENVDB_STAMP) $(GOOGLETEST_STAMP)
	@echo Building Walter Procedural && \
	rm -rf $(BUILD_ROOT)/walter_procedural && \
	mkdir $(BUILD_ROOT)/walter_procedural && cd $(BUILD_ROOT)/walter_procedural && \
	$(CMAKE) \
	-DWALTER_MAJOR_VERSION=$(WALTER_MAJOR_VERSION) \
	-DWALTER_MINOR_VERSION=$(WALTER_MINOR_VERSION) \
	-DWALTER_PATCH_VERSION=$(WALTER_PATCH_VERSION) \
	-DUSD_RESOLVER_NAME=$(USD_RESOLVER_NAME) \
	-DRDO_RESOLVER_ROOT=$(PREFIX_ROOT)/RdoResolverProcedural \
	-DALEMBIC_ROOT=$(PREFIX_ROOT)/alembic \
	-DARNOLD_BASE_DIR=$(ARNOLD_ROOT) \
	-DBOOST_ROOT=$(PREFIX_ROOT)/boost \
	-DBUILD_ARNOLD_PROCEDURALS=ON \
	-DBUILD_HOUDINI_PLUGINS=OFF \
	-DBUILD_KATANA_PLUGINS=OFF \
	-DBUILD_MAYA_PLUGINS=OFF \
	-DBUILD_TESTS=ON \
	-DBoost_NAMESPACE=$(BOOST_NAMESPACE) \
	-DCMAKE_BUILD_TYPE=$(BUILD_CONFIG) \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DCMAKE_INSTALL_PREFIX=$(PREFIX_ROOT)/walter \
	-DCMAKE_SHARED_LINKER_FLAGS=-Wl,--no-undefined \
	-DCMAKE_VERBOSE_MAKEFILE=$(VERBOSE) \
	-DGLEW_INCLUDE_DIR=$(PREFIX_ROOT)/glew/include \
	-DGLEW_LOCATION=$(PREFIX_ROOT)/glew \
	-DGLFW_DIR=$(PREFIX_ROOT)/glfw \
	-DGoogleTest_DIR=$(PREFIX_ROOT)/googletest \
	-DHDF5_ROOT=$(PREFIX_ROOT)/hdf5 \
	-DHOUDINI_ROOT=$(HOUDINI_ROOT) \
	-DILMBASE_HOME=$(PREFIX_ROOT)/ilmbase \
	-DILMBASE_ROOT=$(PREFIX_ROOT)/ilmbase \
	-DJPEG_PATH=$(PREFIX_ROOT)/jpeg \
	-DJSONCPP_ROOT_DIR=$(PREFIX_ROOT)/jsoncpp \
	-DKATANA_HOME=$(KATANA_ROOT) \
	-DKTOA_HOME=$(KTOA_ROOT) \
	-DMAYA_LOCATION=$(MAYA_ROOT) \
	-DMTOA_BASE_DIR=$(MTOA_ROOT) \
	-DOCIO_PATH=$(PREFIX_ROOT)/ocio \
	-DOIIO_LOCATION=$(PREFIX_ROOT)/oiio \
	-DOPENEXR_HOME=$(PREFIX_ROOT)/openexr \
	-DOPENSUBDIV_ROOT_DIR=$(PREFIX_ROOT)/opensubdiv \
	-DOPENVDB_LOCATION=$(PREFIX_ROOT)/openvdb \
	-DBLOSC_LOCATION=$(PREFIX_ROOT)/blosc \
	-DPNG_LIBRARY=$(PREFIX_ROOT)/png/lib/libpng.a \
	-DPNG_PNG_INCLUDE_DIR=$(PREFIX_ROOT)/png/include \
	-DPTEX_LOCATION=$(PREFIX_ROOT)/ptex \
	-DPYSTRING_DIR=$(PREFIX_ROOT)/pystring \
	-DPYSTRING_INCLUDE_DIR=$(PREFIX_ROOT)/pystring/include \
	-DPYTHON_EXECUTABLE=$(HOUDINI_ROOT)/python/bin/python \
	-DTBB_LIBRARIES=$(PREFIX_ROOT)/tbb/lib \
	-DTBB_LIBRARY=$(PREFIX_ROOT)/tbb/lib \
	-DTBB_NAMESPACE=$(TBB_NAMESPACE) \
	-DTBB_ROOT_DIR=$(PREFIX_ROOT)/tbb/include \
	-DTIFF_INCLUDE_DIR=$(PREFIX_ROOT)/tiff/include \
	-DTIFF_LIBRARY=$(PREFIX_ROOT)/tiff/lib/libtiff.a \
	-DUSD_ROOT=$(PREFIX_ROOT)/$(USD_PROCEDURAL_PACKAGE_NAME) \
	-DUSE_HDF5=ON \
	-DUSE_STATIC_BOOST=OFF \
	-DUSE_STATIC_HDF5=ON \
	-DZLIB_ROOT=$(PREFIX_ROOT)/zlib \
	$(THIS_DIR)/walter && \
	$(CMAKE) --build . --target install --config $(BUILD_CONFIG) -- -j$(JOB_COUNT)
	echo timestamp > $(WALTER_PROCEDURAL_STAMP)

# fast build rule for target.
walter_procedural_fast :
	@cd $(BUILD_ROOT)/walter_procedural && \
	make install

#===============================================================================
# Function to build libraries using vfx_platform_builder Makefile
# Args:
# 	$(1): lib name for message
# 	$(2): USD_DCC_PACKAGE_NAME
# 	$(3): BOOST_NAMESPACE
# 	$(4): TBB_NAMESPACE
# 	$(5): targets
vfx_builder = \
	echo "Building" $(1) "for Walter" && \
	cd $(THIS_DIR)/vfx_platform_builder && \
	make CC=$(GCC_BIN_PATH)/gcc CXX=$(GCC_BIN_PATH)/g++ \
	USD_PACKAGE_NAME=$(2) \
	BOOST_NAMESPACE=$(3) \
	TBB_NAMESPACE=$(4) \
	SOURCES_ROOT=$(SOURCES_ROOT) \
	BUILD_ROOT=$(BUILD_ROOT) \
	PREFIX_ROOT=$(PREFIX_ROOT) -j$(JOB_COUNT) \
	$(5)

# Help Target
help:
	@printf '********************* Open Walter building system *********************\n\n' ; \
	printf 'Packages required before install:\n' ; \
	printf '\tnasm libtool autoconf mesa-libGL-devel mesa-libGLU-devel\n' ; \
	printf '\tlibXxf86vm-devel libXrandr-devel libXinerama-devel\n' ; \
	printf '\tlibXcursor-devel libXi-devel libXt-devel\n\n' ; \
	printf 'Usage to build all (VFX Platform, Walter for Katana, Houdini, Maya and Arnold):\n' ; \
	printf '\tmake\nor\n\tmake BUILD_CONFIG=Debug\n\n' ; \
	printf 'Options:\n' ; \
	printf '\tCC\t\t: C copiler path.\t value: $(CC)\n' ; \
	printf '\tCXX\t\t: C++ copiler path.\t value: $(CXX)\n' ; \
	printf '\tBUILD_CONFIG\t: Debug|Release.\t value: $(BUILD_CONFIG)\n' ; \
	printf '\tSOURCES_ROOT\t: Source directory.\t value: $(SOURCES_ROOT)\n' ; \
	printf '\tBUILD_ROOT\t: Building directory.\t value: $(BUILD_ROOT)\n' ; \
	printf '\tPREFIX_ROOT\t: Installation dir.\t value: $(PREFIX_ROOT)\n' ; \
	printf '\tARNOLD_ROOT\t: The path to Arnold install directory.\t value: $(ARNOLD_ROOT)\n' ; \
	printf '\tKATANA_ROOT\t: The path to Katana install directory.\t value: $(KATANA_ROOT)\n' ; \
	printf '\tKTOA_ROOT\t: The path to KtoA install directory.\t value: $(KTOA_ROOT)\n' ; \
	printf '\tHOUDINI_ROOT\t: The path to Houdini install directory. value: $(HOUDINI_ROOT)\n' ; \
	printf '\tMAYA_ROOT\t: The path to Maya install directory.\t value: $(MAYA_ROOT)\n' ; \
	printf '\tMTOA_ROOT\t: The path to MtoA install directory.\t value: $(MTOA_ROOT)\n' ; \
	printf '\tUSD_RESOLVER_NAME\t: Usd resolver plugin to use (AbcCoreLayerResolver or RdoResolver).\t value: $(USD_RESOLVER_NAME)\n' ; \
	printf '\nTo disable specific target:\n' ; \
	printf '\tmake walter_dcc                 : only Walter for Houdini, Katana or Maya plugins\n' ; \
	printf '\tmake walter_procedural          : only Walter for Arnold\n' ; \
	printf '\tmake BUILD_HOUDINI_PLUGINS=0    : No Houdini plugin\n' ; \
	printf '\tmake BUILD_KATANA_PLUGINS=0     : No Katana plugin\n' ; \
	printf '\tmake BUILD_MAYA_PLUGINS=0       : No Maya plugin\n\n' ; \
	printf '\tmake BUILD_TESTS=0              : Skip tests\n' ; \
	printf '\tmake BUILD_VIEWER=0             : No viewer\n' ; \
	printf '\tmake usd_dcc                    : only USD for Houdini, Katana or Maya plugins\n' ; \
	printf '\tmake usd_procedural             : only USD for Arnold\n' ; \
	printf '\nTo fast re-build specific target (work only if you already built once):\n' ; \
	printf '\tmake walter_dcc_fast            : fast re-build only Walter for Houdini, Katana or Maya plugins\n' ; \
	printf '\tmake walter_procedural_fast     : re-build only Walter for Arnold\n' ; \
	printf '\nTo test:\n' ; \
	printf '\tmake ls_tests                   : list all tests\n' ; \
	printf '\tmake tests                      : run all tests\n' ; \
	printf '\tmake test_dcc                   : run tests for Houdini, Katana and Maya\n' ; \
	printf '\tmake test TEST=test_name        : run just one test from the DCC plugins tests \n' ; \
	printf '\tmake test_p TEST=test_name      : run just one test from the Procedural plugin tests \n' ; \
	printf '\tmake test_procedural            : run tests for the Procedural plugin\n' ; \
	printf '\nTo clean specific target:\n' ; \
	printf '\tmake clean_dcc                  : clean DCC plugins\n' ; \
	printf '\tmake clean_procedural           : clean Procedural plugins\n' ; \
	printf '\tmake clean                      : clean DCC and Procedural plugins\n' ; \
	printf '\tmake clean_usd                  : clean USD for DCC and Procedural\n' ; \
	printf '\tmake clean_all                  : clean Walter and USD\n' ; \
