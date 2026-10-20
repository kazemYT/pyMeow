/*
 * pyMeow C++ -- keyboard & mouse implementation (port of input.nim)
 * https://github.com/qb-0/pyMeow
 *
 * Portable: the actual key/mouse work is done by the platform layer
 * (XTest on Linux, SendInput on Windows).
 */
#include "pymeow/input.hpp"

#include "pymeow/error.hpp"
#include "pymeow/internal/platform.hpp"
#include "pymeow/utils.hpp"

#include <string>

namespace pymeow {

namespace {

int buttonIndex(MouseButton button) {
    switch (button) {
        case MouseButton::Middle: return 1;
        case MouseButton::Right: return 2;
        case MouseButton::Left:
        default: return 0;
    }
}

void throwOsError(const std::string& context) {
    std::string message = context;
    const std::string platformError = detail::osLastErrorText();
    if (!platformError.empty()) {
        message += ": " + platformError;
    } else {
        const std::pair<int, std::string> err = getOsError();
        if (err.first != 0) message += " (" + std::to_string(err.first) + ": " + err.second + ")";
    }
    throw Error(message);
}

} /* namespace */

MouseButton mouseButtonFromString(const std::string& button) {
    if (button == "middle" || button == "center") return MouseButton::Middle;
    if (button == "right") return MouseButton::Right;
    return MouseButton::Left;
}

bool keyPressed(int key) {
    bool pressed = false;
    if (!detail::osKeyPressed(key, pressed)) throwOsError("Could not read the key state");
    return pressed;
}

void pressKey(int key, bool hold) {
    if (!detail::osFakeKey(key, true)) throwOsError("Could not simulate the key press");
    if (!hold) detail::osFakeKey(key, false);
}

void releaseKey(int key) {
    if (!detail::osFakeKey(key, false)) throwOsError("Could not simulate the key release");
}

bool mousePressed(MouseButton button) {
    bool down = false;
    if (!detail::osMouseButtonState(buttonIndex(button), down)) throwOsError("Could not read the mouse button state");
    return down;
}

void mouseMove(int x, int y, bool relative) {
    if (!detail::osFakeMouseMove(x, y, relative)) throwOsError("Could not move the mouse");
}

void mouseDown(MouseButton button) {
    if (!detail::osFakeMouseButton(buttonIndex(button), true)) throwOsError("Could not press the mouse button");
}

void mouseUp(MouseButton button) {
    if (!detail::osFakeMouseButton(buttonIndex(button), false)) throwOsError("Could not release the mouse button");
}

Vec2 mousePosition() {
    int x = 0;
    int y = 0;
    if (!detail::osCursorPosition(x, y)) throwOsError("Could not read the mouse position");
    return Vec2(static_cast<float>(x), static_cast<float>(y));
}

/* --------------------------------------------- string overloads (Python API) */
bool mousePressed(const std::string& button) { return mousePressed(mouseButtonFromString(button)); }
void mouseDown(const std::string& button) { mouseDown(mouseButtonFromString(button)); }
void mouseUp(const std::string& button) { mouseUp(mouseButtonFromString(button)); }

void mouseClick(MouseButton button, int holdMs) {
    mouseDown(button);
    sleepMs(holdMs);
    mouseUp(button);
}

void mouseClick(const std::string& button, int holdMs) { mouseClick(mouseButtonFromString(button), holdMs); }

} /* namespace pymeow */
