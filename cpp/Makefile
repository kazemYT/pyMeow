# pyMeow C++ -- plain Makefile (no cmake required)
#
#   make                     build the static library  -> build/libpymeow.a
#   make shared              build the shared library  -> build/libpymeow.so|.dll
#   make test                build + run the core test suite (no graphics needed)
#   make examples            build every example of examples/
#   make clean
#
# Useful variables:
#   make NO_GRAPHICS=1                        memory / input / pixel core only
#   make RAYLIB_DIR=/path/to/raylib/src       raylib headers (raylib.h)
#   make RAYGUI_DIR=/path/to/raygui/src       raygui headers (raygui.h)
#   make GUI_IMPLEMENTATION=1                 compile raygui inside pyMeow
#                                             (also enables gui::scrollBar)
#   make CXX=clang++ CXXFLAGS="-std=c++17 -O3"

CXX      ?= g++
AR       ?= ar
CXXSTD   ?= -std=c++17
OPT      ?= -O2
WARN     ?= -Wall -Wextra
CXXFLAGS ?= $(CXXSTD) $(OPT) $(WARN)

BUILD    := build
INCLUDES := -I.
SOURCES  := $(wildcard src/*.cpp)
OBJECTS  := $(patsubst src/%.cpp,$(BUILD)/obj/%.o,$(SOURCES))

UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)

# ------------------------------------------------------------------ platform
ifeq ($(OS),Windows_NT)
  PYMEOW_PLATFORM := windows
else
  ifeq ($(UNAME_S),Linux)
    PYMEOW_PLATFORM := linux
  else
    PYMEOW_PLATFORM := other
  endif
endif

# Platform libraries that are always required (X11 on Linux, Win32 on Windows).
ifeq ($(PYMEOW_PLATFORM),linux)
  LDLIBS   += -lX11 -lXtst -lXinerama
  SHARED_EXT := .so
  LIBNAME  := libpymeow.a
  SHAREDNAME := libpymeow$(SHARED_EXT)
  PICFLAGS := -fPIC
else ifeq ($(PYMEOW_PLATFORM),windows)
  LDLIBS   += -lgdi32 -luser32 -lpsapi -lwinmm -lole32
  SHARED_EXT := .dll
  LIBNAME  := libpymeow.a
  SHAREDNAME := pymeow$(SHARED_EXT)
  PICFLAGS :=
else
  SHARED_EXT := .so
  LIBNAME  := libpymeow.a
  SHAREDNAME := libpymeow$(SHARED_EXT)
  PICFLAGS := -fPIC
endif

# ------------------------------------------------------------------ graphics
ifeq ($(NO_GRAPHICS),1)
  CPPFLAGS += -DPYMEOW_NO_GRAPHICS
else
  ifneq ($(RAYLIB_DIR),)
    INCLUDES += -I$(RAYLIB_DIR)
  endif
  ifneq ($(RAYGUI_DIR),)
    INCLUDES += -I$(RAYGUI_DIR)
  endif
  ifeq ($(GUI_IMPLEMENTATION),1)
    CPPFLAGS += -DPYMEOW_GUI_IMPLEMENTATION
  endif
  ifeq ($(PYMEOW_PLATFORM),linux)
    LDLIBS += -lraylib -lm -lpthread -ldl -lrt -lGL
  else ifeq ($(PYMEOW_PLATFORM),windows)
    LDLIBS += -lraylib -lopengl32 -lwinmm
  endif
endif

ALL_CXXFLAGS := $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(PICFLAGS)

# -------------------------------------------------------------------- targets
.PHONY: all shared test examples clean run-test info

all: $(BUILD)/$(LIBNAME)

info:
	@echo "platform      : $(PYMEOW_PLATFORM)"
	@echo "graphics      : $(if $(NO_GRAPHICS),disabled (PYMEOW_NO_GRAPHICS),enabled)"
	@echo "sources       : $(SOURCES)"
	@echo "CXXFLAGS      : $(ALL_CXXFLAGS)"
	@echo "LDLIBS        : $(LDLIBS)"

$(BUILD)/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

$(BUILD)/$(LIBNAME): $(OBJECTS)
	@mkdir -p $(BUILD)
	$(AR) rcs $@ $(OBJECTS)
	@echo "built $@"

shared: CPPFLAGS += -DPYMEOW_SHARED
shared: $(BUILD)/$(SHAREDNAME)

$(BUILD)/$(SHAREDNAME): $(OBJECTS)
	@mkdir -p $(BUILD)
	$(CXX) -shared $(PICFLAGS) -o $@ $(OBJECTS) $(LDLIBS)
	@echo "built $@"

# The test suite only needs the memory / input core, it never opens a window.
$(BUILD)/test_core: tests/test_core.cpp $(SOURCES)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -DPYMEOW_NO_GRAPHICS $(INCLUDES) -o $@ tests/test_core.cpp $(SOURCES) \
	    $(filter-out -lraylib,$(LDLIBS))

test: $(BUILD)/test_core
run-test: test
	./$(BUILD)/test_core

# ------------------------------------------------------------------- examples
# These examples only use the memory core and build without raylib.
CORE_EXAMPLES := memory_basics windows/csgo_glow windows/csgo_wallhack

# Only the examples of the current platform are built.
EXAMPLE_SOURCES := $(wildcard examples/*.cpp)
ifeq ($(PYMEOW_PLATFORM),linux)
  EXAMPLE_SOURCES += $(wildcard examples/linux/*.cpp)
else ifeq ($(PYMEOW_PLATFORM),windows)
  EXAMPLE_SOURCES += $(wildcard examples/windows/*.cpp)
endif
# Without raylib only the core examples can be built.
ifeq ($(NO_GRAPHICS),1)
  EXAMPLE_SOURCES := $(filter $(addprefix examples/,$(addsuffix .cpp,$(CORE_EXAMPLES))),$(EXAMPLE_SOURCES))
endif
EXAMPLE_BINS := $(patsubst examples/%.cpp,$(BUILD)/examples/%,$(EXAMPLE_SOURCES))

examples: $(EXAMPLE_BINS)

$(BUILD)/examples/%: examples/%.cpp $(BUILD)/$(LIBNAME)
	@mkdir -p $(dir $@)
	$(CXX) $(ALL_CXXFLAGS) $(if $(filter $*,$(CORE_EXAMPLES)),-DPYMEOW_NO_GRAPHICS,) \
	    -o $@ $< $(BUILD)/$(LIBNAME) $(LDLIBS)

clean:
	rm -rf $(BUILD)
