/*
 * pyMeow C++ -- Windows platform layer
 * https://github.com/qb-0/pyMeow
 *
 * Every Win32 API call lives here. This translation unit must never include
 * raylib: windows.h and raylib.h cannot be used together (GDI declares a
 * `Rectangle()` function, raylib a `Rectangle` type; windows.h also defines
 * PlaySound, CloseWindow, min/max, ...). raylib itself avoids windows.h for
 * the same reason.
 *
 * Links against: kernel32, user32, gdi32, psapi
 */
#include "pymeow/internal/platform.hpp"

#if defined(PYMEOW_WINDOWS)

#  include "pymeow/error.hpp"

#  include <windows.h>

#  include <dwmapi.h>
#  include <psapi.h>

#  include <algorithm>
#  include <cstring>

namespace pymeow {

/* ---------------------------------------------------------------- os errors */
std::pair<int, std::string> getOsError() {
    const DWORD code = ::GetLastError();
    char* message = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    std::string text;
    if (::FormatMessageA(flags, nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<char*>(&message), 0,
                         nullptr) != 0 &&
        message != nullptr) {
        text = message;
        ::LocalFree(message);
    }
    while (!text.empty() && (text.back() == '\r' || text.back() == '\n')) text.pop_back();
    return std::make_pair(static_cast<int>(code), text);
}



namespace detail {

namespace {

std::string gLastErrorText;

void setErrorText(const std::string& text) { gLastErrorText = text; }

std::string utf16ToUtf8(const wchar_t* text, int length = -1) {
    if (text == nullptr || *text == L'\0') return std::string();
    if (length < 0) length = static_cast<int>(::wcslen(text));
    if (length == 0) return std::string();
    const int size = ::WideCharToMultiByte(CP_UTF8, 0, text, length, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return std::string();
    std::string out(static_cast<std::size_t>(size), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, text, length, &out[0], size, nullptr, nullptr);
    return out;
}

std::wstring utf8ToUtf16(const std::string& text) {
    if (text.empty()) return std::wstring();
    const int size = ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) return std::wstring();
    std::wstring out(static_cast<std::size_t>(size), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &out[0], size);
    return out;
}

std::string baseName(const std::string& path) {
    std::size_t cut = path.find_last_of("\\/");
    return (cut == std::string::npos) ? path : path.substr(cut + 1);
}

struct WindowTitleSearch {
    DWORD pid = 0;
    std::string title;
    bool found = false;
};

BOOL CALLBACK enumWindowsCallback(HWND window, LPARAM param) {
    WindowTitleSearch* search = reinterpret_cast<WindowTitleSearch*>(param);

    DWORD windowPid = 0;
    ::GetWindowThreadProcessId(window, &windowPid);
    if (windowPid != search->pid) return TRUE;

    wchar_t buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    if (::GetWindowTextW(window, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0]))) == 0) return TRUE;

    const std::string title = utf16ToUtf8(buffer);
    if (!title.empty()) {
        search->title = title;
        search->found = true;
        return FALSE;
    }
    return TRUE;
}

} /* namespace */

std::string osLastErrorText() { return gLastErrorText; }

/* ----------------------------------------------------------------- process */
bool osEnumProcesses(std::vector<OsProcess>& out) {
    out.clear();
    DWORD pids[2048];
    DWORD bytesReturned = 0;
    std::memset(pids, 0, sizeof(pids));
    if (::EnumProcesses(pids, sizeof(pids), &bytesReturned) == 0) {
        setErrorText("EnumProcesses failed: " + errorString());
        return false;
    }
    const std::size_t count = static_cast<std::size_t>(bytesReturned / sizeof(DWORD));
    for (std::size_t i = 0; i < count; ++i) {
        const DWORD pid = pids[i];
        if (pid == 0) continue;
        OsProcess process;
        process.pid = static_cast<int>(pid);
        HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (handle == nullptr) continue;
        wchar_t name[1024];
        std::memset(name, 0, sizeof(name));
        if (::GetModuleBaseNameW(handle, nullptr, name, static_cast<DWORD>(sizeof(name) / sizeof(name[0]))) != 0) {
            process.name = utf16ToUtf8(name);
        }
        ::CloseHandle(handle);
        if (process.name.empty()) continue;
        out.push_back(process);
    }
    std::sort(out.begin(), out.end(), [](const OsProcess& a, const OsProcess& b) { return a.pid < b.pid; });
    return true;
}

