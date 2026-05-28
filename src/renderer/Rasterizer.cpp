#include "Rasterizer.h"
#include "../math/MathUtils.h"
#include <algorithm>
#include <cmath>

Vec3 Rasterizer::Barycentric(Vec2 A, Vec2 B, Vec2 C, Vec2 P) {
    // Edge function approach
    float denom = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
    if (std::abs(denom) < 1e-7f) return { -1, 1, 1 };  // degenerate
    float l0 = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / denom;
    float l1 = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / denom;
    float l2 = 1.0f - l0 - l1;
    return { l0, l1, l2 };
}

void Rasterizer::DrawTriangle(VertexOut v0, VertexOut v1, VertexOut v2, IShader& shader) {
    int W = m_fb.Width(), H = m_fb.Height();

    // Simple near-plane clip: discard triangle if any vertex behind camera
    if (!ClipW(v0.clipPos) || !ClipW(v1.clipPos) || !ClipW(v2.clipPos)) return;

    // Perspective divide → NDC [-1,1]
    Vec3 ndc0 = v0.clipPos.PerspectiveDivide();
    Vec3 ndc1 = v1.clipPos.PerspectiveDivide();
    Vec3 ndc2 = v2.clipPos.PerspectiveDivide();

    // NDC → screen space (y flipped: NDC +y = up, screen +y = down)
    auto toScreen = [&](Vec3 ndc) -> Vec2 {
        return { (ndc.x + 1.0f) * 0.5f * (W - 1),
                 (1.0f - ndc.y) * 0.5f * (H - 1) };
    };

    Vec2 s0 = toScreen(ndc0);
    Vec2 s1 = toScreen(ndc1);
    Vec2 s2 = toScreen(ndc2);

    // Back-face culling: signed area < 0 → back-face
    float area = (s1.x - s0.x) * (s2.y - s0.y) - (s1.y - s0.y) * (s2.x - s0.x);
    if (area <= 0.0f) return;

    // Bounding box clamped to screen
    int minX = (int)std::max(0.0f,       std::floor(std::min({ s0.x, s1.x, s2.x })));
    int maxX = (int)std::min((float)(W-1), std::ceil( std::max({ s0.x, s1.x, s2.x })));
    int minY = (int)std::max(0.0f,       std::floor(std::min({ s0.y, s1.y, s2.y })));
    int maxY = (int)std::min((float)(H-1), std::ceil( std::max({ s0.y, s1.y, s2.y })));

    // Precompute 1/w for perspective-correct interpolation
    float rw0 = 1.0f / v0.clipPos.w;
    float rw1 = 1.0f / v1.clipPos.w;
    float rw2 = 1.0f / v2.clipPos.w;

    for (int py = minY; py <= maxY; ++py) {
        for (int px = minX; px <= maxX; ++px) {
            Vec2 P = { (float)px, (float)py };
            Vec3 bc = Barycentric(s0, s1, s2, P);

            if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;

            // Perspective-correct weight
            float wc0 = bc.x * rw0;
            float wc1 = bc.y * rw1;
            float wc2 = bc.z * rw2;
            float wSum = wc0 + wc1 + wc2;
            if (wSum < 1e-9f) continue;
            wc0 /= wSum; wc1 /= wSum; wc2 /= wSum;

            // Interpolate depth (NDC z)
            float depth = ndc0.z * wc0 + ndc1.z * wc1 + ndc2.z * wc2;
            if (!m_fb.TestAndSetDepth(px, py, depth)) continue;

            // Interpolate varyings
            Varying frag;
            auto lerpV3 = [&](Vec3 a, Vec3 b, Vec3 c) {
                return Vec3{
                    a.x*wc0 + b.x*wc1 + c.x*wc2,
                    a.y*wc0 + b.y*wc1 + c.y*wc2,
                    a.z*wc0 + b.z*wc1 + c.z*wc2
                };
            };
            frag.worldPos = lerpV3(v0.varying.worldPos, v1.varying.worldPos, v2.varying.worldPos);
            frag.normal   = lerpV3(v0.varying.normal,   v1.varying.normal,   v2.varying.normal).normalized();
            frag.uv.x     = v0.varying.uv.x * wc0 + v1.varying.uv.x * wc1 + v2.varying.uv.x * wc2;
            frag.uv.y     = v0.varying.uv.y * wc0 + v1.varying.uv.y * wc1 + v2.varying.uv.y * wc2;
            frag.color.r  = (uint8_t)(v0.varying.color.r * wc0 + v1.varying.color.r * wc1 + v2.varying.color.r * wc2);
            frag.color.g  = (uint8_t)(v0.varying.color.g * wc0 + v1.varying.color.g * wc1 + v2.varying.color.g * wc2);
            frag.color.b  = (uint8_t)(v0.varying.color.b * wc0 + v1.varying.color.b * wc1 + v2.varying.color.b * wc2);

            Color outColor = shader.Fragment(frag);
            m_fb.SetPixel(px, py, outColor);
        }
    }
}
