/*
 * pyMeow C++ -- Linux platform layer
 * https://github.com/qb-0/pyMeow
 *
 * /proc parsing, process_vm_readv()/process_vm_writev(), ptrace based remote
 * allocation and the X11 part (input injection, screen capture, window lookup).
 *
 * This translation unit must never include raylib (Xlib.h defines a global
 * `Font` typedef that collides with raylib's Font).
 *
 * Links against: libX11, libXtst, libXinerama
 */
#include "pymeow/internal/platform.hpp"

#if defined(PYMEOW_LINUX)

#  include "pymeow/error.hpp"
#  include "pymeow/ptrace.hpp"

#  include <dirent.h>
#  include <fstream>
#  include <signal.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <sys/uio.h>
#  include <unistd.h>

#  include <X11/Xatom.h>
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/extensions/Xinerama.h>
#  include <X11/extensions/XTest.h>
#  include <X11/keysym.h>

#  include <algorithm>
#  include <cerrno>
#  include <cstdlib>
#  include <cstring>
#  include <map>

namespace pymeow {

/* ---------------------------------------------------------------- os errors */
std::pair<int, std::string> getOsError() {
    const int code = errno;
    const char* message = std::strerror(code);
    return std::make_pair(code, message != nullptr ? std::string(message) : std::string());
}



namespace detail {

namespace {

std::string gLastErrorText;

void setErrorText(const std::string& text) { gLastErrorText = text; }

/* ------------------------------------------------------------------ /proc */
struct MapsEntry {
    std::uint64_t start = 0;
    std::uint64_t end = 0;
    std::string perms;
    std::string path;
};

bool parseMapsLine(const std::string& line, MapsEntry& out) {
    const std::size_t dash = line.find('-');
    if (dash == std::string::npos) return false;
    const std::size_t space = line.find(' ', dash);
    if (space == std::string::npos) return false;

    char* endPtr = nullptr;
    const unsigned long long start = std::strtoull(line.substr(0, dash).c_str(), &endPtr, 16);
    const unsigned long long end = std::strtoull(line.substr(dash + 1, space - dash - 1).c_str(), &endPtr, 16);
    if (end <= start) return false;
    out.start = static_cast<std::uint64_t>(start);
    out.end = static_cast<std::uint64_t>(end);

    /* perms offset dev inode pathname */
    std::size_t pos = space + 1;
    const std::size_t permsEnd = line.find(' ', pos);
    if (permsEnd == std::string::npos) return false;
    out.perms = line.substr(pos, permsEnd - pos);

    pos = permsEnd + 1;
    for (int field = 0; field < 3 && pos < line.size(); ++field) { /* skip offset, dev, inode */
        const std::size_t next = line.find(' ', pos);
        if (next == std::string::npos) break;
        pos = next + 1;
    }
    while (pos < line.size() && line[pos] == ' ') ++pos;
    out.path = line.substr(pos);
    while (!out.path.empty() && (out.path.back() == '\r' || out.path.back() == '\n')) out.path.pop_back();
    return true;
}

std::vector<MapsEntry> readMaps(int pid) {
    std::vector<MapsEntry> entries;
    std::ifstream maps(("/proc/" + std::to_string(pid) + "/maps").c_str());
    if (!maps.is_open()) {
        setErrorText("Unable to read /proc/" + std::to_string(pid) + "/maps");
        return entries;
    }
    std::string line;
    while (std::getline(maps, line)) {
        MapsEntry entry;
        if (parseMapsLine(line, entry)) entries.push_back(entry);
    }
    return entries;
}

std::string baseName(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

/* Anonymous mappings and pseudo paths ("[heap]", "[stack]", "[vdso]") are not
 * modules -- the Python module requires a '/' in the maps line as well. */
bool isRealPath(const std::string& path) {
    if (path.empty() || path[0] == '[') return false;
    return path.find('/') != std::string::npos;
}

/* /proc/pid/maps permissions ("r-xp") mapped onto the Windows PAGE_* values so
 * that the portable modules can filter on MemoryProtection everywhere. */
unsigned int permsToProtection(const std::string& perms) {
    if (perms.size() < 3) return 0x01u; /* PAGE_NOACCESS */
    const bool r = (perms[0] == 'r');
    const bool w = (perms[1] == 'w');
    const bool x = (perms[2] == 'x');
    if (x && r && w) return 0x40u; /* PAGE_EXECUTE_READWRITE */
    if (x && r) return 0x20u;      /* PAGE_EXECUTE_READ */
    if (x && w) return 0x80u;      /* PAGE_EXECUTE_WRITECOPY */
    if (x) return 0x10u;           /* PAGE_EXECUTE */
    if (r && w) return 0x04u;      /* PAGE_READWRITE */
    if (w) return 0x08u;           /* PAGE_WRITECOPY */
    if (r) return 0x02u;           /* PAGE_READONLY */
    return 0x01u;                  /* PAGE_NOACCESS */
}

std::string readFirstLine(const std::string& fileName) {
    std::ifstream in(fileName.c_str());
    std::string line;
    if (in.is_open()) std::getline(in, line);
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) line.pop_back();
    return line;
}

/* --------------------------------------------------------------------- X11 */
Display* gDisplay = nullptr;
bool gDisplayTried = false;
std::string gX11Error;

int x11ErrorHandler(Display* display, XErrorEvent* event) {
    char buffer[256];
    std::memset(buffer, 0, sizeof(buffer));
    if (display != nullptr) XGetErrorText(display, event->error_code, buffer, sizeof(buffer) - 1);
    gX11Error = std::string(buffer) + " (request " + std::to_string(static_cast<int>(event->request_code)) + "." +
                std::to_string(static_cast<int>(event->minor_code)) + ")";
    return 0; /* do not abort the process, the caller checks the return value */
}

Display* x11Display() {
    if (gDisplay == nullptr && !gDisplayTried) {
        gDisplayTried = true;
        gDisplay = XOpenDisplay(nullptr);
        if (gDisplay != nullptr) XSetErrorHandler(&x11ErrorHandler);
    }
    if (gDisplay == nullptr) setErrorText("Unable to open the X11 display (is $DISPLAY set? On Wayland an XWayland session is required)");
    return gDisplay;
}

Window x11RootWindow() { return DefaultRootWindow(x11Display()); }

std::vector<Window> x11EnumTopLevelWindows() {
    std::vector<Window> result;
    Display* display = x11Display();
    if (display == nullptr) return result;
    Window rootReturn = 0;
    Window parentReturn = 0;
    Window* children = nullptr;
    unsigned int count = 0;
    gX11Error.clear();
    if (XQueryTree(display, x11RootWindow(), &rootReturn, &parentReturn, &children, &count) != 0 && children != nullptr) {
        for (unsigned int i = 0; i < count; ++i) result.push_back(children[i]);
        XFree(children);
    }
    if (!gX11Error.empty()) setErrorText(gX11Error);
    return result;
}

std::string x11WindowTitle(Window window) {
    Display* display = x11Display();
    if (display == nullptr) return std::string();
    const Atom netWmName = XInternAtom(display, "_NET_WM_NAME", False);
    XTextProperty property;
    std::memset(&property, 0, sizeof(property));

    gX11Error.clear();
    if (netWmName != None && XGetTextProperty(display, window, &property, netWmName) != 0 && property.value != nullptr) {
        const std::string title(reinterpret_cast<const char*>(property.value), property.nitems);
        XFree(property.value);
        return title;
    }

    std::memset(&property, 0, sizeof(property));
    gX11Error.clear();
    if (XGetTextProperty(display, window, &property, XA_WM_NAME) != 0 && property.value != nullptr) {
        std::string title;
        char** list = nullptr;
        int listCount = 0;
        if (XmbTextPropertyToTextList(display, &property, &list, &listCount) >= Success && listCount > 0 && list != nullptr) {
            if (list[0] != nullptr) title = list[0];
            XFreeStringList(list);
        } else {
            title.assign(reinterpret_cast<const char*>(property.value), property.nitems);
        }
        XFree(property.value);
        return title;
    }
    return std::string();
}

long x11WindowPid(Window window) {
    Display* display = x11Display();
    if (display == nullptr) return 0;
    const Atom netWmPid = XInternAtom(display, "_NET_WM_PID", False);
    if (netWmPid == None) return 0;
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char* data = nullptr;
    gX11Error.clear();
    if (XGetWindowProperty(display, window, netWmPid, 0, 1, False, XA_CARDINAL, &actualType, &actualFormat, &itemCount,
                           &bytesAfter, &data) != Success ||
        data == nullptr) {
        return 0;
    }
    long pid = 0;
    if (itemCount > 0 && actualFormat == 32) pid = static_cast<long>(*(reinterpret_cast<unsigned long*>(data)));
    XFree(data);
    return pid;
}

int x11MaskShift(unsigned long mask) {
    if (mask == 0) return 0;
    int shift = 0;
    while (((mask >> shift) & 1ul) == 0) ++shift;
    return shift;
}

} /* namespace */

std::string osLastErrorText() { return gLastErrorText; }

/* ----------------------------------------------------------------- process */
bool osEnumProcesses(std::vector<OsProcess>& out) {
    out.clear();
    DIR* dir = opendir("/proc");
    if (dir == nullptr) {
        setErrorText(std::string("Unable to open /proc: ") + std::strerror(errno));
        return false;
    }
    std::vector<int> pids;
    while (dirent* entry = readdir(dir)) {
        const char* name = entry->d_name;
        if (name[0] < '0' || name[0] > '9') continue;
        bool numeric = true;
        for (const char* c = name + 1; *c != '\0'; ++c) {
            if (*c < '0' || *c > '9') {
                numeric = false;
                break;
            }
        }
        if (numeric) pids.push_back(std::atoi(name));
    }
    closedir(dir);

    std::sort(pids.begin(), pids.end());
    for (int pid : pids) {
        OsProcess process;
        process.pid = pid;
        process.name = readFirstLine("/proc/" + std::to_string(pid) + "/comm");
        if (process.name.empty()) continue; /* vanished or not readable */
        out.push_back(process);
    }
    return true;
}

bool osProcessAlive(int pid) { return pid > 0 && kill(static_cast<pid_t>(pid), 0) == 0; }

bool osProcessPath(int pid, std::string& out) {
    char buffer[4096];
    const ssize_t len = readlink(("/proc/" + std::to_string(pid) + "/exe").c_str(), buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        setErrorText(std::string("readlink failed: ") + std::strerror(errno));
        return false;
    }
    buffer[len] = '\0';
    out = buffer;
    return true;
}

bool osOpenProcess(int pid, void** handle) {
    if (handle != nullptr) *handle = nullptr; /* Linux does not need a handle */
    return osProcessAlive(pid);
}

void osCloseProcess(void* handle) { (void)handle; }

bool osEnumModules(int pid, std::vector<OsModule>& out) {
    out.clear();
    std::map<std::string, std::size_t> index;
    for (const MapsEntry& entry : readMaps(pid)) {
        if (!isRealPath(entry.path)) continue;
        const auto it = index.find(entry.path);
        if (it == index.end()) {
            OsModule module;
            module.name = baseName(entry.path);
            module.path = entry.path;
            module.base = entry.start;
            module.size = entry.end - entry.start;
            index[entry.path] = out.size();
            out.push_back(module);
        } else {
            OsModule& module = out[it->second];
            if (entry.start < module.base) {
                module.size += module.base - entry.start;
                module.base = entry.start;
            }
            if (entry.end > module.base + module.size) module.size = entry.end - module.base;
        }
    }
    return true;
}

bool osEnumRegions(int pid, void* handle, const OsModuleRef& module, std::vector<OsRegion>& out) {
    (void)handle;
    out.clear();
    for (const MapsEntry& entry : readMaps(pid)) {
        const bool belongs = (!module.path.empty() && entry.path == module.path) ||
                             (module.path.empty() && !entry.path.empty() && baseName(entry.path) == module.name);
        if (!belongs) continue;
        OsRegion region;
        region.start = entry.start;
        region.size = entry.end - entry.start;
        region.perms = entry.perms;
        region.readable = !entry.perms.empty() && entry.perms[0] == 'r';
        region.protection = permsToProtection(entry.perms);
        out.push_back(region);
    }
    return true;
}

bool osIsProcess64Bit(int pid, void* handle, bool& out) {
    (void)handle;
    std::ifstream exe(("/proc/" + std::to_string(pid) + "/exe").c_str(), std::ios::binary);
    if (!exe.is_open()) {
        setErrorText(std::string("Unable to read /proc/") + std::to_string(pid) + "/exe: " + std::strerror(errno));
        return false;
    }
    unsigned char header[5] = {0, 0, 0, 0, 0};
    exe.read(reinterpret_cast<char*>(header), sizeof(header));
    out = (header[4] == 2); /* ELFCLASS64 */
    return true;
}

/* ------------------------------------------------------------------ memory */
bool osReadMemory(int pid, void* handle, std::uint64_t address, void* dst, std::size_t size) {
    (void)handle;
    if (size == 0 || dst == nullptr) return true;
    std::size_t done = 0;
    while (done < size) {
        struct iovec local;
        struct iovec remote;
        local.iov_base = static_cast<char*>(dst) + done;
        local.iov_len = size - done;
        remote.iov_base = reinterpret_cast<void*>(static_cast<std::uintptr_t>(address) + done);
        remote.iov_len = size - done;
        const ssize_t transferred = process_vm_readv(static_cast<pid_t>(pid), &local, 1, &remote, 1, 0);
        if (transferred <= 0) {
            setErrorText(std::string("process_vm_readv failed: ") + std::strerror(errno));
            return false;
        }
        done += static_cast<std::size_t>(transferred);
    }
    return true;
}

bool osWriteMemory(int pid, void* handle, std::uint64_t address, const void* src, std::size_t size) {
    (void)handle;
    if (size == 0 || src == nullptr) return true;
    std::size_t done = 0;
    while (done < size) {
        struct iovec local;
        struct iovec remote;
        local.iov_base = const_cast<char*>(static_cast<const char*>(src)) + done;
        local.iov_len = size - done;
        remote.iov_base = reinterpret_cast<void*>(static_cast<std::uintptr_t>(address) + done);
        remote.iov_len = size - done;
        const ssize_t transferred = process_vm_writev(static_cast<pid_t>(pid), &local, 1, &remote, 1, 0);
        if (transferred <= 0) {
            setErrorText(std::string("process_vm_writev failed: ") + std::strerror(errno));
            return false;
        }
        done += static_cast<std::size_t>(transferred);
    }
    return true;
}

bool osSetPageProtection(int pid, void* handle, std::uint64_t address, std::size_t size, int protection, int& oldProtection) {
    (void)handle;
    (void)size; /* mprotect works on whole pages, ptrace::pageProtection handles it */
    oldProtection = 0; /* the previous protection is not reported by mprotect */
    try {
        ptrace::pageProtection(pid, static_cast<std::uintptr_t>(address), protection);
    } catch (const std::exception& e) {
        setErrorText(e.what());
        return false;
    }
    return true;
}

bool osAllocateMemory(int pid, void* handle, std::size_t size, int protection, std::uint64_t& out) {
    (void)handle;
    try {
        out = static_cast<std::uint64_t>(ptrace::allocateMemory(pid, size, protection));
    } catch (const std::exception& e) {
        setErrorText(e.what());
        return false;
    }
    return out != 0;
}

bool osFreeMemory(int pid, void* handle, std::uint64_t address) {
    (void)handle;
    try {
        return ptrace::freeMemory(pid, static_cast<std::uintptr_t>(address));
    } catch (const std::exception& e) {
        setErrorText(e.what());
        return false;
    }
}

bool osModuleProcAddress(const std::string& moduleName, const std::string& functionName, std::uint64_t& out) {
    (void)moduleName;
    (void)functionName;
    (void)out;
    setErrorText("getProcAddress is only supported on Windows");
    return false;
}

bool osCreateRemoteThread(void* handle, std::uint64_t startAddress, std::uint64_t param, std::uint64_t& exitCode) {
    (void)handle;
    (void)startAddress;
    (void)param;
    (void)exitCode;
    setErrorText("createRemoteThread is only supported on Windows");
    return false;
}

/* ------------------------------------------------------------ screen/window */
bool osDisplayResolution(int& width, int& height) {
    Display* display = x11Display();
    if (display == nullptr) return false;
    int eventBase = 0;
    int errorBase = 0;
    gX11Error.clear();
    if (XineramaQueryExtension(display, &eventBase, &errorBase) != 0 && XineramaIsActive(display) != 0) {
        int screenCount = 0;
        XineramaScreenInfo* screens = XineramaQueryScreens(display, &screenCount);
        if (screens != nullptr && screenCount > 0) {
            width = screens[0].width;
            height = screens[0].height;
            XFree(screens);
            return true;
        }
        if (screens != nullptr) XFree(screens);
    }
    const int screen = DefaultScreen(display);
    width = DisplayWidth(display, screen);
    height = DisplayHeight(display, screen);
    return width > 0 && height > 0;
}

bool osWindowRectByName(const std::string& name, OsWindowRect& out) {
    Display* display = x11Display();
    if (display == nullptr) return false;

    Window fallback = 0;
    Window match = 0;
    for (Window window : x11EnumTopLevelWindows()) {
        const std::string title = x11WindowTitle(window);
        if (title.empty()) continue;
        if (title == name) {
            match = window;
            break;
        }
        if (fallback == 0 && title.find(name) != std::string::npos) fallback = window;
    }
    if (match == 0) match = fallback;
    if (match == 0) {
        setErrorText("Window (" + name + ") not found");
        return false;
    }

    XWindowAttributes attributes;
    std::memset(&attributes, 0, sizeof(attributes));
    gX11Error.clear();
    if (XGetWindowAttributes(display, match, &attributes) == 0) {
        setErrorText(gX11Error.empty() ? "XGetWindowAttributes failed" : gX11Error);
        return false;
    }
    Window child = 0;
    int absoluteX = 0;
    int absoluteY = 0;
    XTranslateCoordinates(display, match, attributes.root, -attributes.border_width, -attributes.border_width, &absoluteX,
                          &absoluteY, &child);
    out.x = absoluteX;
    out.y = absoluteY;
    out.width = attributes.width;
    out.height = attributes.height;
    return true;
}

bool osWindowTitleByPid(int pid, std::string& out) {
    if (pid <= 0) return false;
    if (x11Display() == nullptr) return false;
    for (Window window : x11EnumTopLevelWindows()) {
        if (x11WindowPid(window) != static_cast<long>(pid)) continue;
        const std::string title = x11WindowTitle(window);
        if (title.empty()) continue;
        out = title;
        return true;
    }
    setErrorText("No Window found. PID: " + std::to_string(pid));
    return false;
}

void osSetDpiAware() { /* nothing to do on X11 */ }

/* ------------------------------------------------------------------- input */
bool osKeyPressed(int key, bool& pressed) {
    pressed = false;
    Display* display = x11Display();
    if (display == nullptr) return false;
    char keys[32];
    std::memset(keys, 0, sizeof(keys));
    gX11Error.clear();
    if (XQueryKeymap(display, keys) == 0) {
        if (!gX11Error.empty()) setErrorText(gX11Error);
        return false;
    }
    const KeyCode code = XKeysymToKeycode(display, static_cast<KeySym>(key));
    if (code == 0) {
        setErrorText("Key symbol 0x" + std::to_string(key) + " is not mapped in the current keyboard layout");
        return false;
    }
    pressed = (keys[code / 8] & (1 << (code % 8))) != 0;
    return true;
}

bool osFakeKey(int key, bool down) {
    Display* display = x11Display();
    if (display == nullptr) return false;
    const KeyCode code = XKeysymToKeycode(display, static_cast<KeySym>(key));
    if (code == 0) {
        setErrorText("Unknown key symbol: " + std::to_string(key));
        return false;
    }
    gX11Error.clear();
    const int result = XTestFakeKeyEvent(display, code, down ? 1 : 0, CurrentTime);
    XFlush(display);
    if (!gX11Error.empty()) setErrorText(gX11Error);
    return result != 0;
}

bool osMouseButtonState(int button, bool& down) {
    down = false;
    Display* display = x11Display();
    if (display == nullptr) return false;
    Window root = 0;
    Window child = 0;
    int rootX = 0;
    int rootY = 0;
    int childX = 0;
    int childY = 0;
    unsigned int mask = 0;
    gX11Error.clear();
    if (XQueryPointer(display, x11RootWindow(), &root, &child, &rootX, &rootY, &childX, &childY, &mask) == 0) {
        if (!gX11Error.empty()) setErrorText(gX11Error);
        return false;
    }
    unsigned int buttonMask = Button1Mask;
    if (button == 1)
        buttonMask = Button2Mask;
    else if (button == 2)
        buttonMask = Button3Mask;
    down = (mask & buttonMask) != 0;
    return true;
}

bool osFakeMouseButton(int button, bool down) {
    Display* display = x11Display();
    if (display == nullptr) return false;
    unsigned int xButton = 1;
    if (button == 1)
        xButton = 2;
    else if (button == 2)
        xButton = 3;
    gX11Error.clear();
    const int result = XTestFakeButtonEvent(display, xButton, down ? 1 : 0, CurrentTime);
    XFlush(display);
    if (!gX11Error.empty()) setErrorText(gX11Error);
    return result != 0;
}

bool osFakeMouseMove(int x, int y, bool relative) {
    Display* display = x11Display();
    if (display == nullptr) return false;
    gX11Error.clear();
    const int result =
        relative ? XTestFakeRelativeMotionEvent(display, x, y, CurrentTime) : XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
    XFlush(display);
    if (!gX11Error.empty()) setErrorText(gX11Error);
    return result != 0;
}

bool osCursorPosition(int& x, int& y) {
    Display* display = x11Display();
    if (display == nullptr) return false;
    Window root = 0;
    Window child = 0;
    int rootX = 0;
    int rootY = 0;
    int childX = 0;
    int childY = 0;
    unsigned int mask = 0;
    gX11Error.clear();
    if (XQueryPointer(display, x11RootWindow(), &root, &child, &rootX, &rootY, &childX, &childY, &mask) == 0) return false;
    x = rootX;
    y = rootY;
    return true;
}

/* ------------------------------------------------------------------- pixel */
bool osCaptureRegion(int x, int y, int width, int height, std::vector<unsigned char>& rgbaOut) {
    Display* display = x11Display();
    if (display == nullptr || width <= 0 || height <= 0) return false;

    rgbaOut.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0);
    for (std::size_t i = 3; i < rgbaOut.size(); i += 4) rgbaOut[i] = 255;

