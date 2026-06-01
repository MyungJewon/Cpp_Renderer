#pragma once
#include <vector>
#include <cstdint>

struct Color {
    uint8_t r, g, b, a;

    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

    static Color FromFloat(float r, float g, float b, float a = 1.0f); // [0,1] float → Color

    uint32_t ToARGB() const { // ARGB 패킹 (Win32/CGImage 출력 전 단계)
        return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
};

inline Color Color::FromFloat(float r, float g, float b, float a) {
    auto c = [](float v) -> uint8_t {
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
        return (uint8_t)(v * 255.0f);
    };
    return { c(r), c(g), c(b), c(a) };
}

class Framebuffer {
public:
    Framebuffer(int width, int height, int sampleCount = 1);

    int Width()       const { return m_width;       }
    int Height()      const { return m_height;      }
    int SampleCount() const { return m_sampleCount; }

    void Clear(Color clearColor, float clearDepth = 1.0f); // 색상/깊이 버퍼 전체 초기화
    void SetPixel(int x, int y, Color color);              // 픽셀 직접 기록 (깊이 테스트 없음)
    bool TestAndSetDepth(int x, int y, float depth);       // 깊이 테스트 통과 시 기록 후 true 반환

    // MSAA: 서브샘플 단위 깊이 테스트 및 색상 기록
    bool TestAndSetMSAA(int x, int y, int sample, float depth, Color color);
    // MSAA: 서브샘플 평균 → 최종 색상 버퍼로 다운샘플
    void Resolve();

    const uint32_t* ColorData() const { return m_color.data(); }

private:
    int m_width, m_height, m_sampleCount;
    std::vector<uint32_t> m_color;       // 최종 출력 버퍼 (W×H)
    std::vector<float>    m_depth;       // 단일 샘플용 깊이 (W×H)
    std::vector<float>    m_msaaDepth;   // MSAA 서브샘플 깊이 (W×H×N)
    std::vector<uint32_t> m_msaaColor;   // MSAA 서브샘플 색상 (W×H×N)
};
