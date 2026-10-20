/*
 * pyMeow C++ -- utilities (port of utils.nim, without the raylib parts)
 * https://github.com/qb-0/pyMeow
 *
 * Colors, screen / window information and the world to screen projection.
 * Everything in this header works without raylib; the drawing related helpers
 * (measureText, collision checks, monitor information) live in render.hpp and
 * overlay.hpp.
 */
#ifndef PYMEOW_UTILS_HPP
#define PYMEOW_UTILS_HPP

#include "common.hpp"
#include "types.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace pymeow {

/* ------------------------------------------------------------------- colors */

PYMEOW_API Color newColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255);
/* 0xRRGGBBAA, same layout as raylib's GetColor(). */
PYMEOW_API Color newColorHex(std::uint32_t hexValue);
/* Components in the 0.0 - 1.0 range. */
PYMEOW_API Color newColorFloat(float r, float g, float b, float a = 1.0f);

/* Accepts a color name ("cyan", "OrangeRed"), "#RRGGBB", "#RRGGBBAA" or
 * "0xRRGGBB". Returns black when the name is unknown, exactly like the Python
 * get_color(); use parseColor() to get an exception instead. */
PYMEOW_API Color getColor(const std::string& colorName);
PYMEOW_API Color parseColor(const std::string& colorName);
PYMEOW_API bool tryParseColor(const std::string& colorName, Color& out);

/* Color name table (the 140 CSS / X11 names the Python module supports). */
PYMEOW_API const std::vector<std::pair<std::string, Color>>& allColors();
PYMEOW_API bool isColorName(const std::string& name);

/* Applies an alpha value (0.0 - 1.0) to a color. */
PYMEOW_API Color fadeColor(Color color, float alpha);
/* Similarity of two colors in percent (100 == identical), ignoring alpha. */
PYMEOW_API float compareColorPct(Color color1, Color color2);

/* ------------------------------------------------------------ window / screen */

/* Resolution of the primary screen / monitor. */
PYMEOW_API std::pair<int, int> getDisplayResolution();

struct PYMEOW_API WindowInfo {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

/* Position and size of a window, found by title.
 * Windows: FindWindow() (exact title). Linux: X11 (exact or substring match).
 * Throws NotFoundError when the window does not exist. */
PYMEOW_API WindowInfo getWindowInfo(const std::string& name);
/* Title of the main window of a process (UTF-8). Throws NotFoundError. */
PYMEOW_API std::string getWindowTitle(int processId);

/* --------------------------------------------------------------------- misc */

/* Seconds since the first call (monotonic). */
PYMEOW_API double runTime();
PYMEOW_API void sleepMs(int milliseconds);

/* --------------------------------------------------------- world to screen */

struct PYMEOW_API WtsResult {
    bool inBounds = false;
    Vec2 pos;
};

/* Projects a world position with a 4x4 view (projection) matrix.
 * algo 0: column major layout used by most games (AssaultCube, CS:GO, ...)
 * algo 1: transposed layout
 * Throws OutOfBoundsError when the position is behind the camera
 * (clip.z < 0.2), like the Python world_to_screen(). */
PYMEOW_API Vec2 worldToScreen(const Matrix& matrix, Vec3 pos, int screenWidth, int screenHeight, int algo = 0);
PYMEOW_API WtsResult worldToScreenNoExc(const Matrix& matrix, Vec3 pos, int screenWidth, int screenHeight, int algo = 0);
PYMEOW_API Vec2 worldToScreen(const float* matrix, Vec3 pos, int screenWidth, int screenHeight, int algo = 0);
PYMEOW_API WtsResult worldToScreenNoExc(const float* matrix, Vec3 pos, int screenWidth, int screenHeight, int algo = 0);

} /* namespace pymeow */

#endif /* PYMEOW_UTILS_HPP */
