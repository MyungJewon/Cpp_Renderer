#include "Pipeline.h"

void Pipeline::DrawIndexed(IShader& shader, const std::vector<int>& indices) {
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        VertexOut v0 = shader.Vertex(indices[i    ]);
        VertexOut v1 = shader.Vertex(indices[i + 1]);
        VertexOut v2 = shader.Vertex(indices[i + 2]);
        m_rasterizer.DrawTriangle(v0, v1, v2, shader);
    }
}
