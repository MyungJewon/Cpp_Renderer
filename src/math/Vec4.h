#pragma once
#include "Vec3.h"

struct Vec4 {
    float x, y, z, w;

    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

    Vec4 operator+(const Vec4& o) const { return { x + o.x, y + o.y, z + o.z, w + o.w }; }
    Vec4 operator-(const Vec4& o) const { return { x - o.x, y - o.y, z - o.z, w - o.w }; }
    Vec4 operator*(float s)        const { return { x * s,   y * s,   z * s,   w * s   }; }
    Vec4 operator/(float s)        const { return { x / s,   y / s,   z / s,   w / s   }; }

    Vec3 xyz() const { return { x, y, z }; }                              // xyz 성분 추출
    Vec3 PerspectiveDivide() const { return { x / w, y / w, z / w }; }  // 클립 → NDC 변환
};
