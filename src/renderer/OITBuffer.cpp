#include "OITBuffer.h"
#include <algorithm>

// 픽셀당 프래그먼트 리스트를 담을 버퍼를 화면 해상도에 맞게 할당
OITBuffer::OITBuffer(int width, int height)
    : m_width(width), m_height(height)
    , m_pixels(width * height)
{}

// 투명 패스 시작 전 모든 픽셀의 프래그먼트 카운트를 0으로 리셋
void OITBuffer::Clear() {
    for (auto& p : m_pixels) p.count = 0;
}

// 투명 프래그먼트를 해당 픽셀의 리스트에 추가
// 버퍼가 가득 찬 픽셀은 추가 프래그먼트를 무시 (최대 OIT_MAX_FRAGS개)
void OITBuffer::AddFragment(int x, int y, float depth, Color color, float alpha) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;
    auto& px = m_pixels[y * m_width + x];
    if (px.count >= OIT_MAX_FRAGS) return;
    px.frags[px.count++] = { depth, color, alpha };
}

// 불투명 패스 결과 위에 투명 프래그먼트를 합성
// 각 픽셀에서 깊이 기준 뒤→앞 정렬 후 Porter-Duff over 연산으로 블렌딩
void OITBuffer::Compose(Framebuffer& fb) {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            auto& px = m_pixels[y * m_width + x];
            if (px.count == 0) continue;

            // 뒤쪽 프래그먼트부터 앞쪽 순서로 over 블렌딩하기 위해 depth 내림차순 정렬
            std::sort(px.frags, px.frags + px.count,
                [](const OITFragment& a, const OITFragment& b) {
                    return a.depth > b.depth;
                });

            // 불투명 버퍼의 색상을 초기 배경으로 사용
            uint32_t base = fb.ColorData()[y * fb.Width() + x];
            float r = ((base >> 16) & 0xFF) / 255.0f;
            float g = ((base >>  8) & 0xFF) / 255.0f;
            float b = ((base      ) & 0xFF) / 255.0f;

            // over 합성: dst = src * alpha + dst * (1 - alpha)
            for (int i = 0; i < px.count; ++i) {
                float a  = px.frags[i].alpha;
                float sr = px.frags[i].color.r / 255.0f;
                float sg = px.frags[i].color.g / 255.0f;
                float sb = px.frags[i].color.b / 255.0f;
                r = sr * a + r * (1.0f - a);
                g = sg * a + g * (1.0f - a);
                b = sb * a + b * (1.0f - a);
            }

            fb.SetPixel(x, y, Color::FromFloat(r, g, b));
        }
    }
}