bool osProcessAlive(int pid) {
    if (pid <= 0) return false;
    HANDLE handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (handle == nullptr) return false;
    DWORD exitCode = 0;
    const BOOL ok = ::GetExitCodeProcess(handle, &exitCode);
    ::CloseHandle(handle);
    return ok != 0 && exitCode == STILL_ACTIVE;
}

bool osProcessPath(int pid, std::string& out) {
    HANDLE handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (handle == nullptr) {
        setErrorText("OpenProcess failed: " + errorString());
        return false;
    }
    wchar_t buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    DWORD size = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
    const BOOL ok = ::QueryFullProcessImageNameW(handle, 0, buffer, &size);
    ::CloseHandle(handle);
    if (ok == 0) {
        setErrorText("QueryFullProcessImageName failed: " + errorString());
        return false;
    }
    out = utf16ToUtf8(buffer);
    return true;
}

bool osOpenProcess(int pid, void** handle) {
    if (handle == nullptr) return false;
    HANDLE process = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, static_cast<DWORD>(pid));
    if (process == nullptr) {
        const DWORD rights = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION;
        process = ::OpenProcess(rights, FALSE, static_cast<DWORD>(pid));
    }
    if (process == nullptr) {
        setErrorText("OpenProcess failed: " + errorString());
        return false;
    }
    *handle = process;
    return true;
}

void osCloseProcess(void* handle) {
    if (handle != nullptr) ::CloseHandle(static_cast<HANDLE>(handle));
}

bool osEnumModules(int pid, std::vector<OsModule>& out) {
    out.clear();
    HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, static_cast<DWORD>(pid));
    if (handle == nullptr) {
        setErrorText("OpenProcess failed: " + errorString());
        return false;
    }
    HMODULE modules[1024];
    DWORD bytesReturned = 0;
    std::memset(modules, 0, sizeof(modules));
    const BOOL ok = ::EnumProcessModulesEx(handle, modules, sizeof(modules), &bytesReturned, LIST_MODULES_ALL);
    if (ok == 0) {
        ::CloseHandle(handle);
        setErrorText("EnumProcessModulesEx failed: " + errorString());
        return false;
    }
    const std::size_t count = static_cast<std::size_t>(bytesReturned / sizeof(HMODULE));
    for (std::size_t i = 0; i < count; ++i) {
        OsModule module;
        MODULEINFO info;
        std::memset(&info, 0, sizeof(info));
        if (::GetModuleInformation(handle, modules[i], &info, sizeof(info)) != 0) {
            module.base = reinterpret_cast<std::uint64_t>(info.lpBaseOfDll);
            module.size = static_cast<std::uint64_t>(info.SizeOfImage);
        }
        wchar_t name[1024];
        std::memset(name, 0, sizeof(name));
        if (::GetModuleBaseNameW(handle, modules[i], name, static_cast<DWORD>(sizeof(name) / sizeof(name[0]))) != 0) {
            module.name = utf16ToUtf8(name);
        }
        std::memset(name, 0, sizeof(name));
        if (::GetModuleFileNameExW(handle, modules[i], name, static_cast<DWORD>(sizeof(name) / sizeof(name[0]))) != 0) {
            module.path = utf16ToUtf8(name);
        }
        if (module.name.empty()) continue;
        out.push_back(module);
    }
    ::CloseHandle(handle);
    return true;
}

bool osEnumRegions(int pid, void* handle, const OsModuleRef& module, std::vector<OsRegion>& out) {
    out.clear();
    HANDLE process = static_cast<HANDLE>(handle);
    const bool ownHandle = (process == nullptr);
    if (ownHandle) {
        process = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, static_cast<DWORD>(pid));
        if (process == nullptr) {
            setErrorText("OpenProcess failed: " + errorString());
            return false;
        }
    }

    SYSTEM_INFO sysInfo;
    std::memset(&sysInfo, 0, sizeof(sysInfo));
    ::GetSystemInfo(&sysInfo);

    std::uint64_t address = module.base;
    const std::uint64_t end = (module.end != 0) ? module.end : static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(sysInfo.lpMaximumApplicationAddress));
    if (address == 0) address = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(sysInfo.lpMinimumApplicationAddress));

    MEMORY_BASIC_INFORMATION info;
    std::memset(&info, 0, sizeof(info));
    while (address < end) {
        if (::VirtualQueryEx(process, reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), &info, sizeof(info)) == 0) break;
        const std::uint64_t regionBase = reinterpret_cast<std::uint64_t>(info.BaseAddress);
        if (module.base != 0 && reinterpret_cast<std::uint64_t>(info.AllocationBase) != module.base) {
            address = regionBase + static_cast<std::uint64_t>(info.RegionSize);
            continue;
        }
        if (info.State == MEM_COMMIT) {
            OsRegion region;
            region.start = regionBase;
            region.size = static_cast<std::uint64_t>(info.RegionSize);
            region.protection = static_cast<unsigned int>(info.Protect);
            region.readable = (info.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ |
                                               PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
            out.push_back(region);
        }
        const std::uint64_t next = regionBase + static_cast<std::uint64_t>(info.RegionSize);
        if (next <= address) break;
        address = next;
    }
    if (ownHandle) ::CloseHandle(process);
    return true;
}

