/*
 * pyMeow C++ -- utilities implementation (port of utils.nim)
 * https://github.com/qb-0/pyMeow
 */
#include "pymeow/utils.hpp"

#include "pymeow/error.hpp"
#include "pymeow/internal/platform.hpp"
#include "pymeow/internal/strings.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace pymeow {

/* ==================================================================== colors */
namespace {

struct NamedColor {
    const char* name;
    unsigned int rgb;
};

#define PM_COLOR_ENTRY(name, value) {name, value},
const NamedColor kNamedColors[] = {
#include "pymeow/internal/color_names.inc"
};
#undef PM_COLOR_ENTRY

const std::size_t kNamedColorCount = sizeof(kNamedColors) / sizeof(kNamedColors[0]);

const NamedColor* findNamedColor(const std::string& name) {
    /* Only 140 entries: a case insensitive linear scan (like Nim's
     * colorNameCmp) is fast enough and can not get out of sync with the table. */
    for (std::size_t i = 0; i < kNamedColorCount; ++i) {
        if (detail::iequals(kNamedColors[i].name, name)) return &kNamedColors[i];
    }
    return nullptr;
}

bool isHexString(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (std::isxdigit(static_cast<unsigned char>(c)) == 0) return false;
    }
    return true;
}

inline Color rgbToColor(unsigned int rgb) { return Color((rgb << 8) | 0xFFu); }

} /* namespace */

Color newColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) { return Color(r, g, b, a); }

Color newColorHex(std::uint32_t hexValue) { return Color(hexValue); }

Color newColorFloat(float r, float g, float b, float a) {
    const auto clamp255 = [](float v) -> std::uint8_t {
        if (v <= 0.0f) return 0;
        if (v >= 1.0f) return 255;
        return static_cast<std::uint8_t>(v * 255.0f);
    };
    return Color(clamp255(r), clamp255(g), clamp255(b), clamp255(a));
}

bool tryParseColor(const std::string& colorName, Color& out) {
    std::string name = detail::trim(colorName);
    if (name.empty()) return false;

    if (name[0] == '#') {
        name = name.substr(1);
    } else if (name.size() > 2 && name[0] == '0' && (name[1] == 'x' || name[1] == 'X')) {
        name = name.substr(2);
    }

    if ((name.size() == 6 || name.size() == 8) && isHexString(name)) {
        const std::uint32_t value = static_cast<std::uint32_t>(std::strtoul(name.c_str(), nullptr, 16));
        out = (name.size() == 6) ? rgbToColor(value) : Color(value); /* RRGGBB or RRGGBBAA */
        return true;
    }

    const NamedColor* named = findNamedColor(name);
    if (named != nullptr) {
        out = rgbToColor(named->rgb);
        return true;
    }
    return false;
}

Color parseColor(const std::string& colorName) {
    Color result;
    if (!tryParseColor(colorName, result)) throw InvalidArgumentError("unknown color: " + colorName);
    return result;
}

Color getColor(const std::string& colorName) {
    Color result;
    if (!tryParseColor(colorName, result)) return Color(0, 0, 0, 255); /* like the Python module */
    return result;
}

const std::vector<std::pair<std::string, Color>>& allColors() {
    static const std::vector<std::pair<std::string, Color>> table = [] {
        std::vector<std::pair<std::string, Color>> colors;
        colors.reserve(kNamedColorCount);
        for (std::size_t i = 0; i < kNamedColorCount; ++i)
            colors.emplace_back(std::string(kNamedColors[i].name), rgbToColor(kNamedColors[i].rgb));
        return colors;
    }();
    return table;
}

bool isColorName(const std::string& name) { return findNamedColor(detail::trim(name)) != nullptr; }

Color fadeColor(Color color, float alpha) {
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    color.a = static_cast<std::uint8_t>(255.0f * alpha);
    return color;
}

