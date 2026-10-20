/*
 * pyMeow C++ -- pixel scanning (port of pixel.nim)
 * https://github.com/qb-0/pyMeow
 *
 * Captures pixels from the screen: GDI (BitBlt) on Windows, XGetImage on Linux.
 * Saving a capture as PNG is implemented natively (no raylib needed).
 *
 *   pm::Pixel p = pm::pixelAtMouse();
 *   auto hits = pm::pixelSearchColors(0, 0, 100, 100, {pm::colors::red}, 90.0f);
 */
#ifndef PYMEOW_PIXEL_HPP
#define PYMEOW_PIXEL_HPP

#include "common.hpp"
#include "types.hpp"

#include <functional>
#include <string>
#include <vector>

namespace pymeow {

/* Streams every pixel of a region into `fn` (much faster than materialising a
 * vector for full screen scans). */
PYMEOW_API void forEachPixelInRegion(int x, int y, int width, int height, const std::function<void(const Pixel&)>& fn);

/* Captures a region. The Pixel coordinates are relative to the region origin
 * (like the Python iterator); pass absoluteCoords = true to get screen
 * coordinates instead. */
PYMEOW_API std::vector<Pixel> pixelEnumRegion(int x, int y, int width, int height, bool absoluteCoords = false);
PYMEOW_API std::vector<Pixel> pixelEnumScreen(bool absoluteCoords = false);
PYMEOW_API Pixel pixelAtMouse();
PYMEOW_API Pixel getPixel(int x, int y);

/* Writes the region as PNG. ".png" is appended when missing.
 * Returns false when the file could not be written. */
PYMEOW_API bool pixelSaveToFile(int x, int y, int width, int height, const std::string& fileName);
/* Saves a captured region (see encodePng in internal/png.hpp for the encoder). */
PYMEOW_API bool savePixelsAsPng(const std::vector<Pixel>& pixels, int width, int height, const std::string& fileName);

/* Every pixel of the region whose color matches one of `colors` with at least
 * `similarity` percent (compareColorPct). */
PYMEOW_API std::vector<Pixel> pixelSearchColors(int x, int y, int width, int height, const std::vector<Color>& colors,
                                                float similarity, bool absoluteCoords = false);
PYMEOW_API std::vector<Pixel> pixelSearchColors(int x, int y, int width, int height, std::initializer_list<Color> colors,
                                                float similarity, bool absoluteCoords = false);

} /* namespace pymeow */

#endif /* PYMEOW_PIXEL_HPP */
