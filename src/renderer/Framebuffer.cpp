#include "Framebuffer.h"

Framebuffer::Framebuffer(int width, int height)
    : m_width(width), m_height(height)
    , m_color(width * height, 0)
    , m_depth(width * height, 1.0f)
{}

void Framebuffer::Clear(Color clearColor, float clearDepth) {
    uint32_t packed = clearColor.ToARGB();
    for (int i = 0; i < m_width * m_height; ++i) {
        m_color[i] = packed;
        m_depth[i] = clearDepth;
    }
}

void Framebuffer::SetPixel(int x, int y, Color color) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;
    m_color[y * m_width + x] = color.ToARGB();
}

bool Framebuffer::TestAndSetDepth(int x, int y, float depth) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return false;
    int idx = y * m_width + x;
    if (depth < m_depth[idx]) {
        m_depth[idx] = depth;
        return true;
    }
    return false;
}
