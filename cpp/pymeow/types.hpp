/*
 * pyMeow C++ -- basic types
 * https://github.com/qb-0/pyMeow
 *
 * Vec2 / Vec3 / Color / Rect are layout compatible with the raylib types
 * Vector2 / Vector3 / Color / Rectangle, so a build with raylib available
 * converts between them implicitly (see the conversion operators below).
 * With PYMEOW_NO_GRAPHICS they are plain standalone POD types.
 */
#ifndef PYMEOW_TYPES_HPP
#define PYMEOW_TYPES_HPP

#include "common.hpp"

#include <array>
#include <cstdint>
#include <type_traits>

namespace pymeow {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    constexpr Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    constexpr Vec2 operator*(const Vec2& o) const { return Vec2(x * o.x, y * o.y); }
    constexpr Vec2 operator/(const Vec2& o) const { return Vec2(x / o.x, y / o.y); }
    constexpr Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    constexpr Vec2 operator/(float s) const { return Vec2(x / s, y / s); }
    constexpr Vec2 operator-() const { return Vec2(-x, -y); }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    Vec2& operator/=(float s) { x /= s; y /= s; return *this; }
    constexpr bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Vec2& o) const { return !(*this == o); }

#if defined(PYMEOW_WITH_RAYLIB)
    constexpr Vec2(const ::Vector2& v) : x(v.x), y(v.y) {}
    constexpr operator ::Vector2() const { return ::Vector2{x, y}; }
#endif
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    constexpr Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    constexpr Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    constexpr Vec3 operator*(const Vec3& o) const { return Vec3(x * o.x, y * o.y, z * o.z); }
    constexpr Vec3 operator/(const Vec3& o) const { return Vec3(x / o.x, y / o.y, z / o.z); }
    constexpr Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    constexpr Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }
    constexpr Vec3 operator-() const { return Vec3(-x, -y, -z); }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    Vec3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }
    constexpr bool operator==(const Vec3& o) const { return x == o.x && y == o.y && z == o.z; }
    constexpr bool operator!=(const Vec3& o) const { return !(*this == o); }

#if defined(PYMEOW_WITH_RAYLIB)
    constexpr Vec3(const ::Vector3& v) : x(v.x), y(v.y), z(v.z) {}
    constexpr operator ::Vector3() const { return ::Vector3{x, y, z}; }
#endif
};

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    constexpr Color() = default;
    constexpr Color(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_, std::uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}
    /* 0xRRGGBBAA, the same layout raylib uses for its hex colors. */
    constexpr explicit Color(std::uint32_t rgba)
        : r(static_cast<std::uint8_t>((rgba >> 24) & 0xFF)),
          g(static_cast<std::uint8_t>((rgba >> 16) & 0xFF)),
          b(static_cast<std::uint8_t>((rgba >> 8) & 0xFF)),
          a(static_cast<std::uint8_t>(rgba & 0xFF)) {}

    constexpr bool operator==(const Color& o) const {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
    constexpr bool operator!=(const Color& o) const { return !(*this == o); }

#if defined(PYMEOW_WITH_RAYLIB)
    constexpr Color(const ::Color& c) : r(c.r), g(c.g), b(c.b), a(c.a) {}
    constexpr operator ::Color() const { return ::Color{r, g, b, a}; }
#endif
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    constexpr Rect() = default;
    constexpr Rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), width(w_), height(h_) {}

#if defined(PYMEOW_WITH_RAYLIB)
    constexpr Rect(const ::Rectangle& rec) : x(rec.x), y(rec.y), width(rec.width), height(rec.height) {}
    constexpr operator ::Rectangle() const { return ::Rectangle{x, y, width, height}; }
#endif
};

/* One pixel of a screen capture (see pixel.hpp). */
struct Pixel {
    int x = 0;
    int y = 0;
    Color color;
};

/* View / projection matrix as it is stored in most games: 16 floats. */
using Matrix = std::array<float, 16>;

/* A few colors that are always available, even without raylib. */
namespace colors {
inline constexpr Color transparent{0, 0, 0, 0};
inline constexpr Color white{255, 255, 255, 255};
inline constexpr Color black{0, 0, 0, 255};
inline constexpr Color red{255, 0, 0, 255};
inline constexpr Color green{0, 255, 0, 255};
inline constexpr Color blue{0, 0, 255, 255};
inline constexpr Color cyan{0, 255, 255, 255};
inline constexpr Color magenta{255, 0, 255, 255};
inline constexpr Color yellow{255, 255, 0, 255};
inline constexpr Color orange{255, 165, 0, 255};
inline constexpr Color gray{130, 130, 130, 255};
} /* namespace colors */

/* The types must be trivially copyable: they are read from / written to the
 * target process with a single memory transfer. */
static_assert(std::is_trivially_copyable<Vec2>::value && sizeof(Vec2) == 8, "Vec2 layout");
static_assert(std::is_trivially_copyable<Vec3>::value && sizeof(Vec3) == 12, "Vec3 layout");
static_assert(std::is_trivially_copyable<Color>::value && sizeof(Color) == 4, "Color layout");
static_assert(std::is_trivially_copyable<Rect>::value && sizeof(Rect) == 16, "Rect layout");

#if defined(PYMEOW_WITH_RAYLIB)
static_assert(sizeof(::Vector2) == sizeof(Vec2), "Vec2 must match raylib Vector2");
static_assert(sizeof(::Vector3) == sizeof(Vec3), "Vec3 must match raylib Vector3");
static_assert(sizeof(::Color) == sizeof(Color), "Color must match raylib Color");
static_assert(sizeof(::Rectangle) == sizeof(Rect), "Rect must match raylib Rectangle");
#endif

} /* namespace pymeow */

#endif /* PYMEOW_TYPES_HPP */
