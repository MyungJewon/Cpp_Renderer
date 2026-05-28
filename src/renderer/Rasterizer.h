#pragma once
#include "Framebuffer.h"
#include "Shader.h"
#include "../math/Vec3.h"

class Rasterizer {
public:
    explicit Rasterizer(Framebuffer& fb) : m_fb(fb) {}

    void DrawTriangle(VertexOut v0, VertexOut v1, VertexOut v2, IShader& shader); // 클립 공간 정점 3개로 삼각형 래스터화

private:
    Vec3 Barycentric(Vec2 A, Vec2 B, Vec2 C, Vec2 P); // P의 무게중심 좌표 반환, 음수 성분이면 삼각형 외부
    bool ClipW(const Vec4& v) { return v.w > 0.0f; }  // near plane 뒤 정점 거름

    Framebuffer& m_fb;
};
