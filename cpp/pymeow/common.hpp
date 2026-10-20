/*
 * pyMeow C++
 * https://github.com/qb-0/pyMeow
 *
 * Common definitions shared by every pyMeow module.
 * This is a native C++ port of the Nim/Python pyMeow library, designed to be
 * dropped into a project the same way imgui is: add the headers to your include
 * path and compile the sources in `src/` together with your project.
 *
 * MIT License (see LICENSE in the repository root)
 */
#ifndef PYMEOW_COMMON_HPP
#define PYMEOW_COMMON_HPP

#include "defs.hpp"

/* raylib is only needed for the graphics modules (overlay / render / gui /
 * sound) and for the implicit Color / Vec2 / Vec3 / Rect conversions. */
#if defined(PYMEOW_WITH_RAYLIB)
#  if defined(__has_include)
#    if __has_include(<raylib.h>)
#      include <raylib.h>
#    elif __has_include("raylib.h")
#      include "raylib.h"
#    else
#      error "pyMeow C++: raylib.h not found. Install raylib, add it to your include path, or define PYMEOW_NO_GRAPHICS to build the memory/input core only."
#    endif
#  else
#    include <raylib.h>
#  endif
#endif

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace pymeow {

/* Remote address inside the target process. */
using Address = std::uintptr_t;
using byte = std::uint8_t;

PYMEOW_API const char* version();
PYMEOW_API const char* systemName();

inline constexpr Address invalidAddress = static_cast<Address>(0);

} /* namespace pymeow */

/* Short alias, the examples use `pm::` like the Python examples use `pm.` */
#if !defined(PYMEOW_NO_PM_ALIAS)
namespace pm = pymeow;
#endif

#endif /* PYMEOW_COMMON_HPP */
