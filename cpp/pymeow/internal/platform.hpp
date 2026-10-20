/*
 * pyMeow C++ -- internal OS abstraction layer
 * https://github.com/qb-0/pyMeow
 *
 * Internal header: not part of the public API.
 *
 * Why this exists: windows.h (GDI `Rectangle()`, `CloseWindow()`, `PlaySound()`,
 * min/max macros) and Xlib.h (global `Font` typedef, Bool/Status/None macros)
 * both collide with raylib, which pyMeow wraps. raylib itself avoids including
 * windows.h for the same reason. So every OS specific call is implemented in
 *   - src/platform_linux.cpp  (X11, /proc, process_vm_readv, ptrace)
 *   - src/platform_win32.cpp  (Win32 API)
 * and the portable modules (memcore, utils, input, pixel, overlay) only talk to
 * this interface. Those two translation units never include raylib.
 *
 * All functions report failure with a return value instead of throwing, the
 * public modules convert a failure into an exception (getOsError() provides the
 * platform error of the last failed call).
 */
#ifndef PYMEOW_INTERNAL_PLATFORM_HPP
#define PYMEOW_INTERNAL_PLATFORM_HPP

#include "../defs.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace pymeow {
namespace detail {

struct OsProcess {
    int pid = 0;
    std::string name;
};

struct OsModule {
    std::string name;
    std::string path;
    std::uint64_t base = 0;
    std::uint64_t size = 0;
};

struct OsModuleRef {
    std::string name;
    std::string path;
    std::uint64_t base = 0;
    std::uint64_t end = 0;
};

struct OsRegion {
    std::uint64_t start = 0;
    std::uint64_t size = 0;
    unsigned int protection = 0; /* Windows: PAGE_* */
    bool readable = true;
    std::string perms;           /* Linux: "r-xp" */
};

/* --------------------------------------------------------------- process */
/* Extra context for the last failed call (may be empty). The public modules
 * append it to the exception message. */
std::string osLastErrorText();

bool osEnumProcesses(std::vector<OsProcess>& out);
bool osProcessAlive(int pid);
bool osProcessPath(int pid, std::string& out);
/* Windows: opens a handle (PROCESS_ALL_ACCESS, falling back to the memory
 * specific rights). Linux: no handle is needed, *handle stays nullptr. */
bool osOpenProcess(int pid, void** handle);
void osCloseProcess(void* handle);
bool osEnumModules(int pid, std::vector<OsModule>& out);
bool osEnumRegions(int pid, void* handle, const OsModuleRef& module, std::vector<OsRegion>& out);
bool osIsProcess64Bit(int pid, void* handle, bool& out);

/* ---------------------------------------------------------------- memory */
bool osReadMemory(int pid, void* handle, std::uint64_t address, void* dst, std::size_t size);
bool osWriteMemory(int pid, void* handle, std::uint64_t address, const void* src, std::size_t size);
/* size == 0 -> the whole region containing `address` (VirtualQueryEx on Windows,
 * the page on Linux). Returns the previous protection in oldProtection. */
bool osSetPageProtection(int pid, void* handle, std::uint64_t address, std::size_t size, int protection, int& oldProtection);
bool osAllocateMemory(int pid, void* handle, std::size_t size, int protection, std::uint64_t& out);
bool osFreeMemory(int pid, void* handle, std::uint64_t address);
bool osModuleProcAddress(const std::string& moduleName, const std::string& functionName, std::uint64_t& out);
bool osCreateRemoteThread(void* handle, std::uint64_t startAddress, std::uint64_t param, std::uint64_t& exitCode);

/* -------------------------------------------------------- screen/window */
bool osDisplayResolution(int& width, int& height);
struct OsWindowRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};
bool osWindowRectByName(const std::string& name, OsWindowRect& out);
bool osWindowTitleByPid(int pid, std::string& out);
void osSetDpiAware();

/* ---------------------------------------------------------------- input */
/* Returns false only when the query itself failed (no display, unmapped key),
 * the actual state is written to the out parameter. */
bool osKeyPressed(int key, bool& pressed);
bool osFakeKey(int key, bool down);
/* button: 0 left, 1 middle, 2 right */
bool osMouseButtonState(int button, bool& down);
bool osFakeMouseButton(int button, bool down);
bool osFakeMouseMove(int x, int y, bool relative);
bool osCursorPosition(int& x, int& y);

/* --------------------------------------------------------------- pixel */
/* Captures a screen region as RGBA8 (width * height * 4 bytes). */
bool osCaptureRegion(int x, int y, int width, int height, std::vector<unsigned char>& rgbaOut);

} /* namespace detail */
} /* namespace pymeow */

#endif /* PYMEOW_INTERNAL_PLATFORM_HPP */
