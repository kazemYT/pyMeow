/*
 * pyMeow C++ -- overlay window (port of overlay.nim)
 * https://github.com/qb-0/pyMeow
 *
 * Creates a transparent, always on top, click through raylib window that is
 * used to draw an ESP / radar over a game.
 *
 *   pm::OverlayConfig cfg;
 *   cfg.target = "AssaultCube";
 *   cfg.fps = 144;
 *   pm::overlayInit(cfg);
 *   while (pm::overlayLoop()) {
 *       pm::beginDrawing();
 *       ...
 *       pm::endDrawing();
 *   }
 *
 * Requires raylib. Not compiled when PYMEOW_NO_GRAPHICS is defined.
 */
#ifndef PYMEOW_OVERLAY_HPP
#define PYMEOW_OVERLAY_HPP

#include "common.hpp"

#if defined(PYMEOW_WITH_RAYLIB)

#include "types.hpp"
#include "utils.hpp"

#include <string>

namespace pymeow {

struct PYMEOW_API OverlayConfig {
    std::string target = "Full"; /* window title to overlay, or "Full" for the whole screen */
    int fps = 0;                 /* 0 == unlimited */
    std::string title = "pyMeow";
    int logLevel = 5;            /* raylib trace log level (LOG_ERROR) */
    int exitKey = -1;            /* -1 == platform default (VK_END / XK_End) */
    bool trackTarget = false;    /* follow the target window when it moves / resizes */
    bool transparent = true;
    bool topmost = true;
    bool mousePassthrough = true;
};

PYMEOW_API void overlayInit();
PYMEOW_API void overlayInit(const OverlayConfig& config);
PYMEOW_API void overlayInit(const std::string& target, int fps = 0, const std::string& title = "pyMeow", int logLevel = 5,
                            int exitKey = -1, bool trackTarget = false);

/* Handles the exit key, target tracking and clears the background.
 * Returns false when the overlay should be closed. */
PYMEOW_API bool overlayLoop();
PYMEOW_API void overlayClose();
/* Toggles WINDOW_MOUSE_PASSTHROUGH so the GUI can be clicked. */
PYMEOW_API void toggleMouse();

PYMEOW_API const OverlayConfig& overlayConfig();
PYMEOW_API bool overlayReady();

PYMEOW_API void beginDrawing();
PYMEOW_API void endDrawing();
PYMEOW_API int getFps();
PYMEOW_API int getScreenWidth();
PYMEOW_API int getScreenHeight();
PYMEOW_API Vec2 getWindowPosition();
PYMEOW_API void* getWindowHandle(); /* HWND on Windows, nullptr / void* elsewhere */

PYMEOW_API void setFps(int fps);
PYMEOW_API void setWindowPosition(int x, int y);
PYMEOW_API void setWindowSize(int width, int height);
PYMEOW_API void setWindowTitle(const std::string& title);
PYMEOW_API void setWindowIcon(const std::string& filePath);
PYMEOW_API void setWindowMonitor(int monitor);
PYMEOW_API void setWindowFlag(unsigned int flag); /* toggles a raylib window state flag */
PYMEOW_API void setLogLevel(int level);
PYMEOW_API void takeScreenshot(const std::string& fileName);

PYMEOW_API int getMonitorCount();
PYMEOW_API std::string getMonitorName(int monitor = 0);
PYMEOW_API int getMonitorRefreshRate(int monitor = 0);

} /* namespace pymeow */

#endif /* PYMEOW_WITH_RAYLIB */

#endif /* PYMEOW_OVERLAY_HPP */
