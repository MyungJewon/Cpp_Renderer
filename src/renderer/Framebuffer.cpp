#include "Framebuffer.h"
#include <numeric>

// 색상/깊이/MSAA 샘플 버퍼를 화면 해상도와 샘플 수에 맞게 할당
Framebuffer::Framebuffer(int width, int height, int sampleCount)
    : m_width(width), m_height(height)
    , m_sampleCount(sampleCount < 1 ? 1 : sampleCount)
    , m_color(width * height, 0)
    , m_depth(width * height, 1.0f)
    , m_msaaDepth(width * height * m_sampleCount, 1.0f)
    , m_msaaColor(width * height * m_sampleCount, 0)
{}

// 매 프레임 시작 전 색상·깊이 버퍼를 초기값으로 리셋
void Framebuffer::Clear(Color clearColor, float clearDepth) {
    uint32_t packed = clearColor.ToARGB();
    std::fill(m_color.begin(),     m_color.end(),     packed);
    std::fill(m_depth.begin(),     m_depth.end(),     clearDepth);
    std::fill(m_msaaDepth.begin(), m_msaaDepth.end(), clearDepth);
    std::fill(m_msaaColor.begin(), m_msaaColor.end(), packed);
}

// OIT Compose처럼 깊이 테스트 없이 최종 색상 버퍼에 직접 기록
void Framebuffer::SetPixel(int x, int y, Color color) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;
    m_color[y * m_width + x] = color.ToARGB();
}

// 단일 샘플 경로의 Z-buffer 테스트: 기존보다 앞에 있을 때만 기록
bool Framebuffer::TestAndSetDepth(int x, int y, float depth) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return false;
    int idx = y * m_width + x;
    if (depth < m_depth[idx]) {
        m_depth[idx] = depth;
        return true;
    }
    return false;
}

// MSAA 경로의 서브샘플 단위 Z-buffer 테스트 및 색상 기록
bool Framebuffer::TestAndSetMSAA(int x, int y, int sample, float depth, Color color) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return false;
    if (sample < 0 || sample >= m_sampleCount) return false;
    int idx = (y * m_width + x) * m_sampleCount + sample;
    if (depth < m_msaaDepth[idx]) {
        m_msaaDepth[idx] = depth;
        m_msaaColor[idx] = color.ToARGB();
        return true;
    }
    return false;
}

// Present 전 호출: N개 서브샘플 색상을 평균해 최종 출력 버퍼로 다운샘플
void Framebuffer::Resolve() {
    for (int i = 0; i < m_width * m_height; ++i) {
        uint32_t r = 0, g = 0, b = 0, a = 0;
        for (int s = 0; s < m_sampleCount; ++s) {
            uint32_t c = m_msaaColor[i * m_sampleCount + s];
            a += (c >> 24) & 0xFF;
            r += (c >> 16) & 0xFF;
            g += (c >>  8) & 0xFF;
            b += (c      ) & 0xFF;
        }
        r /= m_sampleCount; g /= m_sampleCount;
        b /= m_sampleCount; a /= m_sampleCount;
        m_color[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }
}
