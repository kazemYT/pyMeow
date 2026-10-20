/*
 * pyMeow C++ -- vector math (port of vec.nim / raymath)
 * https://github.com/qb-0/pyMeow
 *
 * Header only: every function is inline.
 */
#ifndef PYMEOW_VEC_HPP
#define PYMEOW_VEC_HPP

#include "types.hpp"

#include <cmath>
#include <initializer_list>
#include <limits>

namespace pymeow {

/* -------------------------------------------------------------- construction */
inline Vec2 vec2(float x = 0.0f, float y = 0.0f) { return Vec2(x, y); }
inline Vec3 vec3(float x = 0.0f, float y = 0.0f, float z = 0.0f) { return Vec3(x, y, z); }

/* -------------------------------------------------------------------- Vec2 */
inline Vec2 vec2Add(Vec2 v1, Vec2 v2) { return Vec2(v1.x + v2.x, v1.y + v2.y); }
inline Vec2 vec2AddValue(Vec2 v, float value) { return Vec2(v.x + value, v.y + value); }
inline Vec2 vec2Subtract(Vec2 v1, Vec2 v2) { return Vec2(v1.x - v2.x, v1.y - v2.y); }
inline Vec2 vec2SubtractValue(Vec2 v, float value) { return Vec2(v.x - value, v.y - value); }
inline Vec2 vec2Multiply(Vec2 v1, Vec2 v2) { return Vec2(v1.x * v2.x, v1.y * v2.y); }
inline Vec2 vec2MultiplyValue(Vec2 v, float value) { return Vec2(v.x * value, v.y * value); }
inline Vec2 vec2Divide(Vec2 v1, Vec2 v2) { return Vec2(v1.x / v2.x, v1.y / v2.y); }
inline Vec2 vec2DivideValue(Vec2 v, float value) { return Vec2(v.x / value, v.y / value); }

inline float vec2Length(Vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }
inline float vec2LengthSqr(Vec2 v) { return v.x * v.x + v.y * v.y; }
inline float vec2Distance(Vec2 v1, Vec2 v2) {
    const float dx = v2.x - v1.x;
    const float dy = v2.y - v1.y;
    return std::sqrt(dx * dx + dy * dy);
}
inline float vec2DistanceSqr(Vec2 v1, Vec2 v2) {
    const float dx = v2.x - v1.x;
    const float dy = v2.y - v1.y;
    return dx * dx + dy * dy;
}
inline float vec2Dot(Vec2 v1, Vec2 v2) { return v1.x * v2.x + v1.y * v2.y; }
inline Vec2 vec2Normalize(Vec2 v) {
    const float len = vec2Length(v);
    return (len > 0.0f) ? Vec2(v.x / len, v.y / len) : Vec2(0.0f, 0.0f);
}
inline Vec2 vec2Lerp(Vec2 v1, Vec2 v2, float amount) {
    return Vec2(v1.x + amount * (v2.x - v1.x), v1.y + amount * (v2.y - v1.y));
}

/* Closest vector of a list to v. Returns a zero vector for an empty list. */
inline Vec2 vec2Closest(Vec2 v, std::initializer_list<Vec2> vectorList) {
    Vec2 result{};
    float closest = std::numeric_limits<float>::max();
    for (const Vec2& other : vectorList) {
        const float dist = vec2Distance(v, other);
        if (dist < closest) {
            closest = dist;
            result = other;
        }
    }
    return result;
}

/* -------------------------------------------------------------------- Vec3 */
inline Vec3 vec3Add(Vec3 v1, Vec3 v2) { return Vec3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z); }
inline Vec3 vec3AddValue(Vec3 v, float value) { return Vec3(v.x + value, v.y + value, v.z + value); }
inline Vec3 vec3Subtract(Vec3 v1, Vec3 v2) { return Vec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z); }
inline Vec3 vec3SubtractValue(Vec3 v, float value) { return Vec3(v.x - value, v.y - value, v.z - value); }
inline Vec3 vec3Multiply(Vec3 v1, Vec3 v2) { return Vec3(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z); }
inline Vec3 vec3MultiplyValue(Vec3 v, float value) { return Vec3(v.x * value, v.y * value, v.z * value); }
inline Vec3 vec3Divide(Vec3 v1, Vec3 v2) { return Vec3(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z); }
inline Vec3 vec3DivideValue(Vec3 v, float value) { return Vec3(v.x / value, v.y / value, v.z / value); }

inline float vec3Length(Vec3 v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
inline float vec3LengthSqr(Vec3 v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
inline float vec3Distance(Vec3 v1, Vec3 v2) {
    const float dx = v2.x - v1.x;
    const float dy = v2.y - v1.y;
    const float dz = v2.z - v1.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}
inline float vec3DistanceSqr(Vec3 v1, Vec3 v2) {
    const float dx = v2.x - v1.x;
    const float dy = v2.y - v1.y;
    const float dz = v2.z - v1.z;
    return dx * dx + dy * dy + dz * dz;
}
inline float vec3Dot(Vec3 v1, Vec3 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }
inline Vec3 vec3Cross(Vec3 v1, Vec3 v2) {
    return Vec3(v1.y * v2.z - v1.z * v2.y,
                v1.z * v2.x - v1.x * v2.z,
                v1.x * v2.y - v1.y * v2.x);
}
inline Vec3 vec3Normalize(Vec3 v) {
    const float len = vec3Length(v);
    return (len > 0.0f) ? Vec3(v.x / len, v.y / len, v.z / len) : Vec3(0.0f, 0.0f, 0.0f);
}
inline Vec3 vec3Lerp(Vec3 v1, Vec3 v2, float amount) {
    return Vec3(v1.x + amount * (v2.x - v1.x),
                v1.y + amount * (v2.y - v1.y),
                v1.z + amount * (v2.z - v1.z));
}

inline Vec3 vec3Closest(Vec3 v, std::initializer_list<Vec3> vectorList) {
    Vec3 result{};
    float closest = std::numeric_limits<float>::max();
    for (const Vec3& other : vectorList) {
        const float dist = vec3Distance(v, other);
        if (dist < closest) {
            closest = dist;
            result = other;
        }
    }
    return result;
}

} /* namespace pymeow */

#endif /* PYMEOW_VEC_HPP */
