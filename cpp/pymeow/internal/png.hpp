/*
 * pyMeow C++ -- internal PNG writer
 * https://github.com/qb-0/pyMeow
 *
 * Internal header: not part of the public API.
 * pixelSaveToFile() used raylib's ExportImage() in the Python module. The C++
 * port writes the PNG itself so the pixel module works without raylib.
 * The encoder emits 8 bit RGBA scanlines using zlib "stored" (uncompressed)
 * blocks: no dependency, correct output, larger files.
 */
#ifndef PYMEOW_INTERNAL_PNG_HPP
#define PYMEOW_INTERNAL_PNG_HPP

#include "../common.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace pymeow {
namespace detail {

/* Encodes width * height RGBA8 pixels into a PNG file buffer. */
PYMEOW_API std::vector<std::uint8_t> encodePng(int width, int height, const std::uint8_t* rgba);

/* Writes the buffer to disk, returns false when the file could not be written. */
PYMEOW_API bool writeFile(const std::string& fileName, const std::vector<std::uint8_t>& data);

/* Convenience: encode + write. */
PYMEOW_API bool writePng(const std::string& fileName, int width, int height, const std::uint8_t* rgba);

PYMEOW_API std::uint32_t crc32(const std::uint8_t* data, std::size_t size);
PYMEOW_API std::uint32_t adler32(const std::uint8_t* data, std::size_t size);

} /* namespace detail */
} /* namespace pymeow */

#endif /* PYMEOW_INTERNAL_PNG_HPP */
