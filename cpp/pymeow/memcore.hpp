/*
 * pyMeow C++ -- memory core (port of memcore.nim)
 * https://github.com/qb-0/pyMeow
 *
 *   Process process = pm::openProcess("ac_client.exe");
 *   pm::Module mod  = pm::getModule(process, "ac_client.exe");
 *   int health      = pm::readInt(process, mod.base + 0xEC);
 *   pm::write<float>(process, address, 1.0f);
 *   auto hits       = pm::aobScanModule(process, "ac_client.exe", "89 01 48 8B 43");
 */
#ifndef PYMEOW_MEMCORE_HPP
#define PYMEOW_MEMCORE_HPP

#include "common.hpp"
#include "error.hpp"
#include "internal/debug.hpp"
#include "types.hpp"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <vector>

namespace pymeow {

/* ------------------------------------------------------------------ structs */

/* A loaded module (executable / shared library / dll) of a process. */
struct PYMEOW_API Module {
    std::string name; /* base name, e.g. "ac_client.exe" or "linux_64_client" */
    std::string path; /* full path when the OS provides one                   */
    Address base = 0;
    Address end = 0;
    Address size = 0;

    explicit operator bool() const { return base != 0; }
};

/* A memory region (page range) belonging to a module. */
struct PYMEOW_API MemoryRegion {
    Address start = 0;
    Address end = 0;
    Address size = 0;
    std::string perms;      /* Linux: protection string of /proc/pid/maps, e.g. "r-xp" */
    std::uint32_t protection = 0; /* Windows: PAGE_* protection of the region           */
    bool readable = true;   /* false when the region can not be read                    */
};

/* Handle of an opened target process. */
struct PYMEOW_API Process {
    std::string name;
    int pid = 0;
    bool debug = false;  /* log every read / write to stdout */
    Address base = 0;    /* base address of the main module  */
    void* handle = nullptr; /* Windows: HANDLE from OpenProcess, nullptr on Linux */

