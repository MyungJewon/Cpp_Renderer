#pragma once
#include <algorithm>
#include <cmath>

constexpr float PI = 3.14159265358979323846f;

inline float Clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float DegToRad(float deg) {
    return deg * PI / 180.0f;
}

inline float RadToDeg(float rad) {
    return rad * 180.0f / PI;
}
