#pragma once
#include <vector>
#include <cstdint>
#include <limits>

struct Color {
    uint8_t r, g, b, a;

    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

    // From normalized [0,1] floats
    static Color FromFloat(float r, float g, float b, float a = 1.0f); // [0,1] float → Color

    uint32_t ToARGB() const { // Win32 BI_RGB 포맷 전 중간 표현
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
    Framebuffer(int width, int height);

    int Width()  const { return m_width;  }
    int Height() const { return m_height; }

    void Clear(Color clearColor, float clearDepth = 1.0f);        // 색상/깊이 버퍼 초기화
    void SetPixel(int x, int y, Color color);                     // 픽셀 직접 기록 (깊이 테스트 없음)
    bool TestAndSetDepth(int x, int y, float depth);              // 깊이 테스트 통과 시 기록 후 true 반환

    const uint32_t* ColorData() const { return m_color.data(); }

private:
    int m_width, m_height;
    std::vector<uint32_t> m_color;
    std::vector<float>    m_depth;
};
