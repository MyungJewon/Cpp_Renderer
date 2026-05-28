#pragma once
#include "../math/Vec2.h"
#include "../math/Vec3.h"
#include "../math/Vec4.h"
#include "../math/Mat4.h"
#include "Framebuffer.h"

// Data carried from vertex shader to fragment shader
struct Varying {
    Vec3 worldPos;
    Vec3 normal;
    Vec2 uv;
    Color color;
};

// Interpolated vertex output in clip space
struct VertexOut {
    Vec4    clipPos;
    Varying varying;
};

struct IShader {
    virtual ~IShader() = default;
    virtual VertexOut  Vertex(int index)          = 0;
    virtual Color      Fragment(const Varying& v) = 0;
};
