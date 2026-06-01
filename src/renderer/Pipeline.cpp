#include "Pipeline.h"

// 인덱스 배열을 3개씩 묶어 삼각형으로 변환하고 래스터라이저에 전달
// 셰이더의 Vertex()를 정점마다 호출해 클립 공간으로 변환한 뒤 DrawTriangle로 넘김
void Pipeline::DrawIndexed(IShader& shader, const std::vector<int>& indices) {
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        VertexOut v0 = shader.Vertex(indices[i    ]);
        VertexOut v1 = shader.Vertex(indices[i + 1]);
        VertexOut v2 = shader.Vertex(indices[i + 2]);
        m_rasterizer.DrawTriangle(v0, v1, v2, shader);
    }
}
