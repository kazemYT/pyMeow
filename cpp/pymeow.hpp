/*
 * pyMeow C++ -- umbrella header
 * https://github.com/qb-0/pyMeow
 *
 * A native C++ port of pyMeow (Nim/Python): external process memory access,
 * AOB scanning, a transparent raylib overlay, drawing helpers, an immediate
 * mode GUI (raygui), input injection and pixel scanning for Windows and Linux.
 *
 * Usage (imgui style): add this directory to your include path, compile the
 * sources in `src/` with your project and
 *
 *     #include <pymeow.hpp>
 *
 *     int main() {
 *         pm::Process proc = pm::openProcess("ac_client.exe");
 *         pm::Module mod   = pm::getModule(proc, "ac_client.exe");
 *         int health       = pm::readInt(proc, mod.base + 0xEC);
 *     }
 *
 * Build flavours:
 *   default              full library, requires raylib (+ raygui for gui.hpp)
 *   PYMEOW_NO_GRAPHICS   memory / input / pixel core only, no third party deps
 *   PYMEOW_NO_GUI        skip the raygui based widgets
 *
 * Optional: #include <pymeow/pycompat.hpp> adds the snake_case names of the
 * Python module (pm::open_process, pm::r_int, pm::gui_button, ...).
 * See cpp/README.md and cpp/cheatsheet.txt.
 */
#ifndef PYMEOW_HPP
#define PYMEOW_HPP

#include "pymeow/common.hpp"
#include "pymeow/error.hpp"
#include "pymeow/types.hpp"
#include "pymeow/vec.hpp"
#include "pymeow/keys.hpp"
#include "pymeow/memcore.hpp"
#include "pymeow/memop.hpp"
#include "pymeow/utils.hpp"
#include "pymeow/input.hpp"
#include "pymeow/pixel.hpp"

#if defined(PYMEOW_LINUX)
#  include "pymeow/ptrace.hpp"
#endif

#if defined(PYMEOW_WITH_RAYLIB)
#  include "pymeow/overlay.hpp"
#  include "pymeow/render.hpp"
#  include "pymeow/sound.hpp"
#  include "pymeow/gui.hpp" /* does nothing when raygui.h is not available */
#endif

#endif /* PYMEOW_HPP */
