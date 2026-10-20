/*
 * pyMeow C++ -- typed memory operations (port of memop.nim)
 * https://github.com/qb-0/pyMeow
 */
#include "pymeow/memop.hpp"

#include "pymeow/internal/strings.hpp"

#include <algorithm>
#include <cstring>

namespace pymeow {

/* ------------------------------------------------------------------- reads */
std::int8_t readInt8(const Process& process, Address address) { return read<std::int8_t>(process, address); }
std::int16_t readInt16(const Process& process, Address address) { return read<std::int16_t>(process, address); }
std::int32_t readInt(const Process& process, Address address) { return read<std::int32_t>(process, address); }
std::int64_t readInt64(const Process& process, Address address) { return read<std::int64_t>(process, address); }
std::uint8_t readByte(const Process& process, Address address) { return read<std::uint8_t>(process, address); }
std::uint16_t readUInt16(const Process& process, Address address) { return read<std::uint16_t>(process, address); }
std::uint32_t readUInt(const Process& process, Address address) { return read<std::uint32_t>(process, address); }
std::uint64_t readUInt64(const Process& process, Address address) { return read<std::uint64_t>(process, address); }
float readFloat(const Process& process, Address address) { return read<float>(process, address); }
double readFloat64(const Process& process, Address address) { return read<double>(process, address); }
bool readBool(const Process& process, Address address) { return read<std::uint8_t>(process, address) != 0; }
Vec2 readVec2(const Process& process, Address address) { return read<Vec2>(process, address); }
Vec3 readVec3(const Process& process, Address address) { return read<Vec3>(process, address); }

std::vector<std::int8_t> readInts8(const Process& process, Address address, std::size_t count) {
    return readSeq<std::int8_t>(process, address, count);
}
std::vector<std::int16_t> readInts16(const Process& process, Address address, std::size_t count) {
    return readSeq<std::int16_t>(process, address, count);
}
std::vector<std::int32_t> readInts(const Process& process, Address address, std::size_t count) {
    return readSeq<std::int32_t>(process, address, count);
}
std::vector<std::int64_t> readInts64(const Process& process, Address address, std::size_t count) {
    return readSeq<std::int64_t>(process, address, count);
}
std::vector<std::uint8_t> readBytes(const Process& process, Address address, std::size_t count) {
    return readSeq<std::uint8_t>(process, address, count);
}
std::vector<std::uint16_t> readUInts16(const Process& process, Address address, std::size_t count) {
    return readSeq<std::uint16_t>(process, address, count);
}
std::vector<std::uint32_t> readUInts(const Process& process, Address address, std::size_t count) {
    return readSeq<std::uint32_t>(process, address, count);
}
std::vector<std::uint64_t> readUInts64(const Process& process, Address address, std::size_t count) {
    return readSeq<std::uint64_t>(process, address, count);
}
std::vector<float> readFloats(const Process& process, Address address, std::size_t count) {
    return readSeq<float>(process, address, count);
}
std::vector<double> readFloats64(const Process& process, Address address, std::size_t count) {
    return readSeq<double>(process, address, count);
}

std::string readString(const Process& process, Address address, std::size_t size) {
    if (size == 0) return std::string();
    const std::vector<char> buffer = readSeq<char>(process, address, size);
    const char* data = buffer.data();
    std::size_t length = 0;
    while (length < buffer.size() && data[length] != '\0') ++length;
    return std::string(data, length);
}

std::string readWString(const Process& process, Address address, std::size_t maxChars) {
    if (maxChars == 0) return std::string();
    const std::vector<std::uint16_t> buffer = readSeq<std::uint16_t>(process, address, maxChars);
    std::size_t length = 0;
    while (length < buffer.size() && buffer[length] != 0) ++length;
    return detail::utf16ToUtf8(buffer.data(), length);
}

std::string bytesToString(const Process& process, Address address, std::size_t size) {
    if (size < 2) return std::string();
    const std::vector<std::uint8_t> buffer = readSeq<std::uint8_t>(process, address, size);
    std::size_t length = 0;
    while (length + 1 < buffer.size()) {
        const std::uint16_t unit = static_cast<std::uint16_t>(buffer[length]) |
                                   static_cast<std::uint16_t>(static_cast<std::uint16_t>(buffer[length + 1]) << 8);
        if (unit == 0) break;
        length += 2;
    }
    std::string result = detail::utf16ToUtf8(reinterpret_cast<const std::uint16_t*>(buffer.data()), length / 2);
    return detail::trim(result);
}

/* ---------------------------------------------------------- pointer chains */
namespace {
void requireOffsets(std::size_t count) {
    if (count == 0) throw InvalidArgumentError("pointerChain: at least one offset is required");
}
} /* namespace */

std::uint32_t pointerChain32(const Process& process, Address base, const Address* offsets, std::size_t count) {
    requireOffsets(count);
    std::uint32_t value = read<std::uint32_t>(process, base);
    for (std::size_t i = 0; i + 1 < count; ++i) value = read<std::uint32_t>(process, static_cast<Address>(value) + offsets[i]);
    return static_cast<std::uint32_t>(static_cast<Address>(value) + offsets[count - 1]);
}

std::uint64_t pointerChain64(const Process& process, Address base, const Address* offsets, std::size_t count) {
    requireOffsets(count);
    std::uint64_t value = read<std::uint64_t>(process, base);
    for (std::size_t i = 0; i + 1 < count; ++i) value = read<std::uint64_t>(process, static_cast<Address>(value) + offsets[i]);
    return static_cast<std::uint64_t>(value + static_cast<std::uint64_t>(offsets[count - 1]));
}

std::uint32_t pointerChain32(const Process& process, Address base, std::initializer_list<Address> offsets) {
    return pointerChain32(process, base, offsets.begin(), offsets.size());
}
std::uint64_t pointerChain64(const Process& process, Address base, std::initializer_list<Address> offsets) {
    return pointerChain64(process, base, offsets.begin(), offsets.size());
}
std::uint32_t pointerChain32(const Process& process, Address base, const std::vector<Address>& offsets) {
    return pointerChain32(process, base, offsets.empty() ? nullptr : offsets.data(), offsets.size());
}
std::uint64_t pointerChain64(const Process& process, Address base, const std::vector<Address>& offsets) {
    return pointerChain64(process, base, offsets.empty() ? nullptr : offsets.data(), offsets.size());
}

/* ------------------------------------------------------------------ writes */
void writeInt8(const Process& process, Address address, std::int8_t data) { write<std::int8_t>(process, address, data); }
void writeInt16(const Process& process, Address address, std::int16_t data) { write<std::int16_t>(process, address, data); }
void writeInt(const Process& process, Address address, std::int32_t data) { write<std::int32_t>(process, address, data); }
void writeInt64(const Process& process, Address address, std::int64_t data) { write<std::int64_t>(process, address, data); }
void writeByte(const Process& process, Address address, std::uint8_t data) { write<std::uint8_t>(process, address, data); }
void writeUInt16(const Process& process, Address address, std::uint16_t data) { write<std::uint16_t>(process, address, data); }
void writeUInt(const Process& process, Address address, std::uint32_t data) { write<std::uint32_t>(process, address, data); }
void writeUInt64(const Process& process, Address address, std::uint64_t data) { write<std::uint64_t>(process, address, data); }
void writeFloat(const Process& process, Address address, float data) { write<float>(process, address, data); }
void writeFloat64(const Process& process, Address address, double data) { write<double>(process, address, data); }
void writeBool(const Process& process, Address address, bool data) { write<std::uint8_t>(process, address, data ? 1 : 0); }
void writeVec2(const Process& process, Address address, Vec2 data) { write<Vec2>(process, address, data); }
void writeVec3(const Process& process, Address address, Vec3 data) { write<Vec3>(process, address, data); }

#define PYMEOW_WRITE_ARRAY(type, name)                                                                                     \
    void name(const Process& process, Address address, const std::vector<type>& data) {                                    \
        writeArray<type>(process, address, data);                                                                          \
    }                                                                                                                      \
    void name(const Process& process, Address address, std::initializer_list<type> data) {                                 \
        writeArray<type>(process, address, data.begin(), data.size());                                                     \
    }

PYMEOW_WRITE_ARRAY(std::int8_t, writeInts8)
PYMEOW_WRITE_ARRAY(std::int16_t, writeInts16)
PYMEOW_WRITE_ARRAY(std::int32_t, writeInts)
PYMEOW_WRITE_ARRAY(std::int64_t, writeInts64)
PYMEOW_WRITE_ARRAY(std::uint8_t, writeBytes)
PYMEOW_WRITE_ARRAY(std::uint16_t, writeUInts16)
PYMEOW_WRITE_ARRAY(std::uint32_t, writeUInts)
PYMEOW_WRITE_ARRAY(std::uint64_t, writeUInts64)
PYMEOW_WRITE_ARRAY(float, writeFloats)
PYMEOW_WRITE_ARRAY(double, writeFloats64)

#undef PYMEOW_WRITE_ARRAY

void writeString(const Process& process, Address address, const std::string& data) {
    writeArray<char>(process, address, data.data(), data.size());
}

void writeStringZ(const Process& process, Address address, const std::string& data) {
    writeArray<char>(process, address, data.c_str(), data.size() + 1);
}

void writeWString(const Process& process, Address address, const std::string& utf8) {
    const std::vector<std::uint16_t> buffer = detail::utf8ToUtf16(utf8); /* includes the trailing 0 */
    writeArray<std::uint16_t>(process, address, buffer);
}

/* ------------------------------------------------- dynamic (Python r() / w()) */
namespace {
struct TypeEntry {
    const char* name;
    DataType type;
};
const TypeEntry kTypeNames[] = {
    {"int8", DataType::Int8},     {"int16", DataType::Int16},   {"int", DataType::Int},      {"int32", DataType::Int},
    {"int64", DataType::Int64},   {"byte", DataType::Byte},     {"uint8", DataType::Byte},   {"uint16", DataType::UInt16},
    {"uint", DataType::UInt},     {"uint32", DataType::UInt},   {"uint64", DataType::UInt64}, {"float", DataType::Float},
    {"float32", DataType::Float}, {"float64", DataType::Float64}, {"double", DataType::Float64}, {"bool", DataType::Bool},
    {"vec2", DataType::Vec2},     {"vec3", DataType::Vec3},     {"str", DataType::String},   {"string", DataType::String},
};
} /* namespace */

DataType dataTypeFromString(const std::string& name) {
    const std::string lower = detail::toLower(detail::trim(name));
    for (const TypeEntry& entry : kTypeNames) {
        if (lower == entry.name) return entry.type;
    }
    throw InvalidArgumentError("Unknown data type: " + name);
}

const char* dataTypeToString(DataType type) {
    switch (type) {
        case DataType::Int8: return "int8";
        case DataType::Int16: return "int16";
        case DataType::Int: return "int";
        case DataType::Int64: return "int64";
        case DataType::Byte: return "byte";
        case DataType::UInt16: return "uint16";
        case DataType::UInt: return "uint";
        case DataType::UInt64: return "uint64";
        case DataType::Float: return "float";
        case DataType::Float64: return "float64";
        case DataType::Bool: return "bool";
        case DataType::Vec2: return "vec2";
        case DataType::Vec3: return "vec3";
        case DataType::String: return "str";
    }
    return "unknown";
}

std::size_t dataTypeSize(DataType type) {
    switch (type) {
        case DataType::Int8:
        case DataType::Byte:
        case DataType::Bool: return 1;
        case DataType::Int16:
        case DataType::UInt16: return 2;
        case DataType::Int:
        case DataType::UInt:
        case DataType::Float: return 4;
        case DataType::Int64:
        case DataType::UInt64:
        case DataType::Float64:
        case DataType::Vec2: return 8;
        case DataType::Vec3: return 12;
        case DataType::String: return 1;
    }
    return 0;
}

Value readValue(const Process& process, Address address, DataType type) {
    switch (type) {
        case DataType::Int8: return readInt8(process, address);
        case DataType::Int16: return readInt16(process, address);
        case DataType::Int: return readInt(process, address);
        case DataType::Int64: return readInt64(process, address);
        case DataType::Byte: return readByte(process, address);
        case DataType::UInt16: return readUInt16(process, address);
        case DataType::UInt: return readUInt(process, address);
        case DataType::UInt64: return readUInt64(process, address);
        case DataType::Float: return readFloat(process, address);
        case DataType::Float64: return readFloat64(process, address);
        case DataType::Bool: return readBool(process, address);
        case DataType::Vec2: return readVec2(process, address);
        case DataType::Vec3: return readVec3(process, address);
        case DataType::String: return readString(process, address);
    }
    throw InvalidArgumentError(std::string("Unknown data type: ") + dataTypeToString(type));
}

std::vector<Value> readValues(const Process& process, Address address, DataType type, std::size_t count) {
    std::vector<Value> result;
    result.reserve(count);
    const std::size_t elementSize = dataTypeSize(type);
    for (std::size_t i = 0; i < count; ++i) {
        if (type == DataType::String)
            result.push_back(readString(process, address + i * elementSize));
        else
            result.push_back(readValue(process, address + i * elementSize, type));
    }
    return result;
}

void writeValue(const Process& process, Address address, const Value& data) {
    switch (data.index()) {
        case 0: writeInt8(process, address, std::get<std::int8_t>(data)); return;
        case 1: writeInt16(process, address, std::get<std::int16_t>(data)); return;
        case 2: writeInt(process, address, std::get<std::int32_t>(data)); return;
        case 3: writeInt64(process, address, std::get<std::int64_t>(data)); return;
        case 4: writeByte(process, address, std::get<std::uint8_t>(data)); return;
        case 5: writeUInt16(process, address, std::get<std::uint16_t>(data)); return;
        case 6: writeUInt(process, address, std::get<std::uint32_t>(data)); return;
        case 7: writeUInt64(process, address, std::get<std::uint64_t>(data)); return;
        case 8: writeFloat(process, address, std::get<float>(data)); return;
        case 9: writeFloat64(process, address, std::get<double>(data)); return;
        case 10: writeBool(process, address, std::get<bool>(data)); return;
        case 11: writeVec2(process, address, std::get<Vec2>(data)); return;
        case 12: writeVec3(process, address, std::get<Vec3>(data)); return;
        case 13: writeString(process, address, std::get<std::string>(data)); return;
        default: throw InvalidArgumentError("writeValue: unsupported value");
    }
}

void writeValues(const Process& process, Address address, DataType type, const std::vector<Value>& data) {
    if (data.empty()) return;
    const std::size_t elementSize = dataTypeSize(type);
    for (std::size_t i = 0; i < data.size(); ++i) writeValue(process, address + i * elementSize, data[i]);
}

} /* namespace pymeow */
