#include "Rasterizer.h"
#include "../math/MathUtils.h"
#include <algorithm>
#include <cmath>

static const Vec2 kSamples1[1] = { {0.0f, 0.0f} };
static const Vec2 kSamples2[2] = { {-0.25f,-0.25f}, { 0.25f, 0.25f} };
static const Vec2 kSamples4[4] = { {-0.375f, 0.125f}, { 0.125f, 0.375f},
                                   { 0.375f,-0.125f}, {-0.125f,-0.375f} };

// 샘플 수에 따라 픽셀 내 서브샘플 오프셋 배열 반환 (Rotated Grid 패턴으로 수평 줄 무늬 억제)
const Vec2* Rasterizer::SubSampleOffsets(int count) {
    if (count >= 4) return kSamples4;
    if (count == 2) return kSamples2;
    return kSamples1;
}

// Edge function으로 점 P가 삼각형 (A,B,C) 내에 있는지 판별하는 무게중심 좌표 계산
// 반환값 중 음수 성분이 있으면 삼각형 외부
Vec3 Rasterizer::Barycentric(Vec2 A, Vec2 B, Vec2 C, Vec2 P) {
    float denom = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
    if (std::abs(denom) < 1e-7f) return { -1, 1, 1 };
    float l0 = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / denom;
    float l1 = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / denom;
    float l2 = 1.0f - l0 - l1;
    return { l0, l1, l2 };
}