bool osIsProcess64Bit(int pid, void* handle, bool& out) {
    HANDLE process = static_cast<HANDLE>(handle);
    const bool ownHandle = (process == nullptr);
    if (ownHandle) {
        process = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
        if (process == nullptr) {
            setErrorText("OpenProcess failed: " + errorString());
            return false;
        }
    }
    BOOL isWow64 = FALSE;
    if (::IsWow64Process(process, &isWow64) == 0) {
        out = true;
    } else {
        out = (isWow64 == FALSE);
    }
    if (ownHandle) ::CloseHandle(process);
    return true;
}

/* ------------------------------------------------------------------ memory */
bool osReadMemory(int pid, void* handle, std::uint64_t address, void* dst, std::size_t size) {
    HANDLE process = static_cast<HANDLE>(handle);
    const bool ownHandle = (process == nullptr);
    if (ownHandle) {
        process = ::OpenProcess(PROCESS_VM_READ, FALSE, static_cast<DWORD>(pid));
        if (process == nullptr) {
            setErrorText("OpenProcess failed: " + errorString());
            return false;
        }
    }
    SIZE_T bytesRead = 0;
    const BOOL ok = ::ReadProcessMemory(process, reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), dst, size,
                                        &bytesRead);
    if (ownHandle) ::CloseHandle(process);
    if (ok == 0 || static_cast<std::size_t>(bytesRead) != size) {
        setErrorText("ReadProcessMemory failed: " + errorString());
        return false;
    }
    return true;
}

bool osWriteMemory(int pid, void* handle, std::uint64_t address, const void* src, std::size_t size) {
    HANDLE process = static_cast<HANDLE>(handle);
    const bool ownHandle = (process == nullptr);
    if (ownHandle) {
        process = ::OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, static_cast<DWORD>(pid));
        if (process == nullptr) {
            setErrorText("OpenProcess failed: " + errorString());
            return false;
        }
    }
    SIZE_T bytesWritten = 0;
    const BOOL ok = ::WriteProcessMemory(process, reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), src, size,
                                         &bytesWritten);
    if (ownHandle) ::CloseHandle(process);
    if (ok == 0 || static_cast<std::size_t>(bytesWritten) != size) {
        setErrorText("WriteProcessMemory failed: " + errorString());
        return false;
    }
    return true;
}

bool osSetPageProtection(int pid, void* handle, std::uint64_t address, std::size_t size, int protection, int& oldProtection) {
    (void)pid;
    HANDLE process = static_cast<HANDLE>(handle);
    if (process == nullptr) {
        setErrorText("A process handle is required for pageProtection on Windows");
        return false;
    }
    /* The Python module queries the region first and changes its whole size. */
    std::size_t regionSize = size;
    if (regionSize == 0) {
        MEMORY_BASIC_INFORMATION info;
        std::memset(&info, 0, sizeof(info));
        if (::VirtualQueryEx(process, reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), &info, sizeof(info)) == 0) {
            setErrorText("VirtualQueryEx failed: " + errorString());
            return false;
        }
        regionSize = static_cast<std::size_t>(info.RegionSize);
    }
    DWORD previous = 0;
    if (::VirtualProtectEx(process, reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), regionSize,
                           static_cast<DWORD>(protection), &previous) == 0) {
        setErrorText("VirtualProtectEx failed: " + errorString());
        return false;
    }
    oldProtection = static_cast<int>(previous);
    return true;
}

