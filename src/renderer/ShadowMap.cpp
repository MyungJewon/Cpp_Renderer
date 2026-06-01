#include "ShadowMap.h"

// 광원 시점 깊이 버퍼를 할당하고 초기 깊이를 1.0(가장 먼 값)으로 설정
ShadowMap::ShadowMap(int width, int height)
    : m_width(width), m_height(height)
    , m_depth(width * height, 1.0f)
    , m_lightVP(Mat4::Identity())
{}

// 매 프레임 Shadow Pass 전에 깊이 버퍼를 초기화
void ShadowMap::Clear() {
    std::fill(m_depth.begin(), m_depth.end(), 1.0f);
}

// Shadow Pass: 광원 시점에서 가장 앞에 있는 깊이값만 기록
bool ShadowMap::TestAndSet(int x, int y, float depth) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return false;
    int idx = y * m_width + x;
    if (depth < m_depth[idx]) {
        m_depth[idx] = depth;
        return true;
    }
    return false;
}

// Main Pass: 특정 텍셀에 기록된 광원 깊이값을 읽어 그림자 판별에 사용
float ShadowMap::Sample(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return 1.0f;
    return m_depth[y * m_width + x];
}

// 월드 좌표를 광원 VP 행렬로 변환해 NDC 좌표를 반환
// Fragment에서 그림자 맵 텍셀 위치와 현재 깊이 비교에 사용
Vec3 ShadowMap::WorldToLightNDC(const Vec3& worldPos) const {
    Vec4 clip = m_lightVP * Vec4(worldPos, 1.0f);
    if (clip.w <= 0.0f) return { 0, 0, 1 };
    return clip.PerspectiveDivide();
}