// 클립 공간 정점 3개를 받아 삼각형을 래스터화하고 프래그먼트 셰이더를 호출
// 멀티샘플링 활성 시 서브샘플 커버리지 테스트 후 픽셀 중심에서 셰이딩을 한 번만 수행
void Rasterizer::DrawTriangle(VertexOut v0, VertexOut v1, VertexOut v2, IShader& shader) {
    int W = m_fb.Width(), H = m_fb.Height();
    int N = m_fb.SampleCount();
    bool msaa = (N > 1);

    // near plane 뒤에 있는 정점이 포함된 삼각형은 드로우 제외
    if (!ClipW(v0.clipPos) || !ClipW(v1.clipPos) || !ClipW(v2.clipPos)) return;

    Vec3 ndc0 = v0.clipPos.PerspectiveDivide();
    Vec3 ndc1 = v1.clipPos.PerspectiveDivide();
    Vec3 ndc2 = v2.clipPos.PerspectiveDivide();

    // NDC → 스크린 좌표 변환 (y 축 반전: NDC +y = 위, 스크린 +y = 아래)
    auto toScreen = [&](Vec3 ndc) -> Vec2 {
        return { (ndc.x + 1.0f) * 0.5f * (W - 1),
                 (1.0f - ndc.y) * 0.5f * (H - 1) };
    };

    Vec2 s0 = toScreen(ndc0), s1 = toScreen(ndc1), s2 = toScreen(ndc2);

    // 부호 있는 넓이로 뒷면 컬링 (y-down 스크린 공간에서 앞면은 음수 넓이)
    float area = (s1.x - s0.x) * (s2.y - s0.y) - (s1.y - s0.y) * (s2.x - s0.x);
    if (area >= 0.0f) return;

    // 삼각형의 화면상 바운딩 박스로 순회 범위 제한
    int minX = (int)std::max(0.0f,       std::floor(std::min({ s0.x, s1.x, s2.x })));
    int maxX = (int)std::min((float)(W-1), std::ceil( std::max({ s0.x, s1.x, s2.x })));
    int minY = (int)std::max(0.0f,       std::floor(std::min({ s0.y, s1.y, s2.y })));
    int maxY = (int)std::min((float)(H-1), std::ceil( std::max({ s0.y, s1.y, s2.y })));

    // 퍼스펙티브 보정 보간을 위해 클립 w의 역수를 미리 계산
    float rw0 = 1.0f / v0.clipPos.w;
    float rw1 = 1.0f / v1.clipPos.w;
    float rw2 = 1.0f / v2.clipPos.w;

    const Vec2* offsets = SubSampleOffsets(N);

    for (int py = minY; py <= maxY; ++py) {
        for (int px = minX; px <= maxX; ++px) {

            if (msaa) {
                // 각 서브샘플 위치에서 커버리지를 검사하고 깊이 기록
                // 셰이딩은 처음 커버된 샘플에서 픽셀 중심 기준으로 한 번만 실행
                bool  anyCovered = false;
                Color centerColor(0, 0, 0);

                for (int s = 0; s < N; ++s) {
                    Vec2 P = { px + offsets[s].x, py + offsets[s].y };
                    Vec3 bc = Barycentric(s0, s1, s2, P);
                    if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;

                    float wc0 = bc.x*rw0, wc1 = bc.y*rw1, wc2 = bc.z*rw2;
                    float wSum = wc0 + wc1 + wc2;
                    if (wSum < 1e-9f) continue;
                    wc0 /= wSum; wc1 /= wSum; wc2 /= wSum;
                    float depth = ndc0.z*wc0 + ndc1.z*wc1 + ndc2.z*wc2;

                    if (!anyCovered) {
                        // 픽셀 중심에서 Varying 보간 후 Fragment 셰이더 실행
                        Vec3 bc2 = Barycentric(s0, s1, s2, { (float)px, (float)py });
                        float c0 = bc2.x*rw0, c1 = bc2.y*rw1, c2 = bc2.z*rw2;
                        float cs = c0+c1+c2; if (cs < 1e-9f) cs = 1e-9f;
                        c0/=cs; c1/=cs; c2/=cs;
                        auto lerpV3 = [&](Vec3 a, Vec3 b, Vec3 c) {
                            return Vec3{a.x*c0+b.x*c1+c.x*c2, a.y*c0+b.y*c1+c.y*c2, a.z*c0+b.z*c1+c.z*c2};
                        };
                        Varying frag;
                        frag.worldPos = lerpV3(v0.varying.worldPos, v1.varying.worldPos, v2.varying.worldPos);
                        frag.normal   = lerpV3(v0.varying.normal,   v1.varying.normal,   v2.varying.normal).normalized();
                        frag.tangent  = lerpV3(v0.varying.tangent,  v1.varying.tangent,  v2.varying.tangent).normalized();
                        frag.uv.x     = v0.varying.uv.x*c0 + v1.varying.uv.x*c1 + v2.varying.uv.x*c2;
                        frag.uv.y     = v0.varying.uv.y*c0 + v1.varying.uv.y*c1 + v2.varying.uv.y*c2;
                        centerColor = shader.Fragment(frag);
                        anyCovered = true;
                    }
                    m_fb.TestAndSetMSAA(px, py, s, depth, centerColor);
                }
            } else {
                // 단일 샘플: 픽셀 중심에서 커버리지·깊이 테스트 후 셰이딩
                Vec3 bc = Barycentric(s0, s1, s2, { (float)px, (float)py });
                if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;

                float wc0 = bc.x*rw0, wc1 = bc.y*rw1, wc2 = bc.z*rw2;
                float wSum = wc0+wc1+wc2;
                if (wSum < 1e-9f) continue;
                wc0/=wSum; wc1/=wSum; wc2/=wSum;

                float depth = ndc0.z*wc0 + ndc1.z*wc1 + ndc2.z*wc2;
                if (!m_fb.TestAndSetDepth(px, py, depth)) continue;

                // 퍼스펙티브 보정된 무게중심 좌표로 모든 Varying을 보간
                Varying frag;
                auto lerpV3 = [&](Vec3 a, Vec3 b, Vec3 c) {
                    return Vec3{a.x*wc0+b.x*wc1+c.x*wc2, a.y*wc0+b.y*wc1+c.y*wc2, a.z*wc0+b.z*wc1+c.z*wc2};
                };
                frag.worldPos = lerpV3(v0.varying.worldPos, v1.varying.worldPos, v2.varying.worldPos);
                frag.normal   = lerpV3(v0.varying.normal,   v1.varying.normal,   v2.varying.normal).normalized();
                frag.tangent  = lerpV3(v0.varying.tangent,  v1.varying.tangent,  v2.varying.tangent).normalized();
                frag.uv.x     = v0.varying.uv.x*wc0 + v1.varying.uv.x*wc1 + v2.varying.uv.x*wc2;
                frag.uv.y     = v0.varying.uv.y*wc0 + v1.varying.uv.y*wc1 + v2.varying.uv.y*wc2;
                frag.color.r  = (uint8_t)(v0.varying.color.r*wc0 + v1.varying.color.r*wc1 + v2.varying.color.r*wc2);
                frag.color.g  = (uint8_t)(v0.varying.color.g*wc0 + v1.varying.color.g*wc1 + v2.varying.color.g*wc2);
                frag.color.b  = (uint8_t)(v0.varying.color.b*wc0 + v1.varying.color.b*wc1 + v2.varying.color.b*wc2);

                m_fb.SetPixel(px, py, shader.Fragment(frag));
            }
        }
    }
}