bool osAllocateMemory(int pid, void* handle, std::size_t size, int protection, std::uint64_t& out) {
    HANDLE process = static_cast<HANDLE>(handle);
    if (process == nullptr) {
        process = ::OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_CREATE_THREAD, FALSE,
                                static_cast<DWORD>(pid));
        if (process == nullptr) {
            setErrorText("OpenProcess failed: " + errorString());
            return false;
        }
        LPVOID memory = ::VirtualAllocEx(process, nullptr, size, MEM_COMMIT | MEM_RESERVE,
                                         protection != 0 ? static_cast<DWORD>(protection) : static_cast<DWORD>(PAGE_EXECUTE_READWRITE));
        ::CloseHandle(process);
        if (memory == nullptr) {
            setErrorText("VirtualAllocEx failed: " + errorString());
            return false;
        }
        out = reinterpret_cast<std::uint64_t>(memory);
        return true;
    }
    LPVOID memory = ::VirtualAllocEx(process, nullptr, size, MEM_COMMIT | MEM_RESERVE,
                                     protection != 0 ? static_cast<DWORD>(protection) : static_cast<DWORD>(PAGE_EXECUTE_READWRITE));
    if (memory == nullptr) {
        setErrorText("VirtualAllocEx failed: " + errorString());
        return false;
    }
    out = reinterpret_cast<std::uint64_t>(memory);
    return true;
}

bool osFreeMemory(int pid, void* handle, std::uint64_t address) {
    HANDLE process = static_cast<HANDLE>(handle);
    const bool ownHandle = (process == nullptr);
    if (ownHandle) {
        process = ::OpenProcess(PROCESS_VM_OPERATION, FALSE, static_cast<DWORD>(pid));
        if (process == nullptr) {
            setErrorText("OpenProcess failed: " + errorString());
            return false;
        }
    }
    const BOOL ok = ::VirtualFreeEx(process, reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), 0, MEM_RELEASE);
    if (ownHandle) ::CloseHandle(process);
    if (ok == 0) {
        setErrorText("VirtualFreeEx failed: " + errorString());
        return false;
    }
    return true;
}

bool osModuleProcAddress(const std::string& moduleName, const std::string& functionName, std::uint64_t& out) {
    const std::wstring wideModule = utf8ToUtf16(moduleName);
    HMODULE module = ::GetModuleHandleW(wideModule.c_str());
    if (module == nullptr) module = ::LoadLibraryW(wideModule.c_str());
    if (module == nullptr) {
        setErrorText("Module not found: " + moduleName + " " + errorString());
        return false;
    }
    FARPROC proc = ::GetProcAddress(module, functionName.c_str());
    if (proc == nullptr) {
        setErrorText("Function not found: " + functionName + " " + errorString());
        return false;
    }
    out = reinterpret_cast<std::uint64_t>(proc);
    return true;
}

bool osCreateRemoteThread(void* handle, std::uint64_t startAddress, std::uint64_t param, std::uint64_t& exitCode) {
    HANDLE process = static_cast<HANDLE>(handle);
    if (process == nullptr) {
        setErrorText("A process handle is required for createRemoteThread");
        return false;
    }
    HANDLE thread = ::CreateRemoteThread(process, nullptr, 0,
                                         reinterpret_cast<LPTHREAD_START_ROUTINE>(static_cast<std::uintptr_t>(startAddress)),
                                         reinterpret_cast<LPVOID>(static_cast<std::uintptr_t>(param)), 0, nullptr);
    if (thread == nullptr) {
        setErrorText("CreateRemoteThread failed: " + errorString());
        return false;
    }
    ::WaitForSingleObject(thread, INFINITE);
    DWORD threadExitCode = 0;
    const BOOL ok = ::GetExitCodeThread(thread, &threadExitCode);
    ::CloseHandle(thread);
    if (ok == 0) {
        setErrorText("GetExitCodeThread failed: " + errorString());
        return false;
    }
    exitCode = static_cast<std::uint64_t>(threadExitCode);
    return true;
}

/* ------------------------------------------------------------ screen/window */
bool osDisplayResolution(int& width, int& height) {
    width = ::GetSystemMetrics(SM_CXSCREEN);
    height = ::GetSystemMetrics(SM_CYSCREEN);
    return width > 0 && height > 0;
}

bool osWindowRectByName(const std::string& name, OsWindowRect& out) {
    const std::wstring wideName = utf8ToUtf16(name);
    HWND window = ::FindWindowW(nullptr, wideName.c_str());
    if (window == nullptr) {
        setErrorText("Window (" + name + ") not found");
        return false;
    }
    RECT client;
    std::memset(&client, 0, sizeof(client));
    if (::GetClientRect(window, &client) == 0) {
        setErrorText("GetClientRect failed: " + errorString());
        return false;
    }
    WINDOWINFO info;
    std::memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(WINDOWINFO);
    if (::GetWindowInfo(window, &info) == 0) {
        setErrorText("GetWindowInfo failed: " + errorString());
        return false;
    }
    out.x = static_cast<int>(info.rcClient.left);
    out.y = static_cast<int>(info.rcClient.top);
    out.width = static_cast<int>(client.right - client.left);
    out.height = static_cast<int>(client.bottom - client.top);
    return true;
}

