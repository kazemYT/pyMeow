/*
 * pyMeow C++ -- typed memory operations (port of memop.nim)
 * https://github.com/qb-0/pyMeow
 *
 * Typed wrappers around the read<T> / write<T> templates of memcore.hpp plus
 * the pointer chain helpers. The names follow the Python module:
 *
 *   Python                 C++
 *   ------------------------------------------------------------------
 *   pm.r_int(p, a)         pm::readInt(p, a)
 *   pm.r_floats(p, a, 16)  pm::readFloats(p, a, 16)
 *   pm.r_string(p, a)      pm::readString(p, a)
 *   pm.w_uint(p, a, 5)     pm::writeUInt(p, a, 5)
 *   pm.w_bytes(p, a, [..]) pm::writeBytes(p, a, {..})
 *   pm.pointer_chain_32    pm::pointerChain32
 *   pm.r(p, a, "int")      pm::readValue(p, a, pm::DataType::Int)
 *   pm.w(p, a, 5, "int")   pm::writeValue(p, a, 5)
 */
#ifndef PYMEOW_MEMOP_HPP
#define PYMEOW_MEMOP_HPP

#include "memcore.hpp"
#include "types.hpp"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <variant>
#include <vector>

namespace pymeow {

/* ------------------------------------------------------------------- reads */
PYMEOW_API std::int8_t readInt8(const Process& process, Address address);
PYMEOW_API std::int16_t readInt16(const Process& process, Address address);
PYMEOW_API std::int32_t readInt(const Process& process, Address address);
PYMEOW_API std::int64_t readInt64(const Process& process, Address address);
PYMEOW_API std::uint8_t readByte(const Process& process, Address address);
PYMEOW_API std::uint16_t readUInt16(const Process& process, Address address);
PYMEOW_API std::uint32_t readUInt(const Process& process, Address address);
PYMEOW_API std::uint64_t readUInt64(const Process& process, Address address);
PYMEOW_API float readFloat(const Process& process, Address address);
PYMEOW_API double readFloat64(const Process& process, Address address);
PYMEOW_API bool readBool(const Process& process, Address address);
PYMEOW_API Vec2 readVec2(const Process& process, Address address);
PYMEOW_API Vec3 readVec3(const Process& process, Address address);

PYMEOW_API std::vector<std::int8_t> readInts8(const Process& process, Address address, std::size_t count);
PYMEOW_API std::vector<std::int16_t> readInts16(const Process& process, Address address, std::size_t count);
PYMEOW_API std::vector<std::int32_t> readInts(const Process& process, Address address, std::size_t count);
PYMEOW_API std::vector<std::int64_t> readInts64(const Process& process, Address address, std::size_t count);
PYMEOW_API std::vector<std::uint8_t> readBytes(const Process& process, Address address, std::size_t count);
PYMEOW_API std::vector<std::uint16_t> readUInts16(const Process& process, Address address, std::size_t count);
PYMEOW_API std::vector<std::uint32_t> readUInts(const Process& process, Address address, std::size_t count);
PYMEOW_API std::vector<std::uint64_t> readUInts64(const Process& process, Address address, std::size_t count);
PYMEOW_API std::vector<float> readFloats(const Process& process, Address address, std::size_t count);
PYMEOW_API std::vector<double> readFloats64(const Process& process, Address address, std::size_t count);

/* Reads up to `size` bytes and cuts at the first NUL (ASCII / UTF-8). */
PYMEOW_API std::string readString(const Process& process, Address address, std::size_t size = 30);
/* Reads a UTF-16LE string of up to `maxChars` characters (typical for Windows
 * games) and converts it to UTF-8. */
PYMEOW_API std::string readWString(const Process& process, Address address, std::size_t maxChars = 30);
/* Port of bytes_to_string(): reads `size` raw bytes, decodes them as UTF-16LE
 * and returns trimmed UTF-8. */
PYMEOW_API std::string bytesToString(const Process& process, Address address, std::size_t size);

/* ---------------------------------------------------------- pointer chains */
/* pointerChain32(p, base, {0x10, 0x24, 0x0}) resolves
 * [[base] + 0x10] + 0x24 + 0x0 using 32 bit pointers. */
PYMEOW_API std::uint32_t pointerChain32(const Process& process, Address base, std::initializer_list<Address> offsets);
PYMEOW_API std::uint64_t pointerChain64(const Process& process, Address base, std::initializer_list<Address> offsets);
PYMEOW_API std::uint32_t pointerChain32(const Process& process, Address base, const Address* offsets, std::size_t count);
PYMEOW_API std::uint64_t pointerChain64(const Process& process, Address base, const Address* offsets, std::size_t count);
PYMEOW_API std::uint32_t pointerChain32(const Process& process, Address base, const std::vector<Address>& offsets);
PYMEOW_API std::uint64_t pointerChain64(const Process& process, Address base, const std::vector<Address>& offsets);

/* ------------------------------------------------------------------ writes */
PYMEOW_API void writeInt8(const Process& process, Address address, std::int8_t data);
PYMEOW_API void writeInt16(const Process& process, Address address, std::int16_t data);
PYMEOW_API void writeInt(const Process& process, Address address, std::int32_t data);
PYMEOW_API void writeInt64(const Process& process, Address address, std::int64_t data);
PYMEOW_API void writeByte(const Process& process, Address address, std::uint8_t data);
PYMEOW_API void writeUInt16(const Process& process, Address address, std::uint16_t data);
PYMEOW_API void writeUInt(const Process& process, Address address, std::uint32_t data);
PYMEOW_API void writeUInt64(const Process& process, Address address, std::uint64_t data);
PYMEOW_API void writeFloat(const Process& process, Address address, float data);
PYMEOW_API void writeFloat64(const Process& process, Address address, double data);
PYMEOW_API void writeBool(const Process& process, Address address, bool data);
PYMEOW_API void writeVec2(const Process& process, Address address, Vec2 data);
PYMEOW_API void writeVec3(const Process& process, Address address, Vec3 data);

PYMEOW_API void writeInts8(const Process& process, Address address, const std::vector<std::int8_t>& data);
PYMEOW_API void writeInts16(const Process& process, Address address, const std::vector<std::int16_t>& data);
PYMEOW_API void writeInts(const Process& process, Address address, const std::vector<std::int32_t>& data);
PYMEOW_API void writeInts64(const Process& process, Address address, const std::vector<std::int64_t>& data);
PYMEOW_API void writeBytes(const Process& process, Address address, const std::vector<std::uint8_t>& data);
PYMEOW_API void writeUInts16(const Process& process, Address address, const std::vector<std::uint16_t>& data);
PYMEOW_API void writeUInts(const Process& process, Address address, const std::vector<std::uint32_t>& data);
PYMEOW_API void writeUInts64(const Process& process, Address address, const std::vector<std::uint64_t>& data);
PYMEOW_API void writeFloats(const Process& process, Address address, const std::vector<float>& data);
PYMEOW_API void writeFloats64(const Process& process, Address address, const std::vector<double>& data);

/* Initializer list overloads: pm::writeBytes(p, addr, {0x90, 0x90}); */
PYMEOW_API void writeInts8(const Process& process, Address address, std::initializer_list<std::int8_t> data);
PYMEOW_API void writeInts16(const Process& process, Address address, std::initializer_list<std::int16_t> data);
PYMEOW_API void writeInts(const Process& process, Address address, std::initializer_list<std::int32_t> data);
PYMEOW_API void writeInts64(const Process& process, Address address, std::initializer_list<std::int64_t> data);
PYMEOW_API void writeBytes(const Process& process, Address address, std::initializer_list<std::uint8_t> data);
PYMEOW_API void writeUInts16(const Process& process, Address address, std::initializer_list<std::uint16_t> data);
PYMEOW_API void writeUInts(const Process& process, Address address, std::initializer_list<std::uint32_t> data);
PYMEOW_API void writeUInts64(const Process& process, Address address, std::initializer_list<std::uint64_t> data);
PYMEOW_API void writeFloats(const Process& process, Address address, std::initializer_list<float> data);
PYMEOW_API void writeFloats64(const Process& process, Address address, std::initializer_list<double> data);

/* Writes the characters of the string without the terminating NUL
 * (identical to the Python w_string). Use writeStringZ() to include it. */
PYMEOW_API void writeString(const Process& process, Address address, const std::string& data);
PYMEOW_API void writeStringZ(const Process& process, Address address, const std::string& data);
PYMEOW_API void writeWString(const Process& process, Address address, const std::string& utf8);

/* ------------------------------------------------- dynamic (Python r() / w()) */
enum class DataType {
    Int8, Int16, Int, Int64,
    Byte, UInt16, UInt, UInt64,
    Float, Float64, Bool, Vec2, Vec3, String
};

/* Accepts the Python type names: "int"/"int32", "int8", "int16", "int64",
 * "uint"/"uint32", "uint16", "uint64", "float", "float64", "byte", "bool",
 * "vec2", "vec3", "str"/"string". Throws InvalidArgumentError when unknown. */
PYMEOW_API DataType dataTypeFromString(const std::string& name);
PYMEOW_API const char* dataTypeToString(DataType type);
PYMEOW_API std::size_t dataTypeSize(DataType type);

using Value = std::variant<std::int8_t, std::int16_t, std::int32_t, std::int64_t,
                           std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
                           float, double, bool, Vec2, Vec3, std::string>;

PYMEOW_API Value readValue(const Process& process, Address address, DataType type);
PYMEOW_API std::vector<Value> readValues(const Process& process, Address address, DataType type, std::size_t count);
/* The written type is taken from the variant alternative. */
PYMEOW_API void writeValue(const Process& process, Address address, const Value& data);
PYMEOW_API void writeValues(const Process& process, Address address, DataType type, const std::vector<Value>& data);

} /* namespace pymeow */

#endif /* PYMEOW_MEMOP_HPP */
