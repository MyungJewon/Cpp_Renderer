#include "Texture.h"
#include <fstream>
#include <cstring>
#include <algorithm>

#pragma pack(push, 1)
struct TGAHeader {
    uint8_t  idLength;
    uint8_t  colorMapType;
    uint8_t  imageType;
    uint8_t  colorMapSpec[5];
    uint16_t originX;
    uint16_t originY;
    uint16_t width;
    uint16_t height;
    uint8_t  bitsPerPixel;
    uint8_t  imageDesc;
};
#pragma pack(pop)

// TGA 파일을 직접 파싱해 픽셀 배열로 적재
// 외부 라이브러리 없이 구현; uncompressed RGB/RGBA(type 2, 24/32bit)만 지원
// TGA는 BGR 바이트 순서 및 기본 bottom-up 저장이므로 읽을 때 보정
Texture Texture::Load(const std::string& path) {
    Texture tex;
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return tex;

    TGAHeader hdr;
    f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (!f) return tex;

    f.seekg(hdr.idLength, std::ios::cur);

    if (hdr.imageType != 2 && hdr.imageType != 3) return tex;
    if (hdr.bitsPerPixel != 24 && hdr.bitsPerPixel != 32) return tex;

    int w = hdr.width, h = hdr.height;
    int bytesPerPixel = hdr.bitsPerPixel / 8;
    tex.m_width  = w;
    tex.m_height = h;
    tex.m_pixels.resize(w * h);

    std::vector<uint8_t> raw(w * h * bytesPerPixel);
    f.read(reinterpret_cast<char*>(raw.data()), raw.size());

    bool topDown = (hdr.imageDesc & 0x20) != 0;

    for (int y = 0; y < h; ++y) {
        int srcRow = topDown ? y : (h - 1 - y);
        for (int x = 0; x < w; ++x) {
            int     idx = (srcRow * w + x) * bytesPerPixel;
            uint8_t b   = raw[idx + 0];
            uint8_t g   = raw[idx + 1];
            uint8_t r   = raw[idx + 2];
            uint8_t a   = (bytesPerPixel == 4) ? raw[idx + 3] : 255;
            tex.m_pixels[y * w + x] = Color(r, g, b, a);
        }
    }

    return tex;
}

// UV [0,1] 좌표로 픽셀 색상을 샘플링
// UV가 범위를 벗어나면 repeat 처리, 2x2 블록 보간으로 계단 현상 완화 (Bilinear filtering)
Color Texture::Sample(float u, float v) const {
    if (m_pixels.empty()) return Color(255, 0, 255);

    u = u - std::floor(u);
    v = v - std::floor(v);

    float fx = u * (m_width  - 1);
    float fy = v * (m_height - 1);
    int   x0 = (int)fx, y0 = (int)fy;
    int   x1 = std::min(x0 + 1, m_width  - 1);
    int   y1 = std::min(y0 + 1, m_height - 1);
    float tx = fx - x0, ty = fy - y0;

    // 픽셀 좌표로 직접 접근하는 헬퍼
    auto get = [&](int x, int y) -> Color {
        return m_pixels[y * m_width + x];
    };

    // 가로 방향 선형 보간 후 세로 방향 선형 보간
    auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
    auto lerpC = [&](Color a, Color b, float t) -> Color {
        return Color(
            (uint8_t)lerp(a.r, b.r, t),
            (uint8_t)lerp(a.g, b.g, t),
            (uint8_t)lerp(a.b, b.b, t)
        );
    };

    Color c00 = get(x0, y0), c10 = get(x1, y0);
    Color c01 = get(x0, y1), c11 = get(x1, y1);
    return lerpC(lerpC(c00, c10, tx), lerpC(c01, c11, tx), ty);
}