bool osWindowTitleByPid(int pid, std::string& out) {
    if (pid <= 0) {
        setErrorText("Invalid PID");
        return false;
    }
    WindowTitleSearch search;
    search.pid = static_cast<DWORD>(pid);
    ::EnumWindows(&enumWindowsCallback, reinterpret_cast<LPARAM>(&search));
    if (!search.found) {
        setErrorText("No Window found. PID: " + std::to_string(pid));
        return false;
    }
    out = search.title;
    return true;
}

void osSetDpiAware() { ::SetProcessDPIAware(); }

/* ------------------------------------------------------------------- input */
bool osKeyPressed(int key, bool& pressed) {
    pressed = (::GetAsyncKeyState(key) & 0x8000) != 0;
    return true;
}

namespace {

void sendKeyboard(WORD virtualKey, bool up) {
    INPUT input;
    std::memset(&input, 0, sizeof(input));
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = virtualKey;
    if (up) input.ki.dwFlags = KEYEVENTF_KEYUP;
    ::SendInput(1, &input, sizeof(input));
}

void sendMouseInput(DWORD flags) {
    INPUT input;
    std::memset(&input, 0, sizeof(input));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flags;
    ::SendInput(1, &input, sizeof(input));
}

} /* namespace */

bool osFakeKey(int key, bool down) {
    sendKeyboard(static_cast<WORD>(key), !down);
    return true;
}

bool osMouseButtonState(int button, bool& down) {
    int virtualKey = VK_LBUTTON;
    if (button == 1)
        virtualKey = VK_MBUTTON;
    else if (button == 2)
        virtualKey = VK_RBUTTON;
    down = (::GetAsyncKeyState(virtualKey) & 0x8000) != 0;
    return true;
}

bool osFakeMouseButton(int button, bool down) {
    DWORD flags = MOUSEEVENTF_LEFTDOWN;
    if (button == 1)
        flags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    else if (button == 2)
        flags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    else
        flags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    sendMouseInput(flags);
    return true;
}

bool osFakeMouseMove(int x, int y, bool relative) {
    int targetX = x;
    int targetY = y;
    if (relative) {
        POINT position;
        std::memset(&position, 0, sizeof(position));
        if (::GetCursorPos(&position) != FALSE) {
            targetX += position.x;
            targetY += position.y;
        }
    }
    return ::SetCursorPos(targetX, targetY) != FALSE;
}

bool osCursorPosition(int& x, int& y) {
    POINT position;
    std::memset(&position, 0, sizeof(position));
    if (::GetCursorPos(&position) == FALSE) {
        setErrorText("GetCursorPos failed: " + errorString());
        return false;
    }
    x = position.x;
    y = position.y;
    return true;
}

/* ------------------------------------------------------------------- pixel */
bool osCaptureRegion(int x, int y, int width, int height, std::vector<unsigned char>& rgbaOut) {
    if (width <= 0 || height <= 0) return false;
    HDC screen = ::GetDC(nullptr);
    if (screen == nullptr) {
        setErrorText("GetDC failed: " + errorString());
        return false;
    }
    HDC memory = ::CreateCompatibleDC(screen);
    HBITMAP bitmap = ::CreateCompatibleBitmap(screen, width, height);
    HGDIOBJ old = ::SelectObject(memory, bitmap);

    BITMAPINFO info;
    std::memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height; /* top down */
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    rgbaOut.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0);
    const BOOL blitted = ::BitBlt(memory, 0, 0, width, height, screen, x, y, SRCCOPY);
    const int rows = (blitted == 0) ? 0 : ::GetDIBits(memory, bitmap, 0, static_cast<UINT>(height), rgbaOut.data(), &info, DIB_RGB_COLORS);

    ::SelectObject(memory, old);
    ::DeleteObject(bitmap);
    ::DeleteDC(memory);
    ::ReleaseDC(nullptr, screen);

    if (blitted == 0 || rows == 0) {
        setErrorText("Screen capture failed (BitBlt/GetDIBits): " + errorString());
        return false;
    }
    /* BGRA -> RGBA */
    for (std::size_t i = 0; i + 3 < rgbaOut.size(); i += 4) std::swap(rgbaOut[i], rgbaOut[i + 2]);
    return true;
}

} /* namespace detail */
} /* namespace pymeow */

#endif /* PYMEOW_WINDOWS */
