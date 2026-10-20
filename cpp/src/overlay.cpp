/*
 * pyMeow C++ -- overlay window implementation (port of overlay.nim)
 * https://github.com/qb-0/pyMeow
 */
#include "pymeow/overlay.hpp"

#if defined(PYMEOW_WITH_RAYLIB)

#  include "pymeow/error.hpp"
#  include "pymeow/input.hpp"
#  include "pymeow/internal/platform.hpp"
#  include "pymeow/keys.hpp"

#  include <cstring>

namespace pymeow {

namespace {

struct OverlayState {
    OverlayConfig config;   /* configuration the overlay was started with */
    OverlayConfig requested; /* configuration handed to overlayInit        */
    int targetX = 0;
    int targetY = 0;
    int targetWidth = 0;
    int targetHeight = 0;
    int exitKey = -1;
    bool ready = false;
};

OverlayState gOverlay;

} /* namespace */

const OverlayConfig& overlayConfig() { return gOverlay.config; }

bool overlayReady() { return gOverlay.ready; }

void overlayInit(const OverlayConfig& config) {
    if (gOverlay.ready) throw Error("overlayInit: the overlay is already initialized");

    gOverlay.requested = config;
    gOverlay.config = config;

    ::SetTraceLogLevel(config.logLevel);
    ::SetTargetFPS(config.fps);
    ::SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    if (config.mousePassthrough) ::SetConfigFlags(FLAG_WINDOW_MOUSE_PASSTHROUGH);
    if (config.transparent) ::SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    if (config.topmost) ::SetConfigFlags(FLAG_WINDOW_TOPMOST);

    detail::osSetDpiAware();
#  if defined(PYMEOW_WINDOWS)
    /* Multisampling voids the transparent frame buffer on most linux distributions. */
    ::SetConfigFlags(FLAG_MSAA_4X_HINT);
#  endif

    const std::pair<int, int> resolution = getDisplayResolution();
    ::InitWindow(resolution.first > 1 ? resolution.first - 1 : resolution.first,
                 resolution.second > 1 ? resolution.second - 1 : resolution.second, config.title.c_str());
    gOverlay.ready = ::IsWindowReady();
    if (!gOverlay.ready) throw Error("overlayInit: raylib was unable to create the overlay window");

    const bool fullScreen = config.target.empty() || config.target == "Full";
    if (!fullScreen) {
        const WindowInfo info = getWindowInfo(config.target);
        gOverlay.targetX = info.x;
        gOverlay.targetY = info.y;
        gOverlay.targetWidth = info.width;
        gOverlay.targetHeight = info.height;
        ::SetWindowSize(info.width, info.height);
        ::SetWindowPosition(info.x, info.y);
        ::SetTargetFPS(config.fps);
    } else if (config.trackTarget) {
        ::CloseWindow();
        gOverlay.ready = false;
        throw InvalidArgumentError("Target tracking is not supported in Fullscreen mode");
    }

    gOverlay.exitKey = (config.exitKey != -1) ? config.exitKey : keys::End;
    ::SetExitKey(KEY_NULL); /* the exit key is handled in overlayLoop() */
}

void overlayInit() { overlayInit(OverlayConfig()); }

void overlayInit(const std::string& target, int fps, const std::string& title, int logLevel, int exitKey, bool trackTarget) {
    OverlayConfig config;
    config.target = target;
    config.fps = fps;
    config.title = title;
    config.logLevel = logLevel;
    config.exitKey = exitKey;
    config.trackTarget = trackTarget;
    overlayInit(config);
}

bool overlayLoop() {
    ::ClearBackground(BLANK);

    if (gOverlay.exitKey >= 0 && keyPressed(gOverlay.exitKey)) ::CloseWindow();

    if (gOverlay.requested.trackTarget && !gOverlay.requested.target.empty() && gOverlay.requested.target != "Full") {
        try {
            const WindowInfo info = getWindowInfo(gOverlay.requested.target);
            if (info.x != gOverlay.targetX || info.y != gOverlay.targetY) {
                gOverlay.targetX = info.x;
                gOverlay.targetY = info.y;
                ::SetWindowPosition(info.x, info.y);
            }
            if (info.width != gOverlay.targetWidth || info.height != gOverlay.targetHeight) {
                gOverlay.targetWidth = info.width;
                gOverlay.targetHeight = info.height;
                ::SetWindowSize(info.width, info.height);
            }
        } catch (const NotFoundError&) {
            /* The target window is temporarily gone (minimized / restarting). */
        }
    }

    return ::WindowShouldClose() == false;
}

void overlayClose() {
    if (gOverlay.ready) {
        ::CloseWindow();
        gOverlay.ready = false;
    }
}

void toggleMouse() {
    if (::IsWindowState(FLAG_WINDOW_MOUSE_PASSTHROUGH)) {
        ::ClearWindowState(FLAG_WINDOW_MOUSE_PASSTHROUGH);
        return;
    }
    ::SetWindowState(FLAG_WINDOW_MOUSE_PASSTHROUGH);
}

void beginDrawing() { ::BeginDrawing(); }
void endDrawing() { ::EndDrawing(); }

int getFps() { return ::GetFPS(); }
int getScreenWidth() { return ::GetScreenWidth(); }
int getScreenHeight() { return ::GetScreenHeight(); }
Vec2 getWindowPosition() { return Vec2(::GetWindowPosition()); }

void* getWindowHandle() { return ::GetWindowHandle(); }

void setFps(int fps) { ::SetTargetFPS(fps); }
void setWindowPosition(int x, int y) { ::SetWindowPosition(x, y); }
void setWindowSize(int width, int height) { ::SetWindowSize(width, height); }
void setWindowTitle(const std::string& title) { ::SetWindowTitle(title.c_str()); }

void setWindowIcon(const std::string& filePath) {
    ::Image icon = ::LoadImage(filePath.c_str());
    ::SetWindowIcon(icon);
    ::UnloadImage(icon);
}

void setWindowMonitor(int monitor) { ::SetWindowMonitor(monitor); }

void setWindowFlag(unsigned int flag) {
    if (::IsWindowState(flag)) {
        ::ClearWindowState(flag);
        return;
    }
    ::SetWindowState(flag);
}

void setLogLevel(int level) { ::SetTraceLogLevel(level); }
void takeScreenshot(const std::string& fileName) { ::TakeScreenshot(fileName.c_str()); }

int getMonitorCount() { return ::GetMonitorCount(); }

std::string getMonitorName(int monitor) {
    const char* name = ::GetMonitorName(monitor);
    return (name != nullptr) ? std::string(name) : std::string();
}

int getMonitorRefreshRate(int monitor) { return ::GetMonitorRefreshRate(monitor); }

} /* namespace pymeow */

#endif /* PYMEOW_WITH_RAYLIB */
