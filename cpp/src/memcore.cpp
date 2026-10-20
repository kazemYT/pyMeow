/*
 * pyMeow C++ -- memory core implementation (port of memcore.nim)
 * https://github.com/qb-0/pyMeow
 *
 * Portable module: every OS call goes through internal/platform.hpp, so this
 * translation unit can include the public headers (and therefore raylib)
 * without colliding with windows.h / Xlib.h.
 */
#include "pymeow/memcore.hpp"

#include "pymeow/internal/aob.hpp"
#include "pymeow/internal/platform.hpp"
#include "pymeow/internal/strings.hpp"

#include <algorithm>
#include <cstdio>

#if defined(PYMEOW_LINUX)
#  include <unistd.h>
#endif

namespace pymeow {

/* ------------------------------------------------------------------- debug */
namespace detail {

void logMemoryAccess(char tag, const std::string& typeName, Address address, const std::string& value) {
    std::printf("[%c] [%s] 0x%s -> %s\n", tag, typeName.c_str(), toHex(address).c_str(), value.c_str());
}

} /* namespace detail */

/* ------------------------------------------------------------------ errors */
std::string errorString() {
    const std::pair<int, std::string> err = getOsError();
    return "[Error: " + std::to_string(err.first) + " - " + err.second + "]";
}

namespace {

bool gRequireRoot = false;

/* The platform layer keeps a text description of the last failure, which is
 * more helpful than errno on X11 / Win32 errors. */
std::string platformErrorText() {
    const std::string text = detail::osLastErrorText();
    return text.empty() ? errorString() : text;
}

[[noreturn]] void memoryError(const char* operation, Address address) {
    const std::string text = detail::osLastErrorText();
    if (text.empty()) {
        const std::pair<int, std::string> err = getOsError();
        throw MemoryError(operation, address, err.first, err.second);
    }
    throw MemoryError(operation, address, 0, text);
}

/* Port of memcore.nim's checkRoot template. Disabled by default: process_vm_readv
 * also works for processes owned by the same user. */
void checkRoot() {
#if defined(PYMEOW_LINUX)
    if (gRequireRoot && ::getuid() != 0) throw Error("Root access required!");
#endif
}

/* getProcessId / openProcess match a name against the process list. The Nim
 * module checks `process.name in processName`; both directions are accepted here
 * so that "ac_client" also finds "ac_client.exe". */
bool processNameMatches(const std::string& query, const std::string& candidate) {
    if (query.empty() || candidate.empty()) return false;
    if (query == candidate) return true;
    return query.find(candidate) != std::string::npos || candidate.find(query) != std::string::npos;
}

detail::OsModuleRef moduleRefOf(const Module& module) {
    detail::OsModuleRef reference;
    reference.name = module.name;
    reference.path = module.path;
    reference.base = module.base;
    reference.end = module.end != 0 ? module.end : (module.base + module.size);
    return reference;
}

std::vector<Address> scanBuffer(const detail::AobPattern& pattern, const std::uint8_t* data, std::size_t size, Address base,
                                bool relative, bool single, int algorithm) {
    const std::vector<std::size_t> offsets = detail::aobScan(data, size, pattern, single, algorithm);
    std::vector<Address> result;
    result.reserve(offsets.size());
    for (std::size_t offset : offsets) {
        result.push_back(relative ? static_cast<Address>(offset) : static_cast<Address>(offset) + base);
    }
    return result;
}

} /* namespace */

const char* version() { return PYMEOW_VERSION_STRING; }

#if defined(PYMEOW_WINDOWS)
const char* systemName() { return "windows"; }
#else
const char* systemName() { return "linux"; }
#endif

void setRequireRoot(bool enable) { gRequireRoot = enable; }
bool requireRootEnabled() { return gRequireRoot; }

/* --------------------------------------------------------- process handling */
std::vector<Process> enumProcesses() {
    std::vector<detail::OsProcess> entries;
    if (!detail::osEnumProcesses(entries)) throw Error("Could not enumerate processes: " + platformErrorText());

    std::vector<Process> result;
    result.reserve(entries.size());
    for (const detail::OsProcess& entry : entries) {
        Process process;
        process.pid = entry.pid;
        process.name = entry.name;
        result.push_back(process);
    }
    return result;
}

bool pidExists(int pid) {
    for (const Process& process : enumProcesses()) {
        if (process.pid == pid) return true;
    }
    return false;
}

bool processExists(const std::string& processName) {
    for (const Process& process : enumProcesses()) {
        if (process.name == processName) return true;
    }
    return false;
}

bool processRunning(const Process& process) { return detail::osProcessAlive(process.pid); }

int getProcessId(const std::string& processName) {
    for (const Process& process : enumProcesses()) {
        if (processNameMatches(processName, process.name)) return process.pid;
    }
    throw NotFoundError("Process '" + processName + "' not found");
}

std::string getProcessName(int pid) {
    for (const Process& process : enumProcesses()) {
        if (process.pid == pid) return process.name;
    }
    throw NotFoundError("Process '" + std::to_string(pid) + "' not found");
}

std::string getProcessPath(const Process& process) {
    std::string path;
    if (!detail::osProcessPath(process.pid, path)) throw Error("Could not read the process path: " + platformErrorText());
    return path;
}

std::vector<Module> enumModules(const Process& process) {
    std::vector<detail::OsModule> entries;
    if (!detail::osEnumModules(process.pid, entries)) throw Error("Could not enumerate modules: " + platformErrorText());

    std::vector<Module> result;
    result.reserve(entries.size());
    for (const detail::OsModule& entry : entries) {
        Module module;
        module.name = entry.name;
        module.path = entry.path;
        module.base = entry.base;
        module.size = entry.size;
        module.end = entry.base + entry.size;
        result.push_back(module);
    }
    /* The Python module yields the modules in maps / snapshot order; sorting by
     * base address keeps modules[0] == the main module on both platforms. */
    std::sort(result.begin(), result.end(), [](const Module& a, const Module& b) { return a.base < b.base; });
    return result;
}

bool moduleExists(const Process& process, const std::string& moduleName) {
    for (const Module& module : enumModules(process)) {
        if (module.name == moduleName) return true;
    }
    return false;
}

Module getModule(const Process& process, const std::string& moduleName) {
    for (const Module& module : enumModules(process)) {
        if (module.name == moduleName) return module;
    }
    throw NotFoundError("Module '" + moduleName + "' not found");
}

Module findModule(const Process& process, const std::string& moduleName) {
    for (const Module& module : enumModules(process)) {
        if (module.name == moduleName) return module;
    }
    return Module();
}

Process openProcess(int pid, bool debug) {
    if (!pidExists(pid)) throw NotFoundError("Process ID '" + std::to_string(pid) + "' does not exist");

    Process result;
    result.debug = debug;
    result.pid = pid;
    result.name = getProcessName(pid);

    const std::vector<Module> modules = enumModules(result);
    if (!modules.empty()) result.base = modules.front().base;

    if (!detail::osOpenProcess(pid, &result.handle)) {
        throw Error("Unable to open Process [Pid: " + std::to_string(pid) + "] " + platformErrorText());
    }
    return result;
}

Process openProcess(const std::string& processName, bool debug) {
    for (const Process& process : enumProcesses()) {
        if (processNameMatches(processName, process.name)) return openProcess(process.pid, debug);
    }
    throw NotFoundError("Process '" + processName + "' not found");
}

void closeProcess(Process& process) {
    detail::osCloseProcess(process.handle);
    process.handle = nullptr;
}

std::vector<MemoryRegion> enumMemoryRegions(const Process& process, const Module& module) {
    std::vector<detail::OsRegion> entries;
    if (!detail::osEnumRegions(process.pid, process.handle, moduleRefOf(module), entries)) {
        throw Error("Could not enumerate memory regions: " + platformErrorText());
    }

    std::vector<MemoryRegion> result;
    result.reserve(entries.size());
    for (const detail::OsRegion& entry : entries) {
        MemoryRegion region;
        region.start = entry.start;
        region.size = entry.size;
        region.end = entry.start + entry.size;
        region.perms = entry.perms;
        region.protection = entry.protection;
        region.readable = entry.readable;
        result.push_back(region);
    }
    return result;
}

bool is64Bit(const Process& process) {
    bool result = true;
    if (!detail::osIsProcess64Bit(process.pid, process.handle, result)) throw Error("is64Bit failed: " + platformErrorText());
    return result;
}

/* ------------------------------------------------------------- raw transfers */
void readPointer(const Process& process, Address address, void* dst, std::size_t size) {
    checkRoot();
    if (!detail::osReadMemory(process.pid, process.handle, address, dst, size)) memoryError("Read", address);
    if (process.debug) detail::logMemoryAccess('R', "bytes", address, detail::bufferText(dst, size));
}

void writePointer(const Process& process, Address address, const void* src, std::size_t size) {
    checkRoot();
    if (!detail::osWriteMemory(process.pid, process.handle, address, src, size)) memoryError("Write", address);
    if (process.debug) detail::logMemoryAccess('W', "bytes", address, detail::bufferText(src, size));
}

bool tryReadPointer(const Process& process, Address address, void* dst, std::size_t size) {
    checkRoot();
    return detail::osReadMemory(process.pid, process.handle, address, dst, size);
}

bool tryWritePointer(const Process& process, Address address, const void* src, std::size_t size) {
    checkRoot();
    return detail::osWriteMemory(process.pid, process.handle, address, src, size);
}

/* ------------------------------------------------------------ AOB scanning */
std::vector<Address> aobScanBytes(const std::string& pattern, const std::uint8_t* data, std::size_t size, bool single,
                                  int algorithm) {
    const detail::AobPattern parsed = detail::parseAobPattern(pattern);
    if (parsed.empty()) throw InvalidArgumentError("Invalid pattern: '" + pattern + "'");
    return scanBuffer(parsed, data, size, 0, true, single, algorithm);
}

std::vector<Address> aobScanBytes(const std::string& pattern, const std::vector<std::uint8_t>& byteBuffer, bool single,
                                  int algorithm) {
    return aobScanBytes(pattern, byteBuffer.empty() ? nullptr : byteBuffer.data(), byteBuffer.size(), single, algorithm);
}

std::vector<Address> aobScanModule(const Process& process, const std::string& moduleName, const std::string& pattern, bool relative,
                                   bool single, int algorithm) {
    const Module module = getModule(process, moduleName);
    const detail::AobPattern parsed = detail::parseAobPattern(pattern);
    if (parsed.empty()) throw InvalidArgumentError("Invalid pattern: '" + pattern + "'");

    /* The Python module reads the whole module in one go. That fails as soon as
     * a single page is not readable, so fall back to a region by region scan. */
    if (module.size > 0) {
        std::vector<std::uint8_t> buffer(static_cast<std::size_t>(module.size));
        if (tryReadPointer(process, module.base, buffer.data(), buffer.size())) {
            return scanBuffer(parsed, buffer.data(), buffer.size(), module.base, relative, single, algorithm);
        }
    }

    std::vector<Address> result;
    for (const MemoryRegion& region : enumMemoryRegions(process, module)) {
        if (!region.readable || region.size == 0) continue;
        std::vector<std::uint8_t> data(static_cast<std::size_t>(region.size));
        if (!tryReadPointer(process, region.start, data.data(), data.size())) continue;
        const std::vector<Address> hits = scanBuffer(parsed, data.data(), data.size(), region.start, relative, single, algorithm);
        result.insert(result.end(), hits.begin(), hits.end());
        if (single && !result.empty()) break;
    }
    return result;
}

std::vector<Address> aobScanRange(const Process& process, const std::string& pattern, Address rangeStart, Address rangeEnd,
                                  bool relative, bool single, int algorithm) {
    if (rangeStart >= rangeEnd) throw InvalidArgumentError("Invalid range (rangeStart > rangeEnd)");
    const detail::AobPattern parsed = detail::parseAobPattern(pattern);
    if (parsed.empty()) throw InvalidArgumentError("Invalid pattern: '" + pattern + "'");

    const std::size_t size = static_cast<std::size_t>(rangeEnd - rangeStart);
    std::vector<std::uint8_t> buffer(size);
    readPointer(process, rangeStart, buffer.data(), size);
    return scanBuffer(parsed, buffer.data(), buffer.size(), rangeStart, relative, single, algorithm);
}

std::vector<Address> aobScanRegion(const Process& process, const MemoryRegion& region, const std::string& pattern, bool relative,
                                   bool single, int algorithm) {
    return aobScanRange(process, pattern, region.start, region.start + region.size, relative, single, algorithm);
}

/* ------------------------------------------- remote allocation / injection */
std::int32_t pageProtection(const Process& process, Address address, std::int32_t newProtection) {
    int oldProtection = 0;
    /* size 0 -> the platform changes the whole region containing the address */
    if (!detail::osSetPageProtection(process.pid, process.handle, address, 0, static_cast<int>(newProtection), oldProtection)) {
        throw Error("page_protection failed: " + platformErrorText());
    }
    return static_cast<std::int32_t>(oldProtection);
}

Address allocateMemory(const Process& process, std::size_t size, std::int32_t protection) {
    std::uint64_t address = 0;
    if (!detail::osAllocateMemory(process.pid, process.handle, size, static_cast<int>(protection), address) || address == 0) {
        throw Error("allocate_memory failed: " + platformErrorText());
    }
    return static_cast<Address>(address);
}

bool freeMemory(const Process& process, Address address) {
    return detail::osFreeMemory(process.pid, process.handle, address);
}

Address getProcAddress(const std::string& moduleName, const std::string& functionName) {
    std::uint64_t address = 0;
    if (!detail::osModuleProcAddress(moduleName, functionName, address)) throw Error("get_proc_address failed: " + platformErrorText());
    return static_cast<Address>(address);
}

bool createRemoteThread(const Process& process, Address startAddress, Address param, Address* exitCode) {
    std::uint64_t code = 0;
    const bool ok = detail::osCreateRemoteThread(process.handle, startAddress, param, code);
    if (exitCode != nullptr) *exitCode = static_cast<Address>(code);
    if (!ok) throw Error("create_remote_thread failed: " + platformErrorText());
    return true;
}

bool injectLibrary(const Process& process, const std::string& dllPath) {
#if defined(PYMEOW_WINDOWS)
    const Address allocated = allocateMemory(process, dllPath.size() + 1, 0x04 /* PAGE_READWRITE */);
    writePointer(process, allocated, dllPath.c_str(), dllPath.size() + 1);
    return createRemoteThread(process, getProcAddress("kernel32.dll", "LoadLibraryA"), allocated);
#else
    (void)process;
    (void)dllPath;
    throw Error("injectLibrary is only supported on Windows");
#endif
}

bool injectShellcode(const Process& process, const std::vector<std::uint8_t>& shellcode, Address params) {
#if defined(PYMEOW_WINDOWS)
    if (shellcode.empty()) throw InvalidArgumentError("injectShellcode: empty shellcode");
    const Address allocated = allocateMemory(process, shellcode.size(), 0x40 /* PAGE_EXECUTE_READWRITE */);
    writePointer(process, allocated, shellcode.data(), shellcode.size());
    return createRemoteThread(process, allocated, params);
#else
    (void)process;
    (void)shellcode;
    (void)params;
    throw Error("injectShellcode is only supported on Windows");
#endif
}

bool injectShellcode(const Process& process, const std::string& shellcode, Address params) {
    return injectShellcode(process, std::vector<std::uint8_t>(shellcode.begin(), shellcode.end()), params);
}

} /* namespace pymeow */