float compareColorPct(Color color1, Color color2) {
    const float r = std::abs(static_cast<int>(color1.r) - static_cast<int>(color2.r)) / 255.0f;
    const float g = std::abs(static_cast<int>(color1.g) - static_cast<int>(color2.g)) / 255.0f;
    const float b = std::abs(static_cast<int>(color1.b) - static_cast<int>(color2.b)) / 255.0f;
    return 100.0f - ((r + g + b) / 3.0f * 100.0f);
}

/* ========================================================= window / screen */
std::pair<int, int> getDisplayResolution() {
    int width = 0;
    int height = 0;
    if (detail::osDisplayResolution(width, height)) return std::make_pair(width, height);
    return std::make_pair(0, 0);
}

WindowInfo getWindowInfo(const std::string& name) {
    detail::OsWindowRect rect;
    if (!detail::osWindowRectByName(name, rect)) {
        const std::string platformError = detail::osLastErrorText();
        if (!platformError.empty()) throw Error(platformError);
        throw NotFoundError("Window (" + name + ") not found");
    }
    WindowInfo result;
    result.x = rect.x;
    result.y = rect.y;
    result.width = rect.width;
    result.height = rect.height;
    return result;
}

std::string getWindowTitle(int processId) {
    std::string title;
    if (!detail::osWindowTitleByPid(processId, title)) {
        const std::string platformError = detail::osLastErrorText();
        if (!platformError.empty()) throw Error(platformError);
        throw NotFoundError("No Window found. PID: " + std::to_string(processId));
    }
    return title;
}

/* ===================================================================== misc */
double runTime() {
    static const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

void sleepMs(int milliseconds) {
    if (milliseconds > 0) std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

/* ========================================================= world to screen */
namespace {

WtsResult project(const float* matrix, Vec3 pos, int screenWidth, int screenHeight, int algo) {
    Vec3 clip;
    if (algo == 0) {
        clip.z = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];
        clip.x = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
        clip.y = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
    } else {
        clip.z = pos.x * matrix[12] + pos.y * matrix[13] + pos.z * matrix[14] + matrix[15];
        clip.x = pos.x * matrix[0] + pos.y * matrix[1] + pos.z * matrix[2] + matrix[3];
        clip.y = pos.x * matrix[4] + pos.y * matrix[5] + pos.z * matrix[6] + matrix[7];
    }

    WtsResult result;
    result.inBounds = clip.z >= 0.2f;
    if (clip.z == 0.0f) return result; /* no projection possible */

    const Vec2 ndc(clip.x / clip.z, clip.y / clip.z);
    const float halfWidth = static_cast<float>(screenWidth) / 2.0f;
    const float halfHeight = static_cast<float>(screenHeight) / 2.0f;
    /* Same formula as world_to_screen() of the Python module. */
    result.pos.x = (halfWidth * ndc.x) + (ndc.x + halfWidth);
    result.pos.y = -(halfHeight * ndc.y) + (ndc.y + halfHeight);
    return result;
}

} /* namespace */

Vec2 worldToScreen(const float* matrix, Vec3 pos, int screenWidth, int screenHeight, int algo) {
    const WtsResult result = project(matrix, pos, screenWidth, screenHeight, algo);
    if (!result.inBounds) throw OutOfBoundsError();
    return result.pos;
}

WtsResult worldToScreenNoExc(const float* matrix, Vec3 pos, int screenWidth, int screenHeight, int algo) {
    return project(matrix, pos, screenWidth, screenHeight, algo);
}

Vec2 worldToScreen(const Matrix& matrix, Vec3 pos, int screenWidth, int screenHeight, int algo) {
    return worldToScreen(matrix.data(), pos, screenWidth, screenHeight, algo);
}

WtsResult worldToScreenNoExc(const Matrix& matrix, Vec3 pos, int screenWidth, int screenHeight, int algo) {
    return worldToScreenNoExc(matrix.data(), pos, screenWidth, screenHeight, algo);
}

} /* namespace pymeow */
