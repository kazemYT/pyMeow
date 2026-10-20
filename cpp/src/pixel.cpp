/*
 * pyMeow C++ -- pixel scanning implementation (port of pixel.nim)
 * https://github.com/qb-0/pyMeow
 */
#include "pymeow/pixel.hpp"

#include "pymeow/error.hpp"
#include "pymeow/input.hpp"
#include "pymeow/internal/png.hpp"
#include "pymeow/utils.hpp"

#include <algorithm>
#include <cstring>

#include "pymeow/internal/platform.hpp"

namespace pymeow {

namespace {

/* A captured region, always stored as RGBA8. */
struct Capture {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;

    Color at(int px, int py) const {
        const std::size_t index = (static_cast<std::size_t>(py) * static_cast<std::size_t>(width) + static_cast<std::size_t>(px)) * 4u;
        if (index + 3 >= rgba.size()) return Color(0, 0, 0, 255);
        return Color(rgba[index], rgba[index + 1], rgba[index + 2], rgba[index + 3]);
    }
};

/* The capture itself is implemented by the platform layer: X11 (XGetImage) on
 * Linux, GDI (BitBlt/GetDIBits) on Windows. Both deliver RGBA8. */
Capture captureRegion(int x, int y, int width, int height) {
    Capture capture;
    capture.x = x;
    capture.y = y;
    capture.width = width;
    capture.height = height;
    capture.rgba.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0);
    for (std::size_t i = 3; i < capture.rgba.size(); i += 4) capture.rgba[i] = 255; /* alpha */

    if (width <= 0 || height <= 0) return capture;

    if (!detail::osCaptureRegion(x, y, width, height, capture.rgba)) {
        const std::string platformError = detail::osLastErrorText();
        throw Error(platformError.empty() ? "Pixel capture failed" : ("Pixel capture failed: " + platformError));
    }
    return capture;
}

void forEachCapturePixel(const Capture& capture, bool absoluteCoords, const std::function<void(const Pixel&)>& fn) {
    for (int row = 0; row < capture.height; ++row) {
        for (int col = 0; col < capture.width; ++col) {
            Pixel pixel;
            pixel.x = absoluteCoords ? capture.x + col : col;
            pixel.y = absoluteCoords ? capture.y + row : row;
            pixel.color = capture.at(col, row);
            fn(pixel);
        }
    }
}

std::string withPngExtension(const std::string& fileName) {
    if (fileName.size() >= 4) {
        const std::string ext = fileName.substr(fileName.size() - 4);
        if (ext == ".png" || ext == ".PNG" || ext == ".Png") return fileName;
    }
    return fileName + ".png";
}

} /* namespace */

void forEachPixelInRegion(int x, int y, int width, int height, const std::function<void(const Pixel&)>& fn) {
    if (width <= 0 || height <= 0) return;
    Capture capture = captureRegion(x, y, width, height);
    capture.x = x;
    capture.y = y;
    forEachCapturePixel(capture, false, fn);
}

std::vector<Pixel> pixelEnumRegion(int x, int y, int width, int height, bool absoluteCoords) {
    std::vector<Pixel> result;
    if (width <= 0 || height <= 0) return result;
    result.reserve(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    forEachPixelInRegion(x, y, width, height, [&](const Pixel& pixel) {
        Pixel p = pixel;
        if (absoluteCoords) {
            p.x += x;
            p.y += y;
        }
        result.push_back(p);
    });
    return result;
}

std::vector<Pixel> pixelEnumScreen(bool absoluteCoords) {
    const std::pair<int, int> resolution = getDisplayResolution();
    return pixelEnumRegion(0, 0, resolution.first, resolution.second, absoluteCoords);
}

Pixel getPixel(int x, int y) {
    Capture capture = captureRegion(x, y, 1, 1);
    capture.x = x;
    capture.y = y;
    Pixel pixel;
    pixel.x = x;
    pixel.y = y;
    pixel.color = capture.at(0, 0);
    return pixel;
}

Pixel pixelAtMouse() {
    const Vec2 position = mousePosition();
    return getPixel(static_cast<int>(position.x), static_cast<int>(position.y));
}

bool pixelSaveToFile(int x, int y, int width, int height, const std::string& fileName) {
    if (width <= 0 || height <= 0) return false;
    const Capture capture = captureRegion(x, y, width, height);
    return detail::writePng(withPngExtension(fileName), width, height, capture.rgba.data());
}

bool savePixelsAsPng(const std::vector<Pixel>& pixels, int width, int height, const std::string& fileName) {
    if (width <= 0 || height <= 0) return false;
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0);
    for (std::size_t i = 3; i < rgba.size(); i += 4) rgba[i] = 255;
    for (const Pixel& pixel : pixels) {
        if (pixel.x < 0 || pixel.y < 0 || pixel.x >= width || pixel.y >= height) continue;
        const std::size_t index =
            (static_cast<std::size_t>(pixel.y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(pixel.x)) * 4u;
        rgba[index + 0] = pixel.color.r;
        rgba[index + 1] = pixel.color.g;
        rgba[index + 2] = pixel.color.b;
        rgba[index + 3] = pixel.color.a;
    }
    return detail::writePng(withPngExtension(fileName), width, height, rgba.data());
}

std::vector<Pixel> pixelSearchColors(int x, int y, int width, int height, const std::vector<Color>& colors, float similarity,
                                     bool absoluteCoords) {
    std::vector<Pixel> result;
    if (width <= 0 || height <= 0 || colors.empty()) return result;
    const Capture capture = captureRegion(x, y, width, height);
    forEachCapturePixel(
        capture, false, [&](const Pixel& pixel) {
            for (const Color& color : colors) {
                if (compareColorPct(pixel.color, color) >= similarity) {
                    Pixel match = pixel;
                    if (absoluteCoords) {
                        match.x += x;
                        match.y += y;
                    }
                    result.push_back(match);
                    break;
                }
            }
        });
    return result;
}

std::vector<Pixel> pixelSearchColors(int x, int y, int width, int height, std::initializer_list<Color> colors, float similarity,
                                     bool absoluteCoords) {
    return pixelSearchColors(x, y, width, height, std::vector<Color>(colors), similarity, absoluteCoords);
}

} /* namespace pymeow */
