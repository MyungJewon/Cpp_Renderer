#pragma once
#include "Framebuffer.h"
#include "Rasterizer.h"
#include "Shader.h"
#include <vector>

class Pipeline {
public:
    Pipeline(Framebuffer& fb) : m_rasterizer(fb) {}

    void DrawIndexed(IShader& shader, const std::vector<int>& indices); // 인덱스 3개씩 묶어 삼각형 드로우

private:
    Rasterizer m_rasterizer;
};
