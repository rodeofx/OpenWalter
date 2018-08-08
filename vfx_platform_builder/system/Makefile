
# Save the current directory
THIS_DIR := $(shell pwd)

# Versions
BISON_VERSION := 3.0.4
FLEX_VERSION := 2.6.4
ISPC_VERSION := v1.9.1
LLVM_VERS := 3.9.0

# Version files that should indicate that the build is successfull
BISON_STAMP := $(PREFIX_ROOT)/built_bison
FLEX_STAMP := $(PREFIX_ROOT)/built_flex
ISPC_STAMP := $(PREFIX_ROOT)/built_ispc
LLVM_STAMP := $(PREFIX_ROOT)/built_llvm

BISON_SOURCE := https://ftp.gnu.org/gnu/bison/bison-$(BISON_VERSION).tar.gz
CLANGTOOLSEXTRA_SOURCE := http://releases.llvm.org/$(LLVM_VERS)/clang-tools-extra-$(LLVM_VERS).src.tar.xz
CLANG_SOURCE := http://releases.llvm.org/$(LLVM_VERS)/cfe-$(LLVM_VERS).src.tar.xz
COMPILERRT_SOURCE := http://releases.llvm.org/$(LLVM_VERS)/compiler-rt-$(LLVM_VERS).src.tar.xz
FLEX_SOURCE := https://github.com/westes/flex/releases/download/v$(FLEX_VERSION)/flex-$(FLEX_VERSION).tar.gz
ISPC_SOURCE := git@github.com:ispc/ispc.git
LLVM_SOURCE := http://releases.llvm.org/$(LLVM_VERS)/llvm-$(LLVM_VERS).src.tar.xz

BISON_FILE := $(SOURCES_ROOT)/$(notdir $(BISON_SOURCE))
CLANGTOOLSEXTRA_FILE := $(SOURCES_ROOT)/$(notdir $(CLANGTOOLSEXTRA_SOURCE))
CLANG_FILE := $(SOURCES_ROOT)/$(notdir $(CLANG_SOURCE))
COMPILERRT_FILE := $(SOURCES_ROOT)/$(notdir $(COMPILERRT_SOURCE))
FLEX_FILE := $(SOURCES_ROOT)/$(notdir $(FLEX_SOURCE))
ISPC_FILE := $(SOURCES_ROOT)/$(notdir $(ISPC_SOURCE))
LLVM_FILE := $(SOURCES_ROOT)/$(notdir $(LLVM_SOURCE))

# Defaults
CMAKE := cmake
CMAKE_BUILD_TYPE := Release
MAKE := $(shell which make)
PREFIX_ROOT=/tmp/lib
PYTHON_BIN := python
SOURCES_ROOT=/tmp/src
CLANG := $(PREFIX_ROOT)/llvm/bin/clang
CLANGXX := $(CLANG)++
ISPC := $(PREFIX_ROOT)/ispc/bin/ispc

ifneq ("$(wildcard $(CLANG))","")
    LLVM_STAMP:=
endif

ifneq ("$(wildcard $(ISPC))","")
    ISPC_STAMP:=
endif

# Number of processors
ifeq "$(OS)" "Darwin"
JOB_COUNT := $(shell sysctl -n machdep.cpu.thread_count)
endif
ifeq "$(OS)" "linux"
JOB_COUNT := $(shell cat /sys/devices/system/cpu/cpu*/topology/thread_siblings | wc -l)
endif

