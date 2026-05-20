# Thin convenience wrapper over CMake. The real build system is CMake;
# this file just spares you from typing `cmake -S . -B build && cmake
# --build build -j`. Out-of-source by design (CMakeLists.txt refuses
# in-source configures).
#
#   make             configure (if needed) and build, Release defaults
#   make test        build + ctest --output-on-failure
#   make debug       configure build-debug/ as Debug and build it
#   make asan        configure build-asan/ with SLI3_SANITIZE=ON
#   make clean       cmake --build build --target clean (preserves config)
#   make distclean   wipe build/, build-debug/, build-asan/
#
# Override the directory or jobs at the command line:
#   make BUILD_DIR=out
#   make JOBS=8

BUILD_DIR ?= build
JOBS      ?=

CMAKE       ?= cmake
CTEST       ?= ctest
CMAKE_FLAGS ?=

# `cmake --build -j` with no argument uses all cores; passing a value
# overrides it. JOBS is empty by default → all cores.
ifeq ($(strip $(JOBS)),)
  BUILD_JOBS := -j
else
  BUILD_JOBS := -j$(JOBS)
endif

.PHONY: all build configure test debug asan clean distclean help

all: build

$(BUILD_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)

configure: $(BUILD_DIR)/CMakeCache.txt

build: configure
	$(CMAKE) --build $(BUILD_DIR) $(BUILD_JOBS)

test: build
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure

debug:
	$(CMAKE) -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug $(CMAKE_FLAGS)
	$(CMAKE) --build build-debug $(BUILD_JOBS)

asan:
	$(CMAKE) -S . -B build-asan -DSLI3_SANITIZE=ON $(CMAKE_FLAGS)
	$(CMAKE) --build build-asan $(BUILD_JOBS)

clean:
	@if [ -d $(BUILD_DIR) ]; then $(CMAKE) --build $(BUILD_DIR) --target clean; \
	 else echo "no $(BUILD_DIR) yet — nothing to clean"; fi

distclean:
	rm -rf build build-debug build-asan

help:
	@sed -n 's/^#\{1,\} \{0,1\}//p' $(firstword $(MAKEFILE_LIST)) | head -20
