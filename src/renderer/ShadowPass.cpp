#include "ShadowPass.h"
#include <algorithm>
#include <cmath>

// Rasterizer::Barycentric과 동일한 구현
// Shadow Pass는 색상이 필요 없어서 별도 경량 버전으로 분리
Vec3 ShadowPassRenderer::Barycentric(Vec2 A, Vec2 B, Vec2 C, Vec2 P) {
    float denom = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
    if (std::abs(denom) < 1e-7f) return { -1, 1, 1 };
    float l0 = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / denom;
    float l1 = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / denom;
    return { l0, l1, 1.0f - l0 - l1 };
}

// 광원 시점으로 씬 전체를 렌더링해 ShadowMap의 깊이 버퍼를 채움
// 색상 연산 없이 깊이만 기록하므로 Fragment 셰이더는 호출하지 않음
void ShadowPassRenderer::Render(ShadowShader& shader, const std::vector<int>& indices) {
    int W = m_sm.Width(), H = m_sm.Height();

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        Vec4 c0 = shader.Vertex(indices[i    ]).clipPos;
        Vec4 c1 = shader.Vertex(indices[i + 1]).clipPos;
        Vec4 c2 = shader.Vertex(indices[i + 2]).clipPos;

        if (c0.w <= 0 || c1.w <= 0 || c2.w <= 0) continue;

        // 클립 좌표를 광원 시점의 스크린 좌표로 변환
        auto toScreen = [&](Vec4 c) -> Vec2 {
            Vec3 ndc = c.PerspectiveDivide();
            return { (ndc.x + 1.0f) * 0.5f * (W - 1),
                     (1.0f - ndc.y) * 0.5f * (H - 1) };
        };

        Vec3 ndc0 = c0.PerspectiveDivide();
        Vec3 ndc1 = c1.PerspectiveDivide();
        Vec3 ndc2 = c2.PerspectiveDivide();
        Vec2 s0 = toScreen(c0), s1 = toScreen(c1), s2 = toScreen(c2);

        int minX = (int)std::max(0.0f,       std::floor(std::min({ s0.x, s1.x, s2.x })));
        int maxX = (int)std::min((float)(W-1), std::ceil( std::max({ s0.x, s1.x, s2.x })));
        int minY = (int)std::max(0.0f,       std::floor(std::min({ s0.y, s1.y, s2.y })));
        int maxY = (int)std::min((float)(H-1), std::ceil( std::max({ s0.y, s1.y, s2.y })));

        // 바운딩 박스 내 픽셀마다 삼각형 내부 판별 후 깊이만 기록
        for (int py = minY; py <= maxY; ++py) {
            for (int px = minX; px <= maxX; ++px) {
                Vec3 bc = Barycentric(s0, s1, s2, { (float)px, (float)py });
                if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;

                float depth = ndc0.z * bc.x + ndc1.z * bc.y + ndc2.z * bc.z;
                m_sm.TestAndSet(px, py, depth);
            }
        }
    }
}
