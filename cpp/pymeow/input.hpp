/*
 * pyMeow C++ -- keyboard & mouse (port of input.nim)
 * https://github.com/qb-0/pyMeow
 *
 * Windows: GetAsyncKeyState / SendInput / SetCursorPos
 * Linux:   X11 (XQueryKeymap, XTest, XQueryPointer) -- requires libX11 + libXtst
 *
 * Keys are virtual key codes on Windows and X11 key symbols on Linux, see
 * keys.hpp for portable names.
 */
#ifndef PYMEOW_INPUT_HPP
#define PYMEOW_INPUT_HPP

#include "common.hpp"
#include "types.hpp"

#include <string>

namespace pymeow {

enum class MouseButton { Left, Middle, Right };

/* "left" / "middle" / "right" (the strings the Python API uses). */
PYMEOW_API MouseButton mouseButtonFromString(const std::string& button);

/* Global key state, independent of the focused window. */
PYMEOW_API bool keyPressed(int key);
/* Sends a key press (down + up). With hold = true only the key down event is
 * sent, release it with releaseKey(). */
PYMEOW_API void pressKey(int key, bool hold = false);
PYMEOW_API void releaseKey(int key);

PYMEOW_API bool mousePressed(MouseButton button = MouseButton::Left);
PYMEOW_API bool mousePressed(const std::string& button);
PYMEOW_API void mouseMove(int x, int y, bool relative = false);
PYMEOW_API void mouseDown(MouseButton button = MouseButton::Left);
PYMEOW_API void mouseDown(const std::string& button);
PYMEOW_API void mouseUp(MouseButton button = MouseButton::Left);
PYMEOW_API void mouseUp(const std::string& button);
PYMEOW_API void mouseClick(MouseButton button = MouseButton::Left, int holdMs = 3);
PYMEOW_API void mouseClick(const std::string& button, int holdMs = 3);
PYMEOW_API Vec2 mousePosition();

} /* namespace pymeow */

#endif /* PYMEOW_INPUT_HPP */
