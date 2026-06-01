#pragma once
#include "../renderer/Framebuffer.h"
#include "../math/Vec2.h"
#include <vector>
#include <string>

class Texture {
public:
    // TGA 파일 로드 (uncompressed RGB/RGBA, type 2만 지원)
    static Texture Load(const std::string& path);

    bool IsValid() const { return !m_pixels.empty(); }

    // UV [0,1] 범위로 색상 샘플링 (Bilinear filtering)
    Color Sample(float u, float v) const;

private:
    std::vector<Color> m_pixels;
    int m_width  = 0;
    int m_height = 0;
};
