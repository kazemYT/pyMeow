# pyMeow C++ -- CMake build
#
#   cmake -S cpp -B cpp/build                       # full library (needs raylib)
#   cmake -S cpp -B cpp/build -DPYMEOW_NO_GRAPHICS=ON   # memory core only
#   cmake --build cpp/build -j
#   ctest --test-dir cpp/build                      # run the core test suite
#
# Options:
#   PYMEOW_NO_GRAPHICS      skip overlay / render / gui / sound (no third party deps)
#   PYMEOW_NO_GUI           skip the raygui based widgets only
#   PYMEOW_GUI_IMPLEMENTATION  compile raygui inside pyMeow (needs raygui.h)
#   PYMEOW_BUILD_TESTS      build tests/test_core.cpp (default ON)
#   PYMEOW_BUILD_EXAMPLES   build examples/ (default OFF)
#   PYMEOW_RAYLIB_INCLUDE_DIR / PYMEOW_RAYGUI_INCLUDE_DIR
#   PYMEOW_RAYLIB_LIBRARY   (only used when find_package(raylib) does not find it)
#
# Consuming pyMeow from another project (imgui style):
#   add_subdirectory(third_party/pyMeow/cpp)
#   target_link_libraries(my_cheat PRIVATE pymeow)

cmake_minimum_required(VERSION 3.16)
project(pymeow VERSION 2.0.0 LANGUAGES CXX)

option(PYMEOW_NO_GRAPHICS "Build without raylib (memory / input / pixel core only)" OFF)
option(PYMEOW_NO_GUI "Build without the raygui widgets" OFF)
option(PYMEOW_GUI_IMPLEMENTATION "Compile raygui inside pyMeow (requires raygui.h)" OFF)
option(PYMEOW_BUILD_TESTS "Build the pyMeow test suite" ON)
option(PYMEOW_BUILD_EXAMPLES "Build the pyMeow examples" OFF)

set(PYMEOW_SOURCES
    src/aob.cpp
    src/gui.cpp
    src/input.cpp
    src/memcore.cpp
    src/memop.cpp
    src/overlay.cpp
    src/pixel.cpp
    src/platform_linux.cpp
    src/platform_win32.cpp
    src/png.cpp
    src/ptrace.cpp
    src/render.cpp
    src/sound.cpp
    src/utils.cpp
)

add_library(pymeow ${PYMEOW_SOURCES})
add_library(pymeow::pymeow ALIAS pymeow)

target_include_directories(pymeow PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(pymeow PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(pymeow PRIVATE /W3 /utf-8)
else()
    target_compile_options(pymeow PRIVATE -Wall -Wextra)
endif()

# ------------------------------------------------------------------ graphics
if(PYMEOW_NO_GRAPHICS)
    target_compile_definitions(pymeow PUBLIC PYMEOW_NO_GRAPHICS)
else()
    if(PYMEOW_NO_GUI)
        target_compile_definitions(pymeow PUBLIC PYMEOW_NO_GUI)
    endif()
    if(PYMEOW_GUI_IMPLEMENTATION)
        target_compile_definitions(pymeow PRIVATE PYMEOW_GUI_IMPLEMENTATION)
    endif()

    # raylib: use find_package when available, otherwise accept plain paths.
    find_package(raylib QUIET)
    if(raylib_FOUND)
        target_link_libraries(pymeow PUBLIC raylib)
    else()
        if(PYMEOW_RAYLIB_INCLUDE_DIR)
            target_include_directories(pymeow PUBLIC ${PYMEOW_RAYLIB_INCLUDE_DIR})
        endif()
        if(PYMEOW_RAYGUI_INCLUDE_DIR)
            target_include_directories(pymeow PUBLIC ${PYMEOW_RAYGUI_INCLUDE_DIR})
        endif()
        if(PYMEOW_RAYLIB_LIBRARY)
            target_link_libraries(pymeow PUBLIC ${PYMEOW_RAYLIB_LIBRARY})
        else()
            target_link_libraries(pymeow PUBLIC raylib)
        endif()
    endif()
endif()

# ------------------------------------------------------------------ platform
if(WIN32)
    target_link_libraries(pymeow PUBLIC gdi32 user32 psapi winmm ole32)
    if(NOT PYMEOW_NO_GRAPHICS)
        target_link_libraries(pymeow PUBLIC opengl32)
    endif()
elseif(UNIX AND NOT APPLE)
    # X11 is used by the platform layer (input injection, screen capture,
    # window lookup). Set PYMEOW_X11_INCLUDE_DIR when the headers are not in
    # the default include path.
    if(PYMEOW_X11_INCLUDE_DIR)
        target_include_directories(pymeow PUBLIC ${PYMEOW_X11_INCLUDE_DIR})
    endif()
    find_library(X11_LIB NAMES X11)
    find_library(XTST_LIB NAMES Xtst)
    find_library(XINERAMA_LIB NAMES Xinerama)
    if(X11_LIB AND XTST_LIB AND XINERAMA_LIB)
        target_link_libraries(pymeow PUBLIC ${X11_LIB} ${XTST_LIB} ${XINERAMA_LIB})
    else()
        target_link_libraries(pymeow PUBLIC X11 Xtst Xinerama)
    endif()
    if(NOT PYMEOW_NO_GRAPHICS)
        target_link_libraries(pymeow PUBLIC m pthread dl rt)
    endif()
endif()

# A graphics free build of the same sources: used by the test suite and by the
# examples that do not draw anything, so those never need raylib.
if(NOT PYMEOW_NO_GRAPHICS)
    add_library(pymeow_core STATIC ${PYMEOW_SOURCES})
    target_include_directories(pymeow_core PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_compile_features(pymeow_core PUBLIC cxx_std_17)
    target_compile_definitions(pymeow_core PUBLIC PYMEOW_NO_GRAPHICS)
    if(WIN32)
        target_link_libraries(pymeow_core PUBLIC gdi32 user32 psapi winmm ole32)
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(pymeow_core PUBLIC X11 Xtst Xinerama)
    endif()
else()
    add_library(pymeow_core ALIAS pymeow)
endif()

# --------------------------------------------------------------------- tests
if(PYMEOW_BUILD_TESTS)
    enable_testing()
    add_executable(pymeow_test_core tests/test_core.cpp)
    target_link_libraries(pymeow_test_core PRIVATE pymeow_core)
    add_test(NAME core COMMAND pymeow_test_core)
endif()

# ------------------------------------------------------------------ examples
if(PYMEOW_BUILD_EXAMPLES)
    file(GLOB PYMEOW_EXAMPLE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/linux/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/windows/*.cpp
    )
    foreach(example_source ${PYMEOW_EXAMPLE_SOURCES})
        get_filename_component(example_name ${example_source} NAME_WE)
        # the windows / linux example folders only build on their own platform
        if(example_source MATCHES "/examples/windows/" AND NOT WIN32)
            continue()
        endif()
        if(example_source MATCHES "/examples/linux/" AND WIN32)
            continue()
        endif()
        add_executable(${example_name} ${example_source})
        if(example_name MATCHES "csgo_glow|csgo_wallhack|memory_basics")
            # memory core only -> no raylib required
            target_link_libraries(${example_name} PRIVATE pymeow_core)
        else()
            target_link_libraries(${example_name} PRIVATE pymeow)
        endif()
    endforeach()
endif()

# ------------------------------------------------------------------- install
include(GNUInstallDirs)
install(TARGETS pymeow EXPORT pymeowTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES pymeow.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY pymeow/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pymeow
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.inc")
install(EXPORT pymeowTargets NAMESPACE pymeow:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pymeow)