    explicit operator bool() const { return pid != 0; }
};

/* --------------------------------------------------------- process handling */

/* Snapshot of every running process (pid + name filled in). */
PYMEOW_API std::vector<Process> enumProcesses();
PYMEOW_API bool pidExists(int pid);
PYMEOW_API bool processExists(const std::string& processName);
PYMEOW_API bool processRunning(const Process& process);
/* Substring match, like the Python API: "ac_client" finds "ac_client.exe". */
PYMEOW_API int getProcessId(const std::string& processName);
PYMEOW_API std::string getProcessName(int pid);
PYMEOW_API std::string getProcessPath(const Process& process);

PYMEOW_API std::vector<Module> enumModules(const Process& process);
PYMEOW_API bool moduleExists(const Process& process, const std::string& moduleName);
/* Throws NotFoundError when the module does not exist. */
PYMEOW_API Module getModule(const Process& process, const std::string& moduleName);
/* Returns an invalid Module (bool() == false) instead of throwing. */
PYMEOW_API Module findModule(const Process& process, const std::string& moduleName);

/* Throws NotFoundError when the process does not exist. */
PYMEOW_API Process openProcess(int pid, bool debug = false);
PYMEOW_API Process openProcess(const std::string& processName, bool debug = false);
PYMEOW_API void closeProcess(Process& process);

PYMEOW_API std::vector<MemoryRegion> enumMemoryRegions(const Process& process, const Module& module);
PYMEOW_API bool is64Bit(const Process& process);

/* Linux: when enabled every read / write requires an effective uid of 0, like
 * the Python module does. Disabled by default because process_vm_readv() also
 * works for processes owned by the same user. */
PYMEOW_API void setRequireRoot(bool enable);
PYMEOW_API bool requireRootEnabled();

/* ------------------------------------------------------------- raw transfers */

/* Low level transfer, throws MemoryError on failure. */
PYMEOW_API void readPointer(const Process& process, Address address, void* dst, std::size_t size);
PYMEOW_API void writePointer(const Process& process, Address address, const void* src, std::size_t size);

/* Same as readPointer / writePointer but returns false instead of throwing. */
PYMEOW_API bool tryReadPointer(const Process& process, Address address, void* dst, std::size_t size);
PYMEOW_API bool tryWritePointer(const Process& process, Address address, const void* src, std::size_t size);

template <class T>
inline T read(const Process& process, Address address) {
    static_assert(std::is_trivially_copyable<T>::value, "pymeow::read<T> requires a trivially copyable type");
    T value{};
    readPointer(process, address, &value, sizeof(T));
    detail::logRead(process.debug, address, value);
    return value;
}

template <class T>
inline std::vector<T> readSeq(const Process& process, Address address, std::size_t count) {
    static_assert(std::is_trivially_copyable<T>::value, "pymeow::readSeq<T> requires a trivially copyable type");
    std::vector<T> out(count);
    if (count > 0) readPointer(process, address, out.data(), count * sizeof(T));
    detail::logReadSeq(process.debug, address, out);
    return out;
}

template <class T>
inline void write(const Process& process, Address address, const T& data) {
    static_assert(std::is_trivially_copyable<T>::value, "pymeow::write<T> requires a trivially copyable type");
    T value = data;
    writePointer(process, address, &value, sizeof(T));
    detail::logWrite(process.debug, address, data);
}

template <class T>
inline void writeArray(const Process& process, Address address, const T* data, std::size_t count) {
    static_assert(std::is_trivially_copyable<T>::value, "pymeow::writeArray<T> requires a trivially copyable type");
    if (count == 0) return;
    writePointer(process, address, static_cast<const void*>(data), count * sizeof(T));
    detail::logWriteSeq(process.debug, address, data, count);
}

template <class T>
inline void writeArray(const Process& process, Address address, const std::vector<T>& data) {
    writeArray(process, address, data.empty() ? static_cast<const T*>(nullptr) : data.data(), data.size());
}

template <class T, std::size_t N>
inline void writeArray(const Process& process, Address address, const std::array<T, N>& data) {
    writeArray(process, address, data.data(), data.size());
}

template <class T, std::size_t N>
inline void writeArray(const Process& process, Address address, const T (&data)[N]) {
    writeArray(process, address, static_cast<const T*>(data), N);
}

template <class T>
inline void writeArray(const Process& process, Address address, std::initializer_list<T> data) {
    writeArray(process, address, data.begin(), data.size());
}

/* ------------------------------------------------------------ AOB scanning */

/* Scan a loaded module. Returns absolute addresses unless relative = true. */
PYMEOW_API std::vector<Address> aobScanModule(const Process& process, const std::string& moduleName, const std::string& pattern,
                                              bool relative = false, bool single = true, int algorithm = 0);
PYMEOW_API std::vector<Address> aobScanRange(const Process& process, const std::string& pattern, Address rangeStart,
                                             Address rangeEnd, bool relative = false, bool single = true, int algorithm = 0);
/* Scan a memory region of a module (safer than reading the whole module). */
PYMEOW_API std::vector<Address> aobScanRegion(const Process& process, const MemoryRegion& region, const std::string& pattern,
                                              bool relative = false, bool single = true, int algorithm = 0);
PYMEOW_API std::vector<Address> aobScanBytes(const std::string& pattern, const std::vector<std::uint8_t>& byteBuffer,
                                             bool single = true, int algorithm = 0);
PYMEOW_API std::vector<Address> aobScanBytes(const std::string& pattern, const std::uint8_t* data, std::size_t size,
                                             bool single = true, int algorithm = 0);

/* ------------------------------------------- remote allocation / injection */

/* Changes the protection of the page containing `address`.
 * Linux: PROT_* values (PROT_READ|PROT_WRITE|PROT_EXEC == 7) via ptrace.
 * Windows: PAGE_* values (PAGE_EXECUTE_READWRITE, ...) via VirtualProtectEx.
 * Returns the previous protection on Windows, 0 on Linux. */
PYMEOW_API std::int32_t pageProtection(const Process& process, Address address, std::int32_t newProtection);

/* Allocates memory in the target process (VirtualAllocEx / mmap via ptrace).
 * protection = 0 selects the platform default (read + write + execute). */
PYMEOW_API Address allocateMemory(const Process& process, std::size_t size, std::int32_t protection = 0);
PYMEOW_API bool freeMemory(const Process& process, Address address);

/* Windows only. */
PYMEOW_API Address getProcAddress(const std::string& moduleName, const std::string& functionName);
/* exitCode (optional) receives the thread exit code, e.g. the HMODULE returned
 * by LoadLibraryA. */
PYMEOW_API bool createRemoteThread(const Process& process, Address startAddress, Address param = 0, Address* exitCode = nullptr);
PYMEOW_API bool injectLibrary(const Process& process, const std::string& dllPath);
PYMEOW_API bool injectShellcode(const Process& process, const std::vector<std::uint8_t>& shellcode, Address params = 0);
PYMEOW_API bool injectShellcode(const Process& process, const std::string& shellcode, Address params = 0);

} /* namespace pymeow */

#endif /* PYMEOW_MEMCORE_HPP */
