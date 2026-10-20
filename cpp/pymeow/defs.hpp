/*
 * pyMeow C++ -- build configuration / platform detection
 * https://github.com/qb-0/pyMeow
 *
 * This header is intentionally dependency free (no raylib, no platform headers)
 * so that internal translation units which need conflicting system headers
 * (Xlib defines a global `Font` typedef, raylib defines one as well) can use
 * the pyMeow macros and error types without pulling raylib in.
 */
#ifndef PYMEOW_DEFS_HPP
#define PYMEOW_DEFS_HPP

#define PYMEOW_VERSION_MAJOR 2
#define PYMEOW_VERSION_MINOR 0
#define PYMEOW_VERSION_PATCH 0
#define PYMEOW_VERSION_STRING "2.0.0"

/* ------------------------------------------------------------------ platform */
/* Windows is checked first so the headers can be syntax checked with a cross
 * compilation setup (-D_WIN32) on a non Windows host. */
#if defined(_WIN32) || defined(_WIN64)
#  define PYMEOW_WINDOWS 1
#elif defined(__linux__)
#  define PYMEOW_LINUX 1
#else
#  error "pyMeow C++ supports Windows and Linux only"
#endif

#if defined(PYMEOW_WINDOWS)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#endif

/* The ptrace based helpers (allocateMemory / pageProtection on Linux) implement
 * an x86-64 syscall injection stub. */
#if defined(__x86_64__) || defined(_M_X64)
#  define PYMEOW_ARCH_X64 1
#elif defined(__i386__) || defined(_M_IX86)
#  define PYMEOW_ARCH_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#  define PYMEOW_ARCH_ARM64 1
#endif

/* ------------------------------------------------------------- graphics deps */
/* overlay / render / gui / sound wrap raylib (+ raygui). Define
 * PYMEOW_NO_GRAPHICS to build the memory, input and pixel core only, which then
 * has no third party dependency at all. */
#if !defined(PYMEOW_NO_GRAPHICS) && !defined(PYMEOW_WITH_RAYLIB)
#  define PYMEOW_WITH_RAYLIB 1
#endif

/* Symbol export macro, only needed when pyMeow is built as a shared library. */
#ifndef PYMEOW_API
#  if defined(PYMEOW_SHARED) && defined(PYMEOW_WINDOWS)
#    define PYMEOW_API __declspec(dllexport)
#  elif defined(PYMEOW_SHARED)
#    define PYMEOW_API __attribute__((visibility("default")))
#  else
#    define PYMEOW_API
#  endif
#endif

#endif /* PYMEOW_DEFS_HPP */
