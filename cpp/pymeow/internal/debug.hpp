/*
 * pyMeow C++ -- internal debug logging (Process.debug)
 * https://github.com/qb-0/pyMeow
 *
 * Internal header: not part of the public API.
 * When a Process is opened with debug = true every read / write is printed to
 * stdout, exactly like the Python module does:
 *   [R] [int32] 0x7FF6A1B2C3D4 -> 1337
 *   [W] [float32] 0x7FF6A1B2C3D4 -> 3.500000
 */
#ifndef PYMEOW_INTERNAL_DEBUG_HPP
#define PYMEOW_INTERNAL_DEBUG_HPP

#include "../common.hpp"
#include "../types.hpp"
#include "strings.hpp"

#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>

namespace pymeow {
namespace detail {

PYMEOW_API void logMemoryAccess(char tag, const std::string& typeName, Address address, const std::string& value);

template <class T>
inline std::string typeLabel() {
    if constexpr (std::is_same<T, bool>::value) return "bool";
    else if constexpr (std::is_same<T, std::int8_t>::value) return "int8";
    else if constexpr (std::is_same<T, std::uint8_t>::value) return "byte";
    else if constexpr (std::is_same<T, std::int16_t>::value) return "int16";
    else if constexpr (std::is_same<T, std::uint16_t>::value) return "uint16";
    else if constexpr (std::is_same<T, std::int32_t>::value) return "int32";
    else if constexpr (std::is_same<T, std::uint32_t>::value) return "uint32";
    else if constexpr (std::is_same<T, std::int64_t>::value) return "int64";
    else if constexpr (std::is_same<T, std::uint64_t>::value) return "uint64";
    else if constexpr (std::is_same<T, float>::value) return "float32";
    else if constexpr (std::is_same<T, double>::value) return "float64";
    else if constexpr (std::is_same<T, Vec2>::value) return "vec2";
    else if constexpr (std::is_same<T, Vec3>::value) return "vec3";
    else if constexpr (std::is_same<T, Color>::value) return "color";
    else return "bytes";
}

template <class T>
inline std::string valueText(const T& value) {
    if constexpr (std::is_same<T, bool>::value) {
        return value ? "true" : "false";
    } else if constexpr (std::is_same<T, float>::value || std::is_same<T, double>::value) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%f", static_cast<double>(value));
        return buf;
    } else if constexpr (std::is_arithmetic<T>::value) {
        return std::to_string(static_cast<long long>(value));
    } else if constexpr (std::is_same<T, Vec2>::value) {
        return "(" + std::to_string(value.x) + ", " + std::to_string(value.y) + ")";
    } else if constexpr (std::is_same<T, Vec3>::value) {
        return "(" + std::to_string(value.x) + ", " + std::to_string(value.y) + ", " + std::to_string(value.z) + ")";
    } else if constexpr (std::is_same<T, Color>::value) {
        return "(" + std::to_string(static_cast<int>(value.r)) + ", " + std::to_string(static_cast<int>(value.g)) + ", " +
               std::to_string(static_cast<int>(value.b)) + ", " + std::to_string(static_cast<int>(value.a)) + ")";
    } else {
        return bytesToHexString(&value, sizeof(T));
    }
}

/* Buffer logging is capped so scanning a whole module does not flood stdout. */
inline std::string bufferText(const void* data, std::size_t size) {
    const std::size_t cap = 64;
    std::string out = bytesToHexString(data, size < cap ? size : cap);
    if (size > cap) out += " ...";
    return "[" + std::to_string(size) + " bytes] " + out;
}

template <class T>
inline void logRead(bool debug, Address address, const T& value) {
    if (debug) logMemoryAccess('R', typeLabel<T>(), address, valueText<T>(value));
}

template <class T>
inline void logWrite(bool debug, Address address, const T& value) {
    if (debug) logMemoryAccess('W', typeLabel<T>(), address, valueText<T>(value));
}

template <class T>
inline void logReadSeq(bool debug, Address address, const std::vector<T>& value) {
    if (debug)
        logMemoryAccess('R', std::string(typeLabel<T>()) + "[]", address,
                        bufferText(value.empty() ? nullptr : static_cast<const void*>(value.data()), value.size() * sizeof(T)));
}

template <class T>
inline void logWriteSeq(bool debug, Address address, const T* data, std::size_t count) {
    if (debug)
        logMemoryAccess('W', std::string(typeLabel<T>()) + "[]", address,
                        bufferText(static_cast<const void*>(data), count * sizeof(T)));
}

} /* namespace detail */
} /* namespace pymeow */

#endif /* PYMEOW_INTERNAL_DEBUG_HPP */
