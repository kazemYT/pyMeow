/*
 * pyMeow C++ -- internal string helpers (UTF-8 / UTF-16, hex, trimming)
 * https://github.com/qb-0/pyMeow
 *
 * Internal header: not part of the public API.
 */
#ifndef PYMEOW_INTERNAL_STRINGS_HPP
#define PYMEOW_INTERNAL_STRINGS_HPP

#include "../common.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

namespace pymeow {
namespace detail {

inline std::string trim(const std::string& s) {
    const char* ws = " \t\r\n\v\f";
    const std::size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return std::string();
    const std::size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

inline bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

inline std::string toHex(Address value, int minWidth = 1) {
    static const char* digits = "0123456789ABCDEF";
    char buf[32];
    int i = static_cast<int>(sizeof(buf));
    buf[--i] = '\0';
    do {
        buf[--i] = digits[value & 0xF];
        value >>= 4;
    } while (value != 0 && i > 0);
    while (minWidth > 0 && (static_cast<int>(sizeof(buf)) - 1 - i) < minWidth && i > 0) {
        buf[--i] = '0';
    }
    return std::string(&buf[i]);
}

inline std::string bytesToHexString(const void* data, std::size_t size) {
    static const char* digits = "0123456789ABCDEF";
    const std::uint8_t* p = static_cast<const std::uint8_t*>(data);
    std::string out;
    out.reserve(size * 3);
    for (std::size_t i = 0; i < size; ++i) {
        if (i != 0) out += ' ';
        out += digits[(p[i] >> 4) & 0xF];
        out += digits[p[i] & 0xF];
    }
    return out;
}

/* ------------------------------------------------------- UTF-16LE <-> UTF-8 */
/* Windows strings (window titles, module paths) are UTF-16. The Python module
 * converted them with Nim's `convert("utf-8", "utf-16")`, this is the native
 * replacement. */
inline std::string utf16ToUtf8(const std::uint16_t* src, std::size_t count) {
    std::string out;
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::uint32_t cp = src[i];
        /* surrogate pair */
        if (cp >= 0xD800u && cp <= 0xDBFFu && i + 1 < count) {
            const std::uint32_t lo = src[i + 1];
            if (lo >= 0xDC00u && lo <= 0xDFFFu) {
                cp = 0x10000u + ((cp - 0xD800u) << 10) + (lo - 0xDC00u);
                ++i;
            }
        }
        if (cp < 0x80u) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800u) {
            out += static_cast<char>(0xC0u | (cp >> 6));
            out += static_cast<char>(0x80u | (cp & 0x3Fu));
        } else if (cp < 0x10000u) {
            out += static_cast<char>(0xE0u | (cp >> 12));
            out += static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
            out += static_cast<char>(0x80u | (cp & 0x3Fu));
        } else {
            out += static_cast<char>(0xF0u | (cp >> 18));
            out += static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu));
            out += static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
            out += static_cast<char>(0x80u | (cp & 0x3Fu));
        }
    }
    return out;
}

inline std::string utf16ToUtf8(const std::uint16_t* src) {
    std::size_t count = 0;
    while (src[count] != 0) ++count;
    return utf16ToUtf8(src, count);
}

inline std::vector<std::uint16_t> utf8ToUtf16(const std::string& s) {
    std::vector<std::uint16_t> out;
    out.reserve(s.size() + 1);
    std::size_t i = 0;
    while (i < s.size()) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        std::uint32_t cp = 0;
        std::size_t extra = 0;
        if (c < 0x80u) { cp = c; extra = 0; }
        else if ((c & 0xE0u) == 0xC0u) { cp = c & 0x1Fu; extra = 1; }
        else if ((c & 0xF0u) == 0xE0u) { cp = c & 0x0Fu; extra = 2; }
        else if ((c & 0xF8u) == 0xF0u) { cp = c & 0x07u; extra = 3; }
        else { ++i; continue; } /* invalid lead byte: skip */

        if (extra > 0 && i + extra >= s.size()) break; /* truncated sequence */

        bool ok = true;
        for (std::size_t k = 1; k <= extra; ++k) {
            const unsigned char cc = static_cast<unsigned char>(s[i + k]);
            if ((cc & 0xC0u) != 0x80u) { ok = false; break; }
            cp = (cp << 6) | (cc & 0x3Fu);
        }
        i += extra + 1;
        if (!ok) continue;

        if (cp < 0x10000u) {
            out.push_back(static_cast<std::uint16_t>(cp));
        } else {
            cp -= 0x10000u;
            out.push_back(static_cast<std::uint16_t>(0xD800u + (cp >> 10)));
            out.push_back(static_cast<std::uint16_t>(0xDC00u + (cp & 0x3FFu)));
        }
    }
    out.push_back(0);
    return out;
}

} /* namespace detail */
} /* namespace pymeow */

#endif /* PYMEOW_INTERNAL_STRINGS_HPP */
