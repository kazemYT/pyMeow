/*
 * pyMeow C++ -- minimal PNG writer implementation
 * https://github.com/qb-0/pyMeow
 */
#include "pymeow/internal/png.hpp"

#include <cstdio>
#include <fstream>

namespace pymeow {
namespace detail {

namespace {

std::uint32_t gCrcTable[256];
bool gCrcTableReady = false;

void prepareCrcTable() {
    if (gCrcTableReady) return;
    for (std::uint32_t n = 0; n < 256; ++n) {
        std::uint32_t c = n;
        for (int k = 0; k < 8; ++k) c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        gCrcTable[n] = c;
    }
    gCrcTableReady = true;
}

void pushByte(std::vector<std::uint8_t>& out, std::uint8_t value) { out.push_back(value); }

void pushBe32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>(value & 0xFF));
}

void pushChunk(std::vector<std::uint8_t>& out, const char type[4], const std::vector<std::uint8_t>& data) {
    pushBe32(out, static_cast<std::uint32_t>(data.size()));
    std::vector<std::uint8_t> crcInput;
    crcInput.reserve(4 + data.size());
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<std::uint8_t>(type[i]));
        crcInput.push_back(static_cast<std::uint8_t>(type[i]));
    }
    out.insert(out.end(), data.begin(), data.end());
    crcInput.insert(crcInput.end(), data.begin(), data.end());
    pushBe32(out, crc32(crcInput.data(), crcInput.size()));
}

} /* namespace */

std::uint32_t crc32(const std::uint8_t* data, std::size_t size) {
    prepareCrcTable();
    std::uint32_t crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < size; ++i) crc = gCrcTable[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

std::uint32_t adler32(const std::uint8_t* data, std::size_t size) {
    std::uint32_t a = 1;
    std::uint32_t b = 0;
    for (std::size_t i = 0; i < size; ++i) {
        a = (a + data[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

std::vector<std::uint8_t> encodePng(int width, int height, const std::uint8_t* rgba) {
    std::vector<std::uint8_t> out;
    if (width <= 0 || height <= 0 || rgba == nullptr) return out;

    /* Raw scanlines: one filter byte (0 == None) followed by RGBA pixels. */
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 4u;
    std::vector<std::uint8_t> raw;
    raw.reserve((rowBytes + 1) * static_cast<std::size_t>(height));
    for (int y = 0; y < height; ++y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgba + rowBytes * static_cast<std::size_t>(y), rgba + rowBytes * (static_cast<std::size_t>(y) + 1));
    }

    /* zlib stream with uncompressed (stored) deflate blocks. */
    std::vector<std::uint8_t> zlib;
    zlib.reserve(raw.size() + raw.size() / 65535 * 5 + 16);
    zlib.push_back(0x78); /* CMF: deflate, 32K window */
    zlib.push_back(0x01); /* FLG: no dict, check bits valid */
    std::size_t offset = 0;
    while (offset < raw.size()) {
        const std::size_t remaining = raw.size() - offset;
        const std::size_t blockSize = remaining > 65535 ? 65535 : remaining;
        offset += blockSize;
        zlib.push_back(offset >= raw.size() ? 0x01 : 0x00); /* BFINAL + BTYPE=00 */
        zlib.push_back(static_cast<std::uint8_t>(blockSize & 0xFF));
        zlib.push_back(static_cast<std::uint8_t>((blockSize >> 8) & 0xFF));
        zlib.push_back(static_cast<std::uint8_t>(~blockSize & 0xFF));
        zlib.push_back(static_cast<std::uint8_t>((~blockSize >> 8) & 0xFF));
        zlib.insert(zlib.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset - blockSize),
                    raw.begin() + static_cast<std::ptrdiff_t>(offset));
    }
    pushBe32(zlib, adler32(raw.data(), raw.size()));

    /* PNG signature */
    static const std::uint8_t signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    out.insert(out.end(), signature, signature + 8);

    std::vector<std::uint8_t> ihdr;
    pushBe32(ihdr, static_cast<std::uint32_t>(width));
    pushBe32(ihdr, static_cast<std::uint32_t>(height));
    ihdr.push_back(8);  /* bit depth */
    ihdr.push_back(6);  /* color type: RGBA */
    ihdr.push_back(0);  /* compression */
    ihdr.push_back(0);  /* filter */
    ihdr.push_back(0);  /* interlace */
    pushChunk(out, "IHDR", ihdr);
    pushChunk(out, "IDAT", zlib);
    pushChunk(out, "IEND", std::vector<std::uint8_t>());

    (void)pushByte;
    return out;
}

bool writeFile(const std::string& fileName, const std::vector<std::uint8_t>& data) {
    if (fileName.empty() || data.empty()) return false;
    std::ofstream file(fileName.c_str(), std::ios::binary | std::ios::trunc);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    file.flush();
    return file.good();
}

bool writePng(const std::string& fileName, int width, int height, const std::uint8_t* rgba) {
    const std::vector<std::uint8_t> png = encodePng(width, height, rgba);
    return writeFile(fileName, png);
}

} /* namespace detail */
} /* namespace pymeow */
