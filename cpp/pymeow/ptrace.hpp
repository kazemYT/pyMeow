/*
 * pyMeow C++ -- ptrace based syscall injection (Linux, port of ptrace.nim)
 * https://github.com/qb-0/pyMeow
 *
 * Credits of the original approach:
 *   - https://github.com/ba0f3/ptrace.nim
 *   - https://guidedhacking.com/members/obdr.128625/
 *
 * Attaches to the target, replaces the instruction at RIP with `syscall; nop...`,
 * sets the registers, single steps and restores everything afterwards.
 * x86-64 only, and it needs ptrace permission (usually root).
 */
#ifndef PYMEOW_PTRACE_HPP
#define PYMEOW_PTRACE_HPP

#include "defs.hpp"

#if defined(PYMEOW_LINUX)

#include <cstddef>
#include <cstdint>

namespace pymeow {
namespace ptrace {

/* Injects a syscall into the target process and returns its result (rax).
 * Throws Error when the process can not be traced or the syscall failed. */
PYMEOW_API std::uintptr_t injectSyscall(int pid, long syscallNumber, std::uintptr_t arg0 = 0, std::uintptr_t arg1 = 0,
                                        std::uintptr_t arg2 = 0, std::uintptr_t arg3 = 0, std::uintptr_t arg4 = 0,
                                        std::uintptr_t arg5 = 0);

/* mprotect() on the page containing `address` (PROT_* values). */
PYMEOW_API void pageProtection(int pid, std::uintptr_t address, int protection);
/* mmap() inside the target process, returns the remote address. */
PYMEOW_API std::uintptr_t allocateMemory(int pid, std::size_t size, int protection);
/* munmap() of a region previously returned by allocateMemory(). */
PYMEOW_API bool freeMemory(int pid, std::uintptr_t address);

/* True when the current process is allowed to ptrace other processes
 * (root, or /proc/sys/kernel/yama/ptrace_scope permits it). */
PYMEOW_API bool canTrace();

} /* namespace ptrace */
} /* namespace pymeow */

#endif /* PYMEOW_LINUX */

#endif /* PYMEOW_PTRACE_HPP */
