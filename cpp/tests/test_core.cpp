/*
 * pyMeow C++ -- core test suite
 * https://github.com/qb-0/pyMeow
 *
 * Self contained: no external test framework, no graphics, no raylib.
 * Everything that does not need a window / a target process is verified against
 * the process itself (reading and writing our own memory works unprivileged
 * through process_vm_readv / ReadProcessMemory).
 *
 * Build & run (Linux):
 *   g++ -std=c++17 -Wall -Wextra -DPYMEOW_NO_GRAPHICS -I.. -o test_core
 *       test_core.cpp (plus every source in ../src) -lX11 -lXtst -lXinerama
 *   ./test_core
 *
 * Build & run (Windows, MSVC):
 *   cl /EHsc /std:c++17 /DPYMEOW_NO_GRAPHICS /I.. test_core.cpp (plus every
 *      source in ..\src) psapi.lib user32.lib gdi32.lib kernel32.lib
 *   test_core.exe
 */
#include <pymeow.hpp>
#include <pymeow/internal/png.hpp>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <unistd.h>
#endif

namespace {

int gChecks = 0;
int gFailures = 0;

void check(bool condition, const std::string& what) {
    ++gChecks;
    if (!condition) {
        ++gFailures;
        std::printf("  FAIL: %s\n", what.c_str());
    }
}

template <class A, class B>
void checkEq(const A& actual, const B& expected, const std::string& what) {
    ++gChecks;
    if (!(actual == expected)) {
        ++gFailures;
        std::printf("  FAIL: %s\n", what.c_str());
    }
}

bool nearly(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }

void checkNearly(float actual, float expected, const std::string& what, float eps = 1e-4f) {
    ++gChecks;
    if (!nearly(actual, expected, eps)) {
        ++gFailures;
        std::printf("  FAIL: %s (got %f, expected %f)\n", what.c_str(), static_cast<double>(actual),
                    static_cast<double>(expected));
    }
}

template <class Exception, class Fn>
bool throwsException(Fn fn) {
    try {
        fn();
    } catch (const Exception&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

int currentPid() {
#if defined(_WIN32)
    return static_cast<int>(::GetCurrentProcessId());
#else
    return static_cast<int>(::getpid());
#endif
}

/* ------------------------------------------------------------------ basics */
void testBasics() {
    std::printf("[basics]\n");
    check(std::strcmp(pm::version(), "2.0.0") == 0, "version() == 2.0.0");
#if defined(_WIN32)
    check(std::strcmp(pm::systemName(), "windows") == 0, "systemName() == windows");
#else
    check(std::strcmp(pm::systemName(), "linux") == 0, "systemName() == linux");
#endif
    check(pm::pidExists(currentPid()), "pidExists(own pid)");
    check(!pm::pidExists(-1), "!pidExists(-1)");
    check(!pm::processExists("definitely_not_running_xyz"), "!processExists(unknown)");
    check(!pm::processRunning(pm::Process{}), "!processRunning(default Process)");
    check(throwsException<pm::NotFoundError>([] { pm::getProcessId("definitely_not_running_xyz"); }),
          "getProcessId(unknown) throws NotFoundError");
    check(pm::getProcessName(currentPid()) != "", "getProcessName(own pid) is not empty");
    check(!pm::requireRootEnabled(), "requireRoot is disabled by default");
    pm::setRequireRoot(false);
}

/* ------------------------------------------------------------------ colors */
void testColors() {
    std::printf("[colors]\n");
    checkEq(pm::getColor("red"), pm::Color(255, 0, 0, 255), "getColor(\"red\")");
    checkEq(pm::getColor("RED"), pm::Color(255, 0, 0, 255), "getColor is case insensitive");
    checkEq(pm::getColor("lime"), pm::Color(0, 255, 0, 255), "getColor(\"lime\")");
    checkEq(pm::getColor("cornflowerblue"), pm::Color(100, 149, 237, 255), "getColor(\"cornflowerblue\")");
    checkEq(pm::getColor("#00FF00"), pm::Color(0, 255, 0, 255), "getColor(#RRGGBB) -> alpha 255");
    checkEq(pm::getColor("#00FF0080"), pm::Color(0, 255, 0, 128), "getColor(#RRGGBBAA)");
    checkEq(pm::getColor("0xFF00FF"), pm::Color(255, 0, 255, 255), "getColor(0xRRGGBB)");
    checkEq(pm::getColor("not_a_color"), pm::Color(0, 0, 0, 255), "getColor(unknown) -> black");
    check(throwsException<pm::Error>([] { pm::parseColor("not_a_color"); }), "parseColor(unknown) throws");
    checkEq(pm::parseColor("blue"), pm::Color(0, 0, 255, 255), "parseColor(\"blue\")");

    pm::Color parsed;
    check(pm::tryParseColor("orange", parsed), "tryParseColor(\"orange\") succeeds");
    checkEq(parsed, pm::Color(255, 165, 0, 255), "tryParseColor value");
    check(!pm::tryParseColor("nope", parsed), "tryParseColor(\"nope\") fails");

    check(pm::isColorName("maroon"), "isColorName(\"maroon\")");
    check(!pm::isColorName("mars"), "!isColorName(\"mars\")");
    checkEq(pm::allColors().size(), static_cast<std::size_t>(140), "allColors() has the 140 Nim color names");

    checkEq(pm::newColor(1, 2, 3, 4), pm::Color(1, 2, 3, 4), "newColor(r,g,b,a)");
    checkEq(pm::newColorHex(0x01020304u), pm::Color(1, 2, 3, 4), "newColorHex(0xRRGGBBAA)");
    checkEq(pm::newColorFloat(1.0f, 0.5f, 0.0f, 1.0f), pm::Color(255, 127, 0, 255), "newColorFloat(1,0.5,0)");

    checkEq(pm::fadeColor(pm::Color(10, 20, 30, 255), 0.5f), pm::Color(10, 20, 30, 127), "fadeColor(alpha 0.5)");
    checkNearly(pm::compareColorPct(pm::Color(0, 0, 0, 255), pm::Color(0, 0, 0, 255)), 100.0f, "compareColorPct(same) == 100");
    checkNearly(pm::compareColorPct(pm::Color(0, 0, 0, 255), pm::Color(255, 255, 255, 255)), 0.0f,
                "compareColorPct(black, white) == 0");
    checkNearly(pm::compareColorPct(pm::Color(100, 100, 100, 255), pm::Color(110, 100, 100, 255)),
                100.0f - (10.0f / 3.0f / 255.0f * 100.0f), "compareColorPct(10 red difference)");
}

/* ---------------------------------------------------------------- vectors */
void testVectors() {
    std::printf("[vectors]\n");
    const pm::Vec2 a(3.0f, 4.0f);
    const pm::Vec2 b(1.0f, 2.0f);
    checkEq(pm::vec2Add(a, b), pm::Vec2(4.0f, 6.0f), "vec2Add");
    checkEq(pm::vec2AddValue(a, 1.0f), pm::Vec2(4.0f, 5.0f), "vec2AddValue");
    checkEq(pm::vec2Subtract(a, b), pm::Vec2(2.0f, 2.0f), "vec2Subtract");
    checkEq(pm::vec2SubtractValue(a, 1.0f), pm::Vec2(2.0f, 3.0f), "vec2SubtractValue");
    checkEq(pm::vec2Multiply(a, b), pm::Vec2(3.0f, 8.0f), "vec2Multiply");
    checkEq(pm::vec2MultiplyValue(a, 2.0f), pm::Vec2(6.0f, 8.0f), "vec2MultiplyValue");
    checkEq(pm::vec2Divide(a, b), pm::Vec2(3.0f, 2.0f), "vec2Divide");
    checkEq(pm::vec2DivideValue(a, 2.0f), pm::Vec2(1.5f, 2.0f), "vec2DivideValue");
    checkNearly(pm::vec2Length(a), 5.0f, "vec2Length(3,4) == 5");
    checkNearly(pm::vec2LengthSqr(a), 25.0f, "vec2LengthSqr");
    checkNearly(pm::vec2Distance(a, b), std::sqrt(8.0f), "vec2Distance");
    checkNearly(pm::vec2DistanceSqr(a, b), 8.0f, "vec2DistanceSqr");
    checkNearly(pm::vec2Dot(a, b), 11.0f, "vec2Dot");
    checkNearly(pm::vec2Length(pm::vec2Normalize(a)), 1.0f, "vec2Normalize -> length 1");
    checkEq(pm::vec2Lerp(pm::Vec2(0, 0), pm::Vec2(10, 20), 0.25f), pm::Vec2(2.5f, 5.0f), "vec2Lerp");
    checkEq(pm::vec2Closest(pm::Vec2(9, 9), {pm::Vec2(0, 0), pm::Vec2(10, 10), pm::Vec2(20, 20)}), pm::Vec2(10, 10),
            "vec2Closest");

    const pm::Vec3 c(1.0f, 2.0f, 3.0f);
    const pm::Vec3 d(4.0f, 5.0f, 6.0f);
    checkEq(pm::vec3Add(c, d), pm::Vec3(5.0f, 7.0f, 9.0f), "vec3Add");
    checkEq(pm::vec3AddValue(c, 1.0f), pm::Vec3(2.0f, 3.0f, 4.0f), "vec3AddValue");
    checkEq(pm::vec3Subtract(d, c), pm::Vec3(3.0f, 3.0f, 3.0f), "vec3Subtract");
    checkEq(pm::vec3SubtractValue(d, 1.0f), pm::Vec3(3.0f, 4.0f, 5.0f), "vec3SubtractValue");
    checkEq(pm::vec3Multiply(c, d), pm::Vec3(4.0f, 10.0f, 18.0f), "vec3Multiply");
    checkEq(pm::vec3MultiplyValue(c, 2.0f), pm::Vec3(2.0f, 4.0f, 6.0f), "vec3MultiplyValue");
    checkEq(pm::vec3Divide(d, c), pm::Vec3(4.0f, 2.5f, 2.0f), "vec3Divide");
    checkEq(pm::vec3DivideValue(d, 2.0f), pm::Vec3(2.0f, 2.5f, 3.0f), "vec3DivideValue");
    checkNearly(pm::vec3Length(pm::Vec3(2, 3, 6)), 7.0f, "vec3Length(2,3,6) == 7");
    checkNearly(pm::vec3LengthSqr(pm::Vec3(2, 3, 6)), 49.0f, "vec3LengthSqr");
    checkNearly(pm::vec3Distance(c, d), std::sqrt(27.0f), "vec3Distance");
    checkNearly(pm::vec3DistanceSqr(c, d), 27.0f, "vec3DistanceSqr");
    checkNearly(pm::vec3Dot(c, d), 32.0f, "vec3Dot");
    checkEq(pm::vec3Cross(pm::Vec3(1, 0, 0), pm::Vec3(0, 1, 0)), pm::Vec3(0, 0, 1), "vec3Cross(x, y) == z");
    checkNearly(pm::vec3Length(pm::vec3Normalize(c)), 1.0f, "vec3Normalize -> length 1");
    checkEq(pm::vec3Lerp(pm::Vec3(0, 0, 0), pm::Vec3(10, 20, 30), 0.5f), pm::Vec3(5, 10, 15), "vec3Lerp");
    checkEq(pm::vec3Closest(pm::Vec3(9, 9, 9), {pm::Vec3(0, 0, 0), pm::Vec3(10, 10, 10)}), pm::Vec3(10, 10, 10),
            "vec3Closest");

    /* operators (types.hpp) */
    checkEq(a + b, pm::Vec2(4.0f, 6.0f), "Vec2 operator+");
    checkEq(a * 2.0f, pm::Vec2(6.0f, 8.0f), "Vec2 operator*(scalar)");
    checkEq(-c, pm::Vec3(-1.0f, -2.0f, -3.0f), "Vec3 operator- (unary)");
    check(c != d, "Vec3 operator!=");
}

/* ---------------------------------------------------------- worldToScreen */
void testWorldToScreen() {
    std::printf("[worldToScreen]\n");
    pm::Matrix identity{};
    identity[0] = identity[5] = identity[10] = identity[15] = 1.0f;

    /* algo 0 (column major) and algo 1 (row major) agree for the identity. */
    const pm::Vec2 center0 = pm::worldToScreen(identity, pm::Vec3(0, 0, 0), 1920, 1080, 0);
    const pm::Vec2 center1 = pm::worldToScreen(identity, pm::Vec3(0, 0, 0), 1920, 1080, 1);
    checkNearly(center0.x, 960.0f, "worldToScreen(0,0,0) -> screen center x");
    checkNearly(center0.y, 540.0f, "worldToScreen(0,0,0) -> screen center y");
    checkNearly(center1.x, 960.0f, "worldToScreen algo 1 -> screen center x");

    /* x_screen = (W/2 * ndc.x) + (ndc.x + W/2) -- the Python mapping is kept as is. */
    const pm::Vec2 right = pm::worldToScreen(identity, pm::Vec3(1, 0, 0), 1920, 1080, 0);
    checkNearly(right.x, 1921.0f, "worldToScreen(1,0,0) x keeps the Python mapping");
    checkNearly(right.y, 540.0f, "worldToScreen(1,0,0) y");

    const pm::Vec2 up = pm::worldToScreen(identity, pm::Vec3(0, 1, 0), 1920, 1080, 0);
    checkNearly(up.y, 1.0f, "worldToScreen(0,1,0) y is inverted (-540 + 541)");

    /* A projection like matrix: clip.w = z + 1 -> z = -2 is behind the camera. */
    pm::Matrix projection = identity;
    projection[11] = 1.0f;
    const pm::WtsResult behind = pm::worldToScreenNoExc(projection, pm::Vec3(0, 0, -2), 1920, 1080, 0);
    check(!behind.inBounds, "worldToScreenNoExc(behind the camera) -> !inBounds");
    check(throwsException<pm::OutOfBoundsError>([&] { pm::worldToScreen(projection, pm::Vec3(0, 0, -2), 1920, 1080, 0); }),
          "worldToScreen(behind the camera) throws OutOfBoundsError");

    const pm::WtsResult inFront = pm::worldToScreenNoExc(projection, pm::Vec3(0, 0, 1), 1920, 1080, 0);
    check(inFront.inBounds, "worldToScreenNoExc(in front of the camera) -> inBounds");

    /* float* overload */
    const float* raw = identity.data();
    checkNearly(pm::worldToScreen(raw, pm::Vec3(0, 0, 0), 800, 600, 0).x, 400.0f, "worldToScreen(float*) x");
    checkNearly(pm::worldToScreen(raw, pm::Vec3(0, 0, 0), 800, 600, 0).y, 300.0f, "worldToScreen(float*) y");
}

/* ------------------------------------------------------------------- misc */
void testTiming() {
    std::printf("[timing]\n");
    const double start = pm::runTime();
    pm::sleepMs(20);
    const double elapsed = pm::runTime() - start;
    check(elapsed >= 0.015, "sleepMs(20) waits at least 15 ms");
    check(pm::runTime() >= start, "runTime() is monotonic");
}

/* -------------------------------------------------------------------- aob */
void testAob() {
    std::printf("[aob]\n");
    const std::vector<std::uint8_t> buffer = {0x00, 0x89, 0x01, 0x48, 0x8B, 0x43, 0x00, 0x89, 0x01, 0x48, 0x8B, 0x43,
                                              0xFF, 0x89, 0xF1, 0x48, 0x8B, 0x43, 0x00, 0x00};

    const std::vector<pm::Address> single = pm::aobScanBytes("89 01 48 8B 43", buffer, true, 0);
    checkEq(single.size(), static_cast<std::size_t>(1), "aobScanBytes(single) finds one match");
    checkEq(single.empty() ? pm::Address(0) : single[0], static_cast<pm::Address>(1), "aobScanBytes offset");

    const std::vector<pm::Address> all = pm::aobScanBytes("89 01 48 8B 43", buffer, false, 0);
    checkEq(all.size(), static_cast<std::size_t>(2), "aobScanBytes(single=false) finds both matches");

    /* wildcards and nibble masks */
    checkEq(pm::aobScanBytes("89 ?? 48 8B 43", buffer, true, 0).size(), static_cast<std::size_t>(1), "? wildcard");
    checkEq(pm::aobScanBytes("?9 01 48 8B 43", buffer, true, 0).size(), static_cast<std::size_t>(1), "low nibble wildcard");
    checkEq(pm::aobScanBytes("8? 01 48 8B 43", buffer, true, 0).size(), static_cast<std::size_t>(1), "high nibble wildcard");
    checkEq(pm::aobScanBytes("89 F1 48", buffer, true, 0).size(), static_cast<std::size_t>(1), "exact match at offset 13");
    checkEq(pm::aobScanBytes("DE AD BE EF", buffer, true, 0).size(), static_cast<std::size_t>(0), "no match -> empty");

    /* algorithm 1 (plain left to right compare) has to agree with algorithm 0 */
    checkEq(pm::aobScanBytes("89 01 48 8B 43", buffer, false, 1).size(), static_cast<std::size_t>(2), "algorithm 1 finds both");
    /* the wildcard also matches 0x89 0xF1 0x48 0x8B 0x43 at offset 13 -> 3 hits */
    checkEq(pm::aobScanBytes("89 ?? 48 8B 43", buffer, false, 0).size(), static_cast<std::size_t>(3), "algorithm 0 wildcard (all)");
    checkEq(pm::aobScanBytes("89 ?? 48 8B 43", buffer, false, 1).size(), static_cast<std::size_t>(3), "algorithm 1 wildcard (all)");

    /* case insensitive + no spaces */
    checkEq(pm::aobScanBytes("89014?8b43", buffer, true, 0).size(), static_cast<std::size_t>(1), "pattern without spaces");
    checkEq(pm::aobScanBytes("89 01 48 8b 43", buffer, true, 0).size(), static_cast<std::size_t>(1), "lower case pattern");

    check(throwsException<pm::InvalidArgumentError>([] { pm::aobScanBytes("", std::vector<std::uint8_t>{1, 2, 3}); }),
          "empty pattern throws InvalidArgumentError");
    check(throwsException<pm::InvalidArgumentError>([] { pm::aobScanBytes("ZZ 01", std::vector<std::uint8_t>{1, 2, 3}); }),
          "invalid hex throws InvalidArgumentError");
}

/* ----------------------------------------------------------------- memory */
void testMemory() {
    std::printf("[memory]\n");
    pm::Process self = pm::openProcess(currentPid());
    check(static_cast<bool>(self), "openProcess(own pid)");
    checkEq(self.pid, currentPid(), "Process.pid");
    check(!self.name.empty(), "Process.name is filled in");
    check(self.base != 0, "Process.base points at the main module");
#if defined(PYMEOW_ARCH_X64)
    check(pm::is64Bit(self), "is64Bit(own process) on x64");
#endif

    /* ------------------------------------------------ scalars */
    std::int32_t intValue = 1337;
    checkEq(pm::readInt(self, reinterpret_cast<pm::Address>(&intValue)), 1337, "readInt");
    pm::writeInt(self, reinterpret_cast<pm::Address>(&intValue), -42);
    checkEq(intValue, static_cast<std::int32_t>(-42), "writeInt modifies the local variable");
    checkEq(pm::read<int>(self, reinterpret_cast<pm::Address>(&intValue)), -42, "read<int>");
    pm::write<int>(self, reinterpret_cast<pm::Address>(&intValue), 7);
    checkEq(intValue, static_cast<std::int32_t>(7), "write<int>");

    float floatValue = 2.5f;
    checkNearly(pm::readFloat(self, reinterpret_cast<pm::Address>(&floatValue)), 2.5f, "readFloat");
    pm::writeFloat(self, reinterpret_cast<pm::Address>(&floatValue), 3.75f);
    checkNearly(floatValue, 3.75f, "writeFloat");

    double doubleValue = 1.5;
    checkNearly(static_cast<float>(pm::readFloat64(self, reinterpret_cast<pm::Address>(&doubleValue))), 1.5f, "readFloat64");

    bool boolValue = true;
    check(pm::readBool(self, reinterpret_cast<pm::Address>(&boolValue)), "readBool(true)");
    pm::writeBool(self, reinterpret_cast<pm::Address>(&boolValue), false);
    check(!boolValue, "writeBool(false)");

    std::int8_t i8 = -8;
    checkEq(pm::readInt8(self, reinterpret_cast<pm::Address>(&i8)), static_cast<std::int8_t>(-8), "readInt8");
    std::int16_t i16 = -1600;
    checkEq(pm::readInt16(self, reinterpret_cast<pm::Address>(&i16)), static_cast<std::int16_t>(-1600), "readInt16");
    std::int64_t i64 = -1234567890123LL;
    checkEq(pm::readInt64(self, reinterpret_cast<pm::Address>(&i64)), static_cast<std::int64_t>(-1234567890123LL), "readInt64");
    std::uint8_t u8 = 200;
    checkEq(pm::readByte(self, reinterpret_cast<pm::Address>(&u8)), static_cast<std::uint8_t>(200), "readByte");
    std::uint16_t u16 = 60000;
    checkEq(pm::readUInt16(self, reinterpret_cast<pm::Address>(&u16)), static_cast<std::uint16_t>(60000), "readUInt16");
    std::uint32_t u32 = 4000000000u;
    checkEq(pm::readUInt(self, reinterpret_cast<pm::Address>(&u32)), 4000000000u, "readUInt");
    std::uint64_t u64 = 12345678901234567ULL;
    checkEq(pm::readUInt64(self, reinterpret_cast<pm::Address>(&u64)), 12345678901234567ULL, "readUInt64");

    /* ------------------------------------------------ vectors */
    pm::Vec2 vec2Value(1.5f, -2.5f);
    checkEq(pm::readVec2(self, reinterpret_cast<pm::Address>(&vec2Value)), pm::Vec2(1.5f, -2.5f), "readVec2");
    pm::writeVec2(self, reinterpret_cast<pm::Address>(&vec2Value), pm::Vec2(9.0f, 8.0f));
    checkEq(vec2Value, pm::Vec2(9.0f, 8.0f), "writeVec2");
    pm::Vec3 vec3Value(1.0f, 2.0f, 3.0f);
    checkEq(pm::readVec3(self, reinterpret_cast<pm::Address>(&vec3Value)), pm::Vec3(1.0f, 2.0f, 3.0f), "readVec3");
    pm::writeVec3(self, reinterpret_cast<pm::Address>(&vec3Value), pm::Vec3(4.0f, 5.0f, 6.0f));
    checkEq(vec3Value, pm::Vec3(4.0f, 5.0f, 6.0f), "writeVec3");

    /* ------------------------------------------------ sequences */
    std::int32_t numbers[5] = {10, 20, 30, 40, 50};
    const std::vector<std::int32_t> readNumbers = pm::readInts(self, reinterpret_cast<pm::Address>(numbers), 5);
    checkEq(readNumbers.size(), static_cast<std::size_t>(5), "readInts count");
    checkEq(readNumbers[2], static_cast<std::int32_t>(30), "readInts value");
    pm::writeInts(self, reinterpret_cast<pm::Address>(numbers), {1, 2, 3, 4, 5});
    checkEq(numbers[4], static_cast<std::int32_t>(5), "writeInts(initializer_list)");
    checkEq(pm::readSeq<std::int32_t>(self, reinterpret_cast<pm::Address>(numbers), 5)[0], static_cast<std::int32_t>(1),
            "readSeq<int32>");
    pm::writeArray<std::int32_t>(self, reinterpret_cast<pm::Address>(numbers), {7, 7, 7, 7, 7});
    checkEq(numbers[1], static_cast<std::int32_t>(7), "writeArray<int32>");

    std::uint8_t bytes[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    checkEq(pm::readBytes(self, reinterpret_cast<pm::Address>(bytes), 4)[0], static_cast<std::uint8_t>(0xDE), "readBytes");
    pm::writeBytes(self, reinterpret_cast<pm::Address>(bytes), {0x90, 0x90, 0x90, 0x90});
    checkEq(bytes[3], static_cast<std::uint8_t>(0x90), "writeBytes(initializer_list)");

    /* ------------------------------------------------ strings */
    char text[32];
    std::memset(text, 0, sizeof(text));
    std::strcpy(text, "pyMeow");
    checkEq(pm::readString(self, reinterpret_cast<pm::Address>(text), 30), std::string("pyMeow"), "readString");
    checkEq(pm::readString(self, reinterpret_cast<pm::Address>(text), 3), std::string("pyM"), "readString(size) truncates");

    char target[32];
    std::memset(target, 'x', sizeof(target));
    pm::writeString(self, reinterpret_cast<pm::Address>(target), "abc");
    check(target[0] == 'a' && target[1] == 'b' && target[2] == 'c' && target[3] == 'x', "writeString does not write a NUL");
    std::memset(target, 'x', sizeof(target));
    pm::writeStringZ(self, reinterpret_cast<pm::Address>(target), "abc");
    check(target[3] == '\0', "writeStringZ writes the terminating NUL");

    /* UTF-16LE ("wide") string of the Python API */
    const std::uint16_t wide[] = {'p', 'y', 'M', 'e', 'o', 'w', 0};
    checkEq(pm::readWString(self, reinterpret_cast<pm::Address>(wide), 30), std::string("pyMeow"), "readWString");
    checkEq(pm::bytesToString(self, reinterpret_cast<pm::Address>(wide), 12), std::string("pyMeow"), "bytesToString(UTF-16LE)");

    /* ------------------------------------------------ raw pointer API */
    std::uint32_t rawValue = 0xABCD;
    std::uint32_t rawCopy = 0;
    pm::readPointer(self, reinterpret_cast<pm::Address>(&rawValue), &rawCopy, sizeof(rawCopy));
    checkEq(rawCopy, 0xABCDu, "readPointer");
    rawCopy = 0x1234;
    pm::writePointer(self, reinterpret_cast<pm::Address>(&rawValue), &rawCopy, sizeof(rawCopy));
    checkEq(rawValue, 0x1234u, "writePointer");
    check(pm::tryReadPointer(self, reinterpret_cast<pm::Address>(&rawValue), &rawCopy, sizeof(rawCopy)), "tryReadPointer");

    /* an unmapped address has to fail */
    check(!pm::tryReadPointer(self, static_cast<pm::Address>(0x10), &rawCopy, sizeof(rawCopy)), "tryReadPointer(0x10) fails");
    check(throwsException<pm::MemoryError>([&] { pm::readPointer(self, static_cast<pm::Address>(0x10), &rawCopy, 4); }),
          "readPointer(0x10) throws MemoryError");
    try {
        pm::readPointer(self, static_cast<pm::Address>(0x10), &rawCopy, 4);
    } catch (const pm::MemoryError& e) {
        check(std::string(e.what()).find("Read failed") != std::string::npos, "MemoryError message");
        checkEq(e.address, static_cast<std::uintptr_t>(0x10), "MemoryError.address");
    }

    /* ------------------------------------------------ pointer chains */
    std::uint64_t chainTarget = 0x1234ABCDULL;
    std::uint64_t* chainMid = &chainTarget;
    std::uint64_t** chainRoot = &chainMid;
    const pm::Address resolved =
        pm::pointerChain64(self, reinterpret_cast<pm::Address>(&chainRoot), {static_cast<pm::Address>(0), static_cast<pm::Address>(0)});
    checkEq(resolved, reinterpret_cast<pm::Address>(&chainTarget), "pointerChain64 resolves the chain");
    checkEq(pm::readUInt64(self, resolved), 0x1234ABCDULL, "value behind the resolved chain");

    std::uint64_t array[4] = {10, 20, 30, 40};
    std::uint64_t* arrayMid = array;
    std::uint64_t** arrayRoot = &arrayMid;
    const pm::Address arrayAddress = pm::pointerChain64(
        self, reinterpret_cast<pm::Address>(&arrayRoot), {static_cast<pm::Address>(0), static_cast<pm::Address>(16)});
    checkEq(pm::readUInt64(self, arrayAddress), 30ULL, "pointerChain64 applies the last offset without dereferencing");

#if defined(PYMEOW_ARCH_X86)
    /* pointerChain32 is for 32 bit targets: on a 64 bit process the pointers of
     * the chain do not fit into 32 bits, so this is only tested on x86. */
    std::uint32_t target32 = 77;
    std::uint32_t* mid32 = &target32;
    std::uint32_t** root32 = &mid32;
    const pm::Address resolved32 =
        pm::pointerChain32(self, reinterpret_cast<pm::Address>(&root32), {static_cast<pm::Address>(0), static_cast<pm::Address>(0)});
    checkEq(pm::readUInt(self, resolved32), 77u, "pointerChain32");
#endif
    check(throwsException<pm::InvalidArgumentError>(
              [&] { pm::pointerChain64(self, reinterpret_cast<pm::Address>(&chainRoot), std::vector<pm::Address>()); }),
          "pointerChain64 without offsets throws");

    /* ------------------------------------------------ modules */
    const std::vector<pm::Module> modules = pm::enumModules(self);
    check(!modules.empty(), "enumModules(own process) is not empty");
    if (!modules.empty()) {
        const pm::Module& first = modules.front();
        check(!first.name.empty(), "Module.name");
        check(first.base != 0, "Module.base");
        check(first.size != 0, "Module.size");
        check(first.end == first.base + first.size, "Module.end == base + size");
        check(pm::moduleExists(self, first.name), "moduleExists(first module)");
        checkEq(pm::getModule(self, first.name).base, first.base, "getModule(first module)");
        check(static_cast<bool>(pm::findModule(self, first.name)), "findModule(first module)");
        check(!static_cast<bool>(pm::findModule(self, "no_such_module_xyz")), "findModule(unknown) -> invalid Module");
        check(throwsException<pm::NotFoundError>([] {
                  pm::Process p;
                  p.pid = 1;
                  pm::getModule(p, "no_such_module_xyz");
              }),
              "getModule(unknown) throws NotFoundError");

        const std::vector<pm::MemoryRegion> regions = pm::enumMemoryRegions(self, first);
        check(!regions.empty(), "enumMemoryRegions(first module) is not empty");
        if (!regions.empty()) {
            check(regions.front().size != 0, "MemoryRegion.size");
            check(regions.front().end == regions.front().start + regions.front().size, "MemoryRegion.end");
#if !defined(_WIN32)
            check(!regions.front().perms.empty(), "MemoryRegion.perms (Linux)");
#endif
        }

        /* ------------------------------------------------ AOB on real memory */
        std::vector<std::uint8_t> haystack(4096, 0x00);
        haystack[1000] = 0x89;
        haystack[1001] = 0x01;
        haystack[1002] = 0x48;
        haystack[1003] = 0x8B;
        const pm::Address haystackAddress = reinterpret_cast<pm::Address>(haystack.data());
        const std::vector<pm::Address> hits =
            pm::aobScanRange(self, "89 01 48 8B", haystackAddress, haystackAddress + haystack.size(), false, true, 0);
        checkEq(hits.size(), static_cast<std::size_t>(1), "aobScanRange finds the pattern in our own memory");
        if (!hits.empty()) checkEq(hits[0], haystackAddress + 1000, "aobScanRange returns the absolute address");
        const std::vector<pm::Address> relativeHits =
            pm::aobScanRange(self, "89 01 48 8B", haystackAddress, haystackAddress + haystack.size(), true, true, 0);
        if (!relativeHits.empty()) checkEq(relativeHits[0], static_cast<pm::Address>(1000), "aobScanRange(relative)");
        check(throwsException<pm::InvalidArgumentError>(
                  [&] { pm::aobScanRange(self, "89 01", haystackAddress + 10, haystackAddress, false, true, 0); }),
              "aobScanRange(start > end) throws");

        /* scanning a whole module must not crash and must not find random patterns */
        const std::vector<pm::Address> moduleHits = pm::aobScanModule(self, first.name, "FF FF FF FF FF FF FF FF FF FF FF FF",
                                                                      false, true, 0);
        check(moduleHits.empty() || moduleHits.front() != 0, "aobScanModule runs over the whole module");
    }

    /* ------------------------------------------------ debug flag */
    pm::Process debugProcess = pm::openProcess(currentPid(), true);
    check(debugProcess.debug, "openProcess(pid, debug = true)");
    std::int32_t debugValue = 5;
    std::printf("  (the next line is the expected debug output)\n");
    checkEq(pm::readInt(debugProcess, reinterpret_cast<pm::Address>(&debugValue)), 5, "readInt with debug logging");

    /* ------------------------------------- remote allocation is Windows/ptrace */
#if !defined(_WIN32)
    /* ptrace can not attach to the own process, so this has to fail cleanly */
    check(throwsException<pm::Error>([&] { pm::allocateMemory(self, 4096); }), "allocateMemory(self) throws on Linux");
    check(!pm::freeMemory(self, 0), "freeMemory(0) fails");
    check(throwsException<pm::Error>([] { pm::getProcAddress("kernel32.dll", "LoadLibraryA"); }),
          "getProcAddress throws on Linux");
    check(throwsException<pm::Error>([&] { pm::injectLibrary(self, "/tmp/x.so"); }), "injectLibrary throws on Linux");
    check(throwsException<pm::Error>([&] { pm::injectShellcode(self, std::vector<std::uint8_t>{0x90}); }),
          "injectShellcode throws on Linux");
#endif

    pm::closeProcess(self);
    check(self.handle == nullptr, "closeProcess clears the handle");
    pm::closeProcess(debugProcess);
}

/* -------------------------------------------------------------- dynamic API */
void testDynamicApi() {
    std::printf("[dynamic read/write]\n");
    pm::Process self = pm::openProcess(currentPid());

    check(pm::dataTypeFromString("int") == pm::DataType::Int, "dataTypeFromString(\"int\")");
    check(pm::dataTypeFromString("int32") == pm::DataType::Int, "dataTypeFromString(\"int32\")");
    check(pm::dataTypeFromString("float") == pm::DataType::Float, "dataTypeFromString(\"float\")");
    check(pm::dataTypeFromString("str") == pm::DataType::String, "dataTypeFromString(\"str\")");
    check(pm::dataTypeFromString("vec3") == pm::DataType::Vec3, "dataTypeFromString(\"vec3\")");
    check(throwsException<pm::InvalidArgumentError>([] { pm::dataTypeFromString("nonsense"); }),
          "dataTypeFromString(unknown) throws");
    checkEq(pm::dataTypeSize(pm::DataType::Float), static_cast<std::size_t>(4), "dataTypeSize(Float) == 4");
    checkEq(pm::dataTypeSize(pm::DataType::Int64), static_cast<std::size_t>(8), "dataTypeSize(Int64) == 8");
    check(std::strcmp(pm::dataTypeToString(pm::DataType::Vec2), "vec2") == 0, "dataTypeToString(Vec2)");

    std::int32_t value = 99;
    const pm::Value read = pm::readValue(self, reinterpret_cast<pm::Address>(&value), pm::DataType::Int);
    checkEq(std::get<std::int32_t>(read), 99, "readValue(Int)");

    char text[16];
    std::memset(text, 0, sizeof(text));
    std::strcpy(text, "hello");
    const pm::Value readText = pm::readValue(self, reinterpret_cast<pm::Address>(text), pm::DataType::String);
    checkEq(std::get<std::string>(readText), std::string("hello"), "readValue(String)");

    pm::closeProcess(self);
}

/* -------------------------------------------------------------------- png */
namespace {

std::uint32_t readU32BE(const std::uint8_t* data) {
    return (static_cast<std::uint32_t>(data[0]) << 24) | (static_cast<std::uint32_t>(data[1]) << 16) |
           (static_cast<std::uint32_t>(data[2]) << 8) | static_cast<std::uint32_t>(data[3]);
}

/* The encoder uses zlib "stored" blocks, so a tiny parser is enough to get the
 * scanlines back and verify them. */
std::vector<std::uint8_t> inflateStored(const std::vector<std::uint8_t>& zlibData) {
    std::vector<std::uint8_t> out;
    if (zlibData.size() < 6) return out;
    std::size_t i = 2; /* zlib header */
    while (i + 4 < zlibData.size()) {
        const std::uint8_t header = zlibData[i++];
        const bool finalBlock = (header & 1u) != 0;
        const int type = (header >> 1) & 3;
        if (type != 0) return std::vector<std::uint8_t>();
        const std::uint16_t len = static_cast<std::uint16_t>(zlibData[i] | (static_cast<std::uint16_t>(zlibData[i + 1]) << 8));
        const std::uint16_t nlen = static_cast<std::uint16_t>(zlibData[i + 2] | (static_cast<std::uint16_t>(zlibData[i + 3]) << 8));
        i += 4;
        if (static_cast<std::uint16_t>(~len) != nlen) return std::vector<std::uint8_t>();
        if (i + len > zlibData.size()) return std::vector<std::uint8_t>();
        out.insert(out.end(), zlibData.begin() + static_cast<std::ptrdiff_t>(i),
                   zlibData.begin() + static_cast<std::ptrdiff_t>(i + len));
        i += len;
        if (finalBlock) break;
    }
    return out;
}

} /* namespace */

void testPng() {
    std::printf("[png]\n");
    const int width = 3;
    const int height = 2;
    const std::vector<std::uint8_t> pixels = {255, 0,   0,   255, 0, 255, 0, 255, 0,   0,   255, 255,
                                              10,  20,  30,  40,  50, 60, 70, 80, 90,  100, 110, 120};
    const std::vector<std::uint8_t> png = pm::detail::encodePng(width, height, pixels.data());

    check(png.size() > 57, "encodePng produces data");
    const std::uint8_t signature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    check(std::memcmp(png.data(), signature, 8) == 0, "PNG signature");

    /* IHDR */
    checkEq(readU32BE(&png[8]), 13u, "IHDR length");
    check(std::memcmp(&png[12], "IHDR", 4) == 0, "IHDR type");
    checkEq(readU32BE(&png[16]), static_cast<std::uint32_t>(width), "IHDR width");
    checkEq(readU32BE(&png[20]), static_cast<std::uint32_t>(height), "IHDR height");
    checkEq(png[24], static_cast<std::uint8_t>(8), "IHDR bit depth 8");
    checkEq(png[25], static_cast<std::uint8_t>(6), "IHDR color type RGBA");
    checkEq(readU32BE(&png[29]), pm::detail::crc32(&png[12], 17), "IHDR CRC");

    /* IDAT */
    const std::uint32_t idatLength = readU32BE(&png[33]);
    check(std::memcmp(&png[37], "IDAT", 4) == 0, "IDAT type");
    std::vector<std::uint8_t> idat(png.begin() + 41, png.begin() + 41 + idatLength);
    checkEq(readU32BE(&png[41 + idatLength]), pm::detail::crc32(&png[37], 4 + idatLength), "IDAT CRC");
    checkEq(idat[0], static_cast<std::uint8_t>(0x78), "zlib header byte 0");
    checkEq(idat[1], static_cast<std::uint8_t>(0x01), "zlib header byte 1");

    const std::uint32_t adler = readU32BE(&idat[idat.size() - 4]);
    const std::vector<std::uint8_t> raw = inflateStored(idat);
    check(!raw.empty(), "IDAT inflates (stored blocks)");
    checkEq(adler, pm::detail::adler32(raw.data(), raw.size()), "adler32 checksum of the raw scanlines");
    checkEq(raw.size(), static_cast<std::size_t>(height * (1 + width * 4)), "raw size == height * (filter byte + row)");

    /* un-filter (filter type 0 per row) and compare with the source pixels */
    std::vector<std::uint8_t> decoded;
    for (int row = 0; row < height; ++row) {
        const std::size_t offset = static_cast<std::size_t>(row) * (1 + static_cast<std::size_t>(width) * 4);
        checkEq(raw[offset], static_cast<std::uint8_t>(0), "filter type 0");
        decoded.insert(decoded.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset) + 1,
                       raw.begin() + static_cast<std::ptrdiff_t>(offset) + 1 + static_cast<std::ptrdiff_t>(width) * 4);
    }
    check(decoded == pixels, "decoded scanlines match the source pixels");

    /* IEND */
    const std::size_t iendOffset = 41 + idatLength + 4;
    checkEq(readU32BE(&png[iendOffset]), 0u, "IEND length");
    check(std::memcmp(&png[iendOffset + 4], "IEND", 4) == 0, "IEND type");
    checkEq(readU32BE(&png[iendOffset + 8]), pm::detail::crc32(&png[iendOffset + 4], 4), "IEND CRC");
    checkEq(png.size(), iendOffset + 12, "file ends after IEND");

    /* crc32 / adler32 against known values */
    checkEq(pm::detail::crc32(reinterpret_cast<const std::uint8_t*>("123456789"), 9), 0xCBF43926u, "crc32(\"123456789\")");
    checkEq(pm::detail::adler32(reinterpret_cast<const std::uint8_t*>("Wikipedia"), 9), 0x11E60398u, "adler32(\"Wikipedia\")");

    /* write to disk and read back */
    const std::string fileName = "pymeow_test.png";
    check(pm::detail::writePng(fileName, width, height, pixels.data()), "writePng()");
    std::FILE* file = std::fopen(fileName.c_str(), "rb");
    check(file != nullptr, "the PNG file exists");
    if (file != nullptr) {
        std::vector<std::uint8_t> fromDisk;
        std::uint8_t chunk[512];
        std::size_t read = 0;
        while ((read = std::fread(chunk, 1, sizeof(chunk), file)) > 0) fromDisk.insert(fromDisk.end(), chunk, chunk + read);
        std::fclose(file);
        check(fromDisk == png, "the file on disk matches the encoded buffer");
        std::remove(fileName.c_str());
    }
}

} /* namespace */

int main() {
    std::printf("pyMeow C++ core tests (%s, %s)\n", pm::version(), pm::systemName());

    try {
        testBasics();
        testColors();
        testVectors();
        testWorldToScreen();
        testTiming();
        testAob();
        testMemory();
        testDynamicApi();
        testPng();
    } catch (const std::exception& e) {
        std::printf("UNEXPECTED EXCEPTION: %s\n", e.what());
        return 2;
    }

    std::printf("\n%d checks, %d failures\n", gChecks, gFailures);
    return gFailures == 0 ? 0 : 1;
}