    const int screenWidth = DisplayWidth(display, DefaultScreen(display));
    const int screenHeight = DisplayHeight(display, DefaultScreen(display));
    const int srcX = std::max(0, std::min(x, screenWidth));
    const int srcY = std::max(0, std::min(y, screenHeight));
    const int srcWidth = std::min(width, screenWidth - srcX);
    const int srcHeight = std::min(height, screenHeight - srcY);
    if (srcWidth <= 0 || srcHeight <= 0) {
        setErrorText("Capture region is outside of the screen");
        return false;
    }

    gX11Error.clear();
    XImage* image = XGetImage(display, x11RootWindow(), srcX, srcY, static_cast<unsigned int>(srcWidth),
                              static_cast<unsigned int>(srcHeight), AllPlanes, ZPixmap);
    if (image == nullptr) {
        setErrorText(gX11Error.empty() ? "XGetImage returned no image" : gX11Error);
        return false;
    }

    const int redShift = x11MaskShift(image->red_mask);
    const int greenShift = x11MaskShift(image->green_mask);
    const int blueShift = x11MaskShift(image->blue_mask);

    for (int row = 0; row < srcHeight; ++row) {
        for (int col = 0; col < srcWidth; ++col) {
            const unsigned long pixel = static_cast<unsigned long>(XGetPixel(image, col, row));
            const std::size_t index =
                (static_cast<std::size_t>(row) * static_cast<std::size_t>(width) + static_cast<std::size_t>(col)) * 4u;
            rgbaOut[index + 0] = static_cast<unsigned char>((pixel & image->red_mask) >> redShift);
            rgbaOut[index + 1] = static_cast<unsigned char>((pixel & image->green_mask) >> greenShift);
            rgbaOut[index + 2] = static_cast<unsigned char>((pixel & image->blue_mask) >> blueShift);
            rgbaOut[index + 3] = 255;
        }
    }
    XDestroyImage(image);
    return true;
}

} /* namespace detail */
} /* namespace pymeow */

#endif /* PYMEOW_LINUX */
