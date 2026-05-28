#pragma once
#include <cmath>

struct Vec2 {
    float x, y;

    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return { x + o.x, y + o.y }; }
    Vec2 operator-(const Vec2& o) const { return { x - o.x, y - o.y }; }
    Vec2 operator*(float s)        const { return { x * s,   y * s   }; }
    Vec2 operator/(float s)        const { return { x / s,   y / s   }; }

    float dot(const Vec2& o) const { return x * o.x + y * o.y; }        // 내적
    float length()           const { return std::sqrt(x * x + y * y); } // 벡터 크기
    Vec2  normalized()       const { float l = length(); return l > 0 ? *this / l : Vec2{}; } // 단위벡터
};
