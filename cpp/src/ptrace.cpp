/*
 * pyMeow C++ -- ptrace based syscall injection (Linux implementation)
 * https://github.com/qb-0/pyMeow
 */
#include "pymeow/ptrace.hpp"

#if defined(PYMEOW_LINUX)

#  include "pymeow/error.hpp"
#  include "pymeow/internal/strings.hpp"

#  include <cerrno>
#  include <cstdio>
#  include <cstdlib>
#  include <cstring>
#  include <fstream>
#  include <map>
#  include <mutex>
#  include <string>

#  include <sys/mman.h>
#  include <sys/ptrace.h>
#  include <sys/syscall.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#  if defined(PYMEOW_ARCH_X64)
#    include <sys/user.h>
#  endif

namespace pymeow {
namespace ptrace {

namespace {

std::mutex gAllocationMutex;
std::map<int, std::map<std::uintptr_t, std::size_t>> gAllocations; /* pid -> address -> size */

std::string osErrorText() {
    const int code = errno;
    const char* msg = std::strerror(code);
    return std::string("[Error: ") + std::to_string(code) + " - " + (msg ? msg : "") + "]";
}

void requireArchitecture() {
#  if !defined(PYMEOW_ARCH_X64)
    throw Error("ptrace syscall injection currently supports x86-64 processes only");
#  endif
}

/* Finds the mapping that contains `address` in /proc/<pid>/maps. */
bool findMapping(int pid, std::uintptr_t address, std::uintptr_t& start, std::uintptr_t& end) {
    std::ifstream maps(("/proc/" + std::to_string(pid) + "/maps").c_str());
    if (!maps.is_open()) return false;
    std::string line;
    while (std::getline(maps, line)) {
        const std::size_t dash = line.find('-');
        const std::size_t space = line.find(' ', dash == std::string::npos ? 0 : dash);
        if (dash == std::string::npos || space == std::string::npos) continue;
        char* endPtr = nullptr;
        const unsigned long long s = std::strtoull(line.substr(0, dash).c_str(), &endPtr, 16);
        const unsigned long long e = std::strtoull(line.substr(dash + 1, space - dash - 1).c_str(), &endPtr, 16);
        if (e == 0) continue;
        if (address >= static_cast<std::uintptr_t>(s) && address < static_cast<std::uintptr_t>(e)) {
            start = static_cast<std::uintptr_t>(s);
            end = static_cast<std::uintptr_t>(e);
            return true;
        }
    }
    return false;
}

#  if defined(PYMEOW_ARCH_X64)
long doPtrace(enum __ptrace_request request, int pid, void* addr, void* data) {
    errno = 0;
    return ::ptrace(request, static_cast<pid_t>(pid), addr, data);
}
#  endif

} /* namespace */

bool canTrace() {
    if (::geteuid() == 0) return true;
    std::ifstream yama("/proc/sys/kernel/yama/ptrace_scope");
    if (!yama.is_open()) return true; /* no Yama LSM: ptrace is allowed for the same uid */
    int scope = 1;
    yama >> scope;
    return scope == 0;
}

std::uintptr_t injectSyscall(int pid, long syscallNumber, std::uintptr_t arg0, std::uintptr_t arg1, std::uintptr_t arg2,
                             std::uintptr_t arg3, std::uintptr_t arg4, std::uintptr_t arg5) {
    requireArchitecture();
#  if defined(PYMEOW_ARCH_X64)
    struct user_regs_struct oldRegs;
    struct user_regs_struct regs;
    std::memset(&oldRegs, 0, sizeof(oldRegs));
    std::memset(&regs, 0, sizeof(regs));

    if (doPtrace(PTRACE_ATTACH, pid, nullptr, nullptr) == -1)
        throw Error("ptrace attach failed [Pid: " + std::to_string(pid) + "] " + osErrorText() +
                    " (root or CAP_SYS_PTRACE required)");

    int status = 0;
    if (::waitpid(static_cast<pid_t>(pid), &status, 0) == -1) {
        doPtrace(PTRACE_DETACH, pid, nullptr, nullptr);
        throw Error("ptrace waitpid failed [Pid: " + std::to_string(pid) + "] " + osErrorText());
    }
    if (doPtrace(PTRACE_GETREGS, pid, nullptr, &oldRegs) == -1) {
        doPtrace(PTRACE_DETACH, pid, nullptr, nullptr);
        throw Error("PTRACE_GETREGS failed [Pid: " + std::to_string(pid) + "] " + osErrorText());
    }

    regs = oldRegs;
    regs.rax = static_cast<unsigned long long>(syscallNumber);
    regs.rdi = static_cast<unsigned long long>(arg0);
    regs.rsi = static_cast<unsigned long long>(arg1);
    regs.rdx = static_cast<unsigned long long>(arg2);
    regs.r10 = static_cast<unsigned long long>(arg3);
    regs.r8 = static_cast<unsigned long long>(arg4);
    regs.r9 = static_cast<unsigned long long>(arg5);

    const std::uintptr_t injectionAddress = static_cast<std::uintptr_t>(regs.rip);
    const long oldData = doPtrace(PTRACE_PEEKDATA, pid, reinterpret_cast<void*>(injectionAddress), nullptr);
    if (oldData == -1 && errno != 0) {
        doPtrace(PTRACE_DETACH, pid, nullptr, nullptr);
        throw Error("PTRACE_PEEKDATA failed [Address: 0x" + detail::toHex(injectionAddress) + "] " + osErrorText());
    }

    /* syscall; nop; nop; nop; nop; nop; nop */
    static const std::uint8_t stub[8] = {0x0F, 0x05, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
    long stubWord = 0;
    std::memcpy(&stubWord, stub, sizeof(stubWord));

    doPtrace(PTRACE_POKEDATA, pid, reinterpret_cast<void*>(injectionAddress), reinterpret_cast<void*>(stubWord));
    doPtrace(PTRACE_SETREGS, pid, nullptr, &regs);
    doPtrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);
    ::waitpid(static_cast<pid_t>(pid), &status, 0);
    doPtrace(PTRACE_GETREGS, pid, nullptr, &regs);

    const long long result = static_cast<long long>(regs.rax);

    /* Restore the original instruction and registers before leaving. */
    doPtrace(PTRACE_POKEDATA, pid, reinterpret_cast<void*>(injectionAddress), reinterpret_cast<void*>(oldData));
    doPtrace(PTRACE_SETREGS, pid, nullptr, &oldRegs);
    doPtrace(PTRACE_DETACH, pid, nullptr, nullptr);

    if (result < 0 && result > -4096) {
        const char* text = std::strerror(static_cast<int>(-result));
        throw Error("Injected syscall " + std::to_string(syscallNumber) + " failed [Error: " + std::to_string(-result) + " - " +
                    (text ? text : "") + "]");
    }
    return static_cast<std::uintptr_t>(result);
#  else
    (void)pid;
    (void)syscallNumber;
    (void)arg0;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    return 0;
#  endif
}

void pageProtection(int pid, std::uintptr_t address, int protection) {
    requireArchitecture();
    std::uintptr_t start = 0;
    std::uintptr_t end = 0;
    if (!findMapping(pid, address, start, end))
        throw Error("pageProtection: no mapping found for address 0x" + detail::toHex(address) + " [Pid: " + std::to_string(pid) + "]");

    const long pageSize = ::sysconf(_SC_PAGESIZE) > 0 ? ::sysconf(_SC_PAGESIZE) : 4096L;
    const std::uintptr_t pageStart = start & ~static_cast<std::uintptr_t>(pageSize - 1);
    /* mprotect() takes a length, the Nim implementation passed the end address. */
    const std::uintptr_t length = end - pageStart;
    injectSyscall(pid, SYS_mprotect, pageStart, length, static_cast<std::uintptr_t>(protection), 0, 0, 0);
}

std::uintptr_t allocateMemory(int pid, std::size_t size, int protection) {
    requireArchitecture();
    if (size == 0) throw InvalidArgumentError("allocateMemory: size must be > 0");
    const std::uintptr_t address =
        injectSyscall(pid, SYS_mmap, 0, static_cast<std::uintptr_t>(size), static_cast<std::uintptr_t>(protection),
                      static_cast<std::uintptr_t>(MAP_PRIVATE | MAP_ANONYMOUS), static_cast<std::uintptr_t>(-1), 0);
    if (address == 0 || address == static_cast<std::uintptr_t>(-1))
        throw Error("allocateMemory failed [Pid: " + std::to_string(pid) + "]");
    {
        std::lock_guard<std::mutex> lock(gAllocationMutex);
        gAllocations[pid][address] = size;
    }
    return address;
}

bool freeMemory(int pid, std::uintptr_t address) {
    requireArchitecture();
    std::size_t size = 0;
    {
        std::lock_guard<std::mutex> lock(gAllocationMutex);
        auto it = gAllocations.find(pid);
        if (it != gAllocations.end()) {
            auto allocation = it->second.find(address);
            if (allocation != it->second.end()) {
                size = allocation->second;
                it->second.erase(allocation);
            }
        }
    }
    if (size == 0) {
        /* Not allocated through pyMeow: fall back to the size of the mapping. */
        std::uintptr_t start = 0;
        std::uintptr_t end = 0;
        if (!findMapping(pid, address, start, end)) return false;
        size = static_cast<std::size_t>(end - start);
    }
    try {
        injectSyscall(pid, SYS_munmap, address, static_cast<std::uintptr_t>(size), 0, 0, 0, 0);
    } catch (const Error&) {
        return false;
    }
    return true;
}

} /* namespace ptrace */
} /* namespace pymeow */

#endif /* PYMEOW_LINUX */
