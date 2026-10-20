# pyMeow C++

A **native C++17 port of [pyMeow](https://github.com/qb-0/pyMeow)** — the Nim/Python library for
external process memory access, AOB scanning, transparent overlays, drawing, an immediate mode GUI,
input injection and pixel scanning on **Windows and Linux**.

This folder is completely separate from the Nim library in the repository root: the Nim sources,
`pyMeow.nim`, `cheatsheet.txt` and `examples/` are untouched.

```
cpp/
├── pymeow.hpp              umbrella header  ->  #include <pymeow.hpp>
├── pymeow/                 public headers (one per module)
│   ├── defs.hpp            version, platform detection, export macros (dependency free)
│   ├── common.hpp          Address type, version(), systemName(), raylib include
│   ├── error.hpp           exception hierarchy + getOsError()
│   ├── types.hpp           Vec2 / Vec3 / Color / Rect / Pixel / Matrix (+ raylib conversions)
│   ├── vec.hpp             vector math (port of vec.nim, header only)
│   ├── keys.hpp            portable key codes (VK_* on Windows, XK_* on Linux)
│   ├── memcore.hpp         processes, modules, memory regions, raw read/write, AOB scan
│   ├── memop.hpp           typed read/write, strings, pointer chains, dynamic r()/w()
│   ├── utils.hpp           colors, window/screen info, timing, worldToScreen
│   ├── input.hpp           keyboard & mouse (state, injection)
│   ├── pixel.hpp           screen capture, pixel search, PNG export
│   ├── ptrace.hpp          Linux only: remote syscalls, mmap, mprotect
│   ├── overlay.hpp         transparent raylib overlay window
│   ├── render.hpp          drawing, textures, fonts, collisions
│   ├── gui.hpp             raygui widgets (namespace pymeow::gui)
│   ├── sound.hpp           sound playback (namespace pymeow::sound)
│   ├── pycompat.hpp        optional: the snake_case names of the Python module
│   └── internal/           implementation details (aob, png, platform, strings, debug)
├── src/                    implementation, compile these with your project
├── examples/               ports of the Python examples
├── tests/test_core.cpp     self contained test suite (no graphics needed)
├── CMakeLists.txt          CMake build (library, tests, examples, install)
└── Makefile                plain GNU make build, no cmake required
```

---

## Quick start (imgui style)

There is nothing to install and no build system required:

1. copy (or `git submodule` / `add_subdirectory`) the `cpp/` folder into your project,
2. add it to your include path,
3. compile the sources in `cpp/src/` together with your code,
4. `#include <pymeow.hpp>`.

```cpp
#include <pymeow.hpp>
#include <cstdio>

int main() {
    try {
        pm::Process proc = pm::openProcess("ac_client.exe");          // or a pid
        pm::Module  mod  = pm::getModule(proc, "ac_client.exe");

        int health = pm::readInt(proc, mod.base + 0xEC);              // typed read
        pm::writeFloat(proc, mod.base + 0xF0, 1337.0f);               // typed write

        auto hits = pm::aobScanModule(proc, "ac_client.exe", "89 01 48 8B 43");
        if (!hits.empty()) std::printf("found at 0x%llx\n", (unsigned long long)hits[0]);

        pm::closeProcess(proc);
    } catch (const pm::Error& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
```

`pm::` is an alias for `namespace pymeow` (disable it with `PYMEOW_NO_PM_ALIAS`).

An overlay ESP is just as short:

```cpp
pm::Process proc = pm::openProcess("linux_64_client");
pm::Address base = pm::getModule(proc, "linux_64_client").base;

pm::overlayInit("AssaultCube", /*fps = */ 144);
while (pm::overlayLoop()) {
    pm::beginDrawing();
    pm::drawFps(0, 0);

    pm::Matrix matrix{};
    const std::vector<float> vm = pm::readFloats(proc, base + 0x1B4FCC, 16);
    std::copy(vm.begin(), vm.end(), matrix.begin());

    pm::Vec3 pos = pm::readVec3(proc, pm::readInt(proc, base + 0x1A3518) + 0x8);
    pm::WtsResult screen = pm::worldToScreenNoExc(matrix, pos);
    if (screen.inBounds) {
        pm::drawCircle(screen.pos.x, screen.pos.y, 6.0f, pm::getColor("cyan"));
        pm::drawText("local player", screen.pos.x, screen.pos.y, 15.0f, pm::colors::white);
    }
    pm::endDrawing();
}
```

---

## Requirements

|                | Linux                                     | Windows                          |
| -------------- | ----------------------------------------- | -------------------------------- |
| Compiler       | g++ 9+ / clang 10+ with `-std=c++17`      | MSVC 2019+ / MinGW-w64 (C++17)   |
| Always needed  | libX11, libXtst, libXinerama (headers+libs) | gdi32, user32, psapi, winmm    |
| Graphics       | [raylib](https://github.com/raysan5/raylib) 5.x, OpenGL | raylib 5.x            |
| GUI widgets    | [raygui](https://github.com/raysan5/raygui) 4.0 or newer | raygui 4.0+          |

Debian/Ubuntu: `sudo apt install libx11-dev libxtst-dev libxinerama-dev libgl1-mesa-dev`
Arch: `sudo pacman -S libx11 libxtst libxinerama`

* The **memory / input / pixel core works without raylib** — build with `PYMEOW_NO_GRAPHICS`.
* raylib and raygui are only needed for `overlay.hpp`, `render.hpp`, `gui.hpp`, `sound.hpp`.
* On Linux the overlay needs an X11 display (XWayland works, pure Wayland does not).

---

## Building

### Manual (the imgui way)

```sh
# memory core only, no third party dependency
g++ -std=c++17 -DPYMEOW_NO_GRAPHICS -Icpp -o my_tool my_tool.cpp cpp/src/*.cpp \
    -lX11 -lXtst -lXinerama

# full library (raylib + raygui headers on the include path)
g++ -std=c++17 -Icpp -I/path/to/raylib/src -I/path/to/raygui/src -o my_esp my_esp.cpp \
    cpp/src/*.cpp -lX11 -lXtst -lXinerama -lraylib -lm -lpthread -ldl -lrt -lGL
```

```bat
:: Windows (MSVC), memory core only
cl /EHsc /std:c++17 /DPYMEOW_NO_GRAPHICS /Icpp my_tool.cpp cpp\src\*.cpp ^
   psapi.lib user32.lib gdi32.lib

:: Windows, full library
cl /EHsc /std:c++17 /Icpp /I%RAYLIB%\src /I%RAYGUI%\src my_esp.cpp cpp\src\*.cpp ^
   raylib.lib opengl32.lib gdi32.lib user32.lib psapi.lib winmm.lib
```

### Makefile

```sh
cd cpp
make                      # static library  -> build/libpymeow.a
make shared               # shared library  -> build/libpymeow.so / pymeow.dll
make test && make run-test# core test suite (no graphics, no raylib needed)
make examples             # build the examples of your platform
make info                 # show the detected configuration

make NO_GRAPHICS=1                                   # core only
make RAYLIB_DIR=~/raylib/src RAYGUI_DIR=~/raygui/src # header locations
make GUI_IMPLEMENTATION=1                            # compile raygui into pyMeow
```

### CMake

```sh
cmake -S cpp -B cpp/build -DPYMEOW_BUILD_EXAMPLES=ON
cmake --build cpp/build -j
ctest --test-dir cpp/build
```

```cmake
# from your own project
add_subdirectory(third_party/pyMeow/cpp)
target_link_libraries(my_cheat PRIVATE pymeow)       # or pymeow_core (no graphics)
```

Options: `PYMEOW_NO_GRAPHICS`, `PYMEOW_NO_GUI`, `PYMEOW_GUI_IMPLEMENTATION`,
`PYMEOW_BUILD_TESTS`, `PYMEOW_BUILD_EXAMPLES`, `PYMEOW_RAYLIB_INCLUDE_DIR`,
`PYMEOW_RAYGUI_INCLUDE_DIR`, `PYMEOW_RAYLIB_LIBRARY`, `PYMEOW_X11_INCLUDE_DIR`.
`make install` installs the headers, the library and a `pymeow::pymeow` CMake package.

### Compile time macros

| Macro                       | Effect                                                                    |
| --------------------------- | ------------------------------------------------------------------------- |
| `PYMEOW_NO_GRAPHICS`        | build only the memory / input / pixel core; raylib is not included at all |
| `PYMEOW_NO_GUI`             | skip the raygui widgets (overlay / render / sound stay available)         |
| `PYMEOW_GUI_IMPLEMENTATION` | compile raygui inside `gui.cpp` (define it in exactly one build)          |
| `PYMEOW_SHARED`             | export the public symbols (shared library build)                          |
| `PYMEOW_NO_PM_ALIAS`        | do not create the `namespace pm = pymeow;` alias                          |

Detected automatically: `PYMEOW_LINUX` / `PYMEOW_WINDOWS`, `PYMEOW_ARCH_X64` / `_X86` / `_ARM64`,
`PYMEOW_WITH_RAYLIB`, `PYMEOW_WITH_GUI` (raygui.h found), `PYMEOW_RAYGUI_5`,
`PYMEOW_RAYLIB_ROUNDED_LINES_EX` (raylib 5.5+ changed `DrawRectangleRoundedLines`).

---

## Modules

| Header        | Namespace          | Python module          | Contents                                                            |
| ------------- | ------------------ | ---------------------- | ------------------------------------------------------------------- |
| `memcore.hpp` | `pymeow`           | `memcore`              | processes, modules, regions, raw transfers, AOB scan, injection      |
| `memop.hpp`   | `pymeow`           | `memop`                | typed reads/writes, strings, pointer chains, dynamic `r()`/`w()`     |
| `ptrace.hpp`  | `pymeow::ptrace`   | `ptrace` (Linux)       | remote syscalls, `mmap`/`munmap`/`mprotect` through ptrace           |
| `utils.hpp`   | `pymeow`           | `utils`                | 140 color names, window/screen info, timing, `worldToScreen`         |
| `vec.hpp`     | `pymeow`           | `vec`                  | vec2/vec3 math (header only)                                         |
| `keys.hpp`    | `pymeow::keys`     | —                      | portable key codes (`keys::End` is VK_END / XK_End)                  |
| `input.hpp`   | `pymeow`           | `input`                | key/mouse state and injection                                        |
| `pixel.hpp`   | `pymeow`           | `pixel`                | screen capture, color search, PNG export                             |
| `overlay.hpp` | `pymeow`           | `overlay`              | transparent click-through overlay window                             |
| `render.hpp`  | `pymeow`           | `render`               | shapes, text, textures, fonts, collision helpers                     |
| `gui.hpp`     | `pymeow::gui`      | `gui`                  | raygui widgets with per-id state                                     |
| `sound.hpp`   | `pymeow::sound`    | `sound`                | sound loading and playback                                           |
| `error.hpp`   | `pymeow`           | —                      | `Error`, `NotFoundError`, `MemoryError`, `OutOfBoundsError`, ...     |
| `pycompat.hpp`| `pymeow`           | *all Python names*     | optional `r_int` / `w_bytes` / `gui_button` / `load_sound` aliases   |

`cpp/cheatsheet.txt` lists every function with its full signature.

### Python → C++

| Python                                  | C++                                              |
| --------------------------------------- | ------------------------------------------------ |
| `proc = pm.open_process("ac_client.exe")` | `pm::Process proc = pm::openProcess("ac_client.exe");` |
| `base = pm.get_module(proc, m)["base"]` | `pm::Address base = pm::getModule(proc, m).base;`  |
| `pm.r_int(proc, addr)`                  | `pm::readInt(proc, addr)`                        |
| `pm.r(proc, addr, "float", 16)`         | `pm::readFloats(proc, addr, 16)`                 |
| `pm.w_bytes(proc, addr, [0x90, 0x90])`  | `pm::writeBytes(proc, addr, {0x90, 0x90});`      |
| `pm.r_ctype(proc, addr, GlowStruct())`  | `pm::read<GlowStruct>(proc, addr)`               |
| `pm.w_ctype(proc, addr, glow)`          | `pm::write<GlowStruct>(proc, addr, glow)`        |
| `pm.aob_scan_module(proc, m, patt)`     | `pm::aobScanModule(proc, m, patt)`               |
| `pm.pointer_chain_64(proc, b, [0, 8])`  | `pm::pointerChain64(proc, b, {0, 8})`            |
| `pm.overlay_init(target="AC", fps=144)` | `pm::overlayInit("AC", 144);`                    |
| `pm.gui_button(0, 0, 100, 50, "hi")`    | `pm::gui::button(0, 0, 100, 50, "hi")`           |
| `pm.get_color("cyan")`                  | `pm::getColor("cyan")`                           |
| `pm.world_to_screen(matrix, pos)`       | `pm::worldToScreen(matrix, pos)` (overlay size)  |
| exceptions                              | `pm::Error` and subclasses                       |

Prefer the original names? Include `pymeow/pycompat.hpp` and keep writing
`pm::r_int(...)`, `pm::w_bytes(...)`, `pm::gui_button(...)`, `pm::load_sound(...)` — every alias
forwards to the C++ function, so all overloads and default arguments keep working.

---

## Behaviour notes / differences to the Python module

The port keeps the semantics (and the quirks) of the Nim sources, with these documented exceptions:

* **Errors are exceptions.** `pm::Error` is the base class; `NotFoundError`, `MemoryError`
  (carries `address` and the OS `code`), `OutOfBoundsError` and `InvalidArgumentError` derive from it.
  `tryReadPointer()` / `tryWritePointer()` / `tryParseColor()` / `worldToScreenNoExc()` are the
  non-throwing variants.
* **`gui::checkBox()` / `gui::toggle()` return the new state** (`true`/`false`), not raygui's result
  code — that is what a checkbox means in C++.
* **`sound::playMulti()`** uses raylib sound aliases: raylib removed `PlaySoundMulti()` after 3.x.
* **`gui::scrollBar()`** calls raygui's internal `GuiScrollBar()` when pyMeow compiles raygui itself
  (`PYMEOW_GUI_IMPLEMENTATION`), otherwise it falls back to `GuiSliderBar()` (raygui made the
  scroll bar `static` in 3.0).
* **`pageProtection()`** returns the previous protection on Windows and `0` on Linux (mprotect does
  not report it). On Linux the region protection of `enumMemoryRegions()` is reported with the
  Windows `PAGE_*` values (mapped from the `/proc/pid/maps` permissions) so the same filter code
  works on both platforms; the raw string stays in `MemoryRegion::perms`.
* **`freeMemory()`** works on Linux too (through a remote `munmap` syscall, see `ptrace.hpp`),
  the Nim module only printed a message.
* **Windows-only functions** (`getProcAddress`, `createRemoteThread`, `injectLibrary`,
  `injectShellcode`) throw `pm::Error` on Linux instead of silently returning `false`.
* **Drawing uses float coordinates**; the Nim module truncated some positions to `int`.
* **The AOB scanner** accepts `??`, `?A` and `A?` nibble wildcards, is case insensitive, validates
  the pattern (throws `InvalidArgumentError` instead of scanning garbage) and does not suffer from
  the `aob1` index bug of the Nim implementation (which could delete index `-1` for patterns like
  `"?? ?? AA"`).
* **`worldToScreen()`** keeps the original mapping
  `x = (w/2*ndc.x) + (ndc.x + w/2)`, `y = -(h/2*ndc.y) + (ndc.y + h/2)` — including its asymmetry,
  so ESP positions match the Python module pixel by pixel. Algo `0` is the column-major layout used
  by most games, algo `1` the transposed one. Positions with `clip.w < 0.2` are out of bounds.
* **`pixelSaveToFile()` / `savePixelsAsPng()`** write the PNG themselves (8-bit RGBA, zlib stored
  blocks), so the pixel module needs neither raylib nor libpng.
* **`setRequireRoot(true)`** restores the Python behaviour of refusing memory access without uid 0.
  It is off by default because `process_vm_readv()` also works for processes of the same user.
* **`getProcessId()` / `openProcess(name)`** match in both directions (`"ac_client"` finds
  `"ac_client.exe"` and vice versa); the Nim module only checked `process.name in processName`.

### Header conflicts (important)

`windows.h` and `X11/Xlib.h` cannot be included in the same translation unit as raylib
(GDI declares a `Rectangle()` *function* while raylib declares a `Rectangle` *type*, Xlib declares a
global `Font` typedef, both define `min`/`max`/`PlaySound`/...). raylib itself avoids `windows.h`
for the same reason.

pyMeow therefore keeps **all** OS calls in two translation units that never see raylib:
`src/platform_win32.cpp` and `src/platform_linux.cpp` (they implement `pymeow/internal/platform.hpp`).
For your own code the same rule applies: include `<pymeow.hpp>` *or* `<windows.h>` / `<X11/Xlib.h>`
per translation unit, and keep the OS specific code in its own `.cpp`.
`PYMEOW_NO_GRAPHICS` builds never include raylib, so mixing with `windows.h` is fine there.

### Linux permissions

* Reading/writing **your own** or **same-user** processes works unprivileged (`process_vm_readv`).
* Other users' processes need root (or `CAP_SYS_PTRACE`).
* `ptrace.hpp` features (remote `mmap`, `mprotect`, syscalls) attach with `PTRACE_ATTACH`, so they
  need root and `kernel.yama.ptrace_scope <= 1`. They cannot be used on the own process.

---

## Examples

| File                                | Python original                       | Needs graphics |
| ----------------------------------- | ------------------------------------- | -------------- |
| `examples/memory_basics.cpp`        | —                                     | no             |
| `examples/overlay_test.cpp`         | `examples/overlay_test.py`            | yes            |
| `examples/pixel_scan.cpp`           | `examples/pixel_scan.py`              | yes            |
| `examples/linux/ac_esp.cpp`         | `examples/linux/ac_esp.py`            | yes            |
| `examples/linux/sauerbraten_esp.cpp`| `examples/linux/sauerbraten_esp.py`   | yes            |
| `examples/windows/ac_esp.cpp`       | `examples/windows/ac_esp.py`          | yes            |
| `examples/windows/csgo_glow.cpp`    | `examples/windows/csgo_glow.py`       | no             |
| `examples/windows/csgo_wallhack.cpp`| `examples/windows/csgo_wallhack.py`   | no             |

## Tests

`tests/test_core.cpp` is a self contained suite (no test framework, no window, no target process —
it reads and writes its own memory). It covers colors, vector math, `worldToScreen`, timing, the AOB
scanner (both algorithms, wildcards, nibble masks), process/module/region enumeration, every typed
read and write, pointer chains, the dynamic `r()`/`w()` API and the PNG encoder (chunk layout, CRC32,
adler32 and a full pixel round trip).

```sh
cd cpp && make run-test          # or: ctest --test-dir build
```

## License

MIT — the same license as pyMeow (see `../LICENSE`).