$(BISON_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(BISON_FILE)... && \
	curl --tlsv1.2 -s -o $@ -L $(BISON_SOURCE)
$(CLANGTOOLSEXTRA_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(CLANGTOOLSEXTRA_FILE)... && \
	curl -s -o $@ -L $(CLANGTOOLSEXTRA_SOURCE)
$(CLANG_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(CLANG_FILE)... && \
	curl -s -o $@ -L $(CLANG_SOURCE)
$(COMPILERRT_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(COMPILERRT_FILE)... && \
	curl -s -o $@ -L $(COMPILERRT_SOURCE)
$(FLEX_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(FLEX_FILE)... && \
	curl --tlsv1.2 -s -o $@ -L $(FLEX_SOURCE)
$(ISPC_FILE)/HEAD :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(ISPC_FILE)... && \
	git clone -q --bare $(ISPC_SOURCE) $(ISPC_FILE)
$(LLVM_FILE) :
	@mkdir -p $(SOURCES_ROOT) && \
	echo Downloading $(LLVM_FILE)... && \
	curl -s -o $@ -L $(LLVM_SOURCE)

all: clang ispc
bison: $(BISON_STAMP)
clang: llvm
flex: $(FLEX_STAMP)
ispc: $(ISPC_STAMP)
llvm: $(LLVM_STAMP)


$(BISON_STAMP) : $(LLVM_STAMP) $(BISON_FILE)
	@echo Building Bison $(BISON_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(basename $(BISON_FILE)))) && \
	tar xf $(SOURCES_ROOT)/$(notdir $(BISON_FILE)) && \
	cd $(notdir $(basename $(basename $(BISON_FILE)))) && \
	CC=$(CLANG) \
	CXX=$(CLANGXX) \
	./configure \
		--prefix=$(PREFIX_ROOT)/bison && \
	$(MAKE) \
		install -j$(JOB_COUNT) && \
	cd .. && \
	rm -rf $(notdir $(basename $(basename $(BISON_FILE)))) && \
	cd $(THIS_DIR) && \
	echo $(BISON_VERSION) > $@


$(FLEX_STAMP) : $(BISON_STAMP) $(LLVM_STAMP) $(FLEX_FILE)
	@echo Building Flex $(FLEX_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(basename $(FLEX_FILE)))) && \
	tar xf $(SOURCES_ROOT)/$(notdir $(FLEX_FILE)) && \
	cd $(notdir $(basename $(basename $(FLEX_FILE)))) && \
	CC=$(CLANG) \
	CXX=$(CLANGXX) \
	./configure \
		--prefix=$(PREFIX_ROOT)/flex && \
	PATH=$(PREFIX_ROOT)/llvm/bin:$(PREFIX_ROOT)/bison/bin:$(PATH) \
	$(MAKE) \
		install -j$(JOB_COUNT) && \
	cd .. && \
	rm -rf $(notdir $(basename $(basename $(FLEX_FILE)))) && \
	cd $(THIS_DIR) && \
	echo $(FLEX_VERSION) > $@


$(LLVM_STAMP) : $(LLVM_FILE) $(CLANG_FILE) $(COMPILERRT_FILE) $(CLANGTOOLSEXTRA_FILE)
	@echo Building llvm and clang $(LLVM_VERS) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(basename $(LLVM_FILE)))) && \
	tar xf $(SOURCES_ROOT)/$(notdir $(LLVM_FILE)) && \
	cd $(notdir $(basename $(basename $(LLVM_FILE)))) && \
	cd tools && \
	tar xf $(SOURCES_ROOT)/$(notdir $(CLANG_FILE)) && \
	mv $(notdir $(basename $(basename $(CLANG_FILE)))) clang && \
	cd clang/tools && \
	tar xf $(SOURCES_ROOT)/$(notdir $(CLANGTOOLSEXTRA_FILE)) && \
	mv $(notdir $(basename $(basename $(CLANGTOOLSEXTRA_FILE)))) extra && \
	cd ../../../projects && \
	tar xf $(SOURCES_ROOT)/$(notdir $(COMPILERRT_FILE)) && \
	mv $(notdir $(basename $(basename $(COMPILERRT_FILE)))) compiler-rt && \
	cd .. && \
	mkdir build && cd build && \
	mkdir -p $(PREFIX_ROOT) && \
	$(CMAKE) \
		$(COMMON_CMAKE_FLAGS) \
		-DCMAKE_INSTALL_PREFIX:STRING=$(PREFIX_ROOT)/llvm \
		-DLLVM_ENABLE_RTTI:BOOL=1 \
		-DLLVM_ENABLE_TERMINFO:BOOL=0 \
		-DLLVM_ENABLE_WARNINGS:BOOL=0 \
		-DLLVM_ENABLE_ZLIB:BOOL=0 \
		-DLLVM_REQUIRES_EH:BOOL=1 \
		-DLLVM_REQUIRES_RTTI:BOOL=1 \
		-DLLVM_TARGETS_TO_BUILD:STRING=X86 \
		-DLLVM_USE_CRT_DEBUG:STRING=MTd \
		-DLLVM_USE_CRT_RELEASE:STRING=MT \
		-DPYTHON_EXECUTABLE:STRING=$(PYTHON_BIN) \
		.. && \
	$(CMAKE) \
		--build . \
		--target install \
		--config $(CMAKE_BUILD_TYPE) \
		-- -j $(JOB_COUNT) && \
	cd ../.. && \
	rm -rf $(notdir $(basename $(basename $(LLVM_FILE)))) && \
	cd $(THIS_DIR) && \
	echo $(LLVM_VERS) > $@


# Intel(r) SPMD Program Compiler
$(ISPC_STAMP) : $(BISON_STAMP) $(FLEX_STAMP) $(LLVM_STAMP) $(ISPC_FILE)/HEAD
	@echo Building ISPC $(ISPC_VERSION) && \
	mkdir -p $(BUILD_ROOT) && cd $(BUILD_ROOT) && \
	rm -rf $(notdir $(basename $(ISPC_FILE))) && \
	git clone -q --no-checkout $(SOURCES_ROOT)/$(notdir $(ISPC_FILE)) $(notdir $(basename $(ISPC_FILE))) && \
	cd $(notdir $(basename $(ISPC_FILE))) && \
	git checkout -q $(ISPC_VERSION) && \
	( printf '/ISPC_LLVM_3_9/a\n#define ISPC_LLVM_4_0 40000\n.\nw\nq' | ed -s ispc_version.h ) && \
	( printf '/LATEST_SUPPORTED_LLVM/s/3_9/4_0/\nw\nq' | ed -s ispc_version.h ) && \
	PATH=$(dir $(CLANG)):$(PREFIX_ROOT)/bison/bin:$(PREFIX_ROOT)/flex/bin:$(PATH) \
	$(MAKE) \
		CXX=$(CLANGXX) \
		LLVM_HOME=$(dir $(dir $(CLANG))) \
		-j$(JOB_COUNT) && \
	mkdir -p $(PREFIX_ROOT)/ispc/bin && \
	mv ./ispc $(PREFIX_ROOT)/ispc/bin && \
	cd .. && \
	rm -rf $(notdir $(basename $(ISPC_FILE))) && \
	cd $(THIS_DIR) && \
	echo $(ISPC_VERSION) > $@
