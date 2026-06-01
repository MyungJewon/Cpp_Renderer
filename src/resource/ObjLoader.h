#pragma once
#include "../math/Vec2.h"
#include "../math/Vec3.h"
#include <vector>
#include <string>

struct MeshVertex {
    Vec3 pos;
    Vec2 uv;
    Vec3 normal;
    Vec3 tangent;  // TBN 행렬 구성용, ObjLoader가 자동 계산
};

struct Mesh {
    std::vector<MeshVertex> vertices;
    std::vector<int>        indices;
};

class ObjLoader {
public:
    // .obj 파일을 읽어 Mesh 하나로 반환, 실패 시 빈 Mesh
    static Mesh Load(const std::string& path);

    // 모델을 원점 중심, [-1,1] 범위로 정규화
    static void Normalize(Mesh& mesh);
};
