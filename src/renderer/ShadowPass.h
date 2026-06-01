#pragma once
#include "Shader.h"
#include "ShadowMap.h"
#include "../resource/ObjLoader.h"

// 깊이만 기록하는 Shadow Pass 셰이더
struct ShadowShader : IShader {
    const Mesh* mesh    = nullptr;
    Mat4        lightMVP;

    VertexOut Vertex(int idx) override {
        VertexOut out;
        out.clipPos = lightMVP * Vec4(mesh->vertices[idx].pos, 1.0f);
        return out;
    }

    Color Fragment(const Varying&) override {
        return Color(0, 0, 0);  // shadow pass는 색상 불필요
    }
};

// 깊이 버퍼만 채우는 패스
class ShadowPassRenderer {
public:
    explicit ShadowPassRenderer(ShadowMap& sm) : m_sm(sm) {}

    void Render(ShadowShader& shader, const std::vector<int>& indices);

private:
    ShadowMap& m_sm;

    Vec3 Barycentric(Vec2 A, Vec2 B, Vec2 C, Vec2 P);
};
