#include "ObjLoader.h"
#include <fstream>
#include <sstream>
#include <map>
#include <tuple>
#include <algorithm>
#include <cfloat>

// .obj 파일을 파싱해 단일 Mesh로 반환
// OBJ 포맷은 위치/UV/법선 인덱스가 각각 독립적이므로 조합을 키로 정점을 통합
// 파일에 법선이 없으면 면 법선으로 자동 계산, 탄젠트는 항상 UV 그라디언트로 계산
Mesh ObjLoader::Load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};

    std::vector<Vec3> positions;
    std::vector<Vec2> uvs;
    std::vector<Vec3> normals;

    Mesh mesh;
    std::map<std::tuple<int,int,int>, int> indexCache;

    // (pos, uv, normal) 인덱스 조합이 처음 나오면 정점을 추가하고, 이미 있으면 기존 인덱스 반환
    auto getOrAdd = [&](int pi, int ui, int ni) -> int {
        auto key = std::make_tuple(pi, ui, ni);
        auto it  = indexCache.find(key);
        if (it != indexCache.end()) return it->second;

        MeshVertex v;
        v.pos    = (pi >= 0) ? positions[pi] : Vec3{};
        v.uv     = (ui >= 0) ? uvs[ui]       : Vec2{};
        v.normal = (ni >= 0) ? normals[ni]    : Vec3{};

        int idx = (int)mesh.vertices.size();
        mesh.vertices.push_back(v);
        indexCache[key] = idx;
        return idx;
    };

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "v") {
            Vec3 p; ss >> p.x >> p.y >> p.z;
            positions.push_back(p);

        } else if (token == "vt") {
            Vec2 uv; ss >> uv.x >> uv.y;
            uvs.push_back(uv);

        } else if (token == "vn") {
            Vec3 n; ss >> n.x >> n.y >> n.z;
            normals.push_back(n);

        } else if (token == "f") {
            // "v", "v/vt", "v//vn", "v/vt/vn" 형식 모두 지원
            std::vector<int> faceIndices;
            std::string part;
            while (ss >> part) {
                std::replace(part.begin(), part.end(), '/', ' ');
                std::istringstream ps(part);
                int pi = -1, ui = -1, ni = -1;
                ps >> pi; pi--;
                if (ps >> ui) { ui--; } else { ui = -1; }
                if (ps >> ni) { ni--; } else { ni = -1; }
                faceIndices.push_back(getOrAdd(pi, ui, ni));
            }

            // 쿼드 이상의 폴리곤은 fan triangulation으로 삼각형으로 분할
            for (int i = 1; i + 1 < (int)faceIndices.size(); ++i) {
                mesh.indices.push_back(faceIndices[0]);
                mesh.indices.push_back(faceIndices[i]);
                mesh.indices.push_back(faceIndices[i + 1]);
            }
        }
    }

    // OBJ에 법선 데이터가 없을 때 인접 면 법선의 평균으로 스무딩 법선을 계산
    if (normals.empty()) {
        std::vector<Vec3> accum(mesh.vertices.size(), Vec3{});
        for (int i = 0; i + 2 < (int)mesh.indices.size(); i += 3) {
            auto& v0 = mesh.vertices[mesh.indices[i    ]];
            auto& v1 = mesh.vertices[mesh.indices[i + 1]];
            auto& v2 = mesh.vertices[mesh.indices[i + 2]];
            Vec3 n = (v1.pos - v0.pos).cross(v2.pos - v0.pos).normalized();
            accum[mesh.indices[i    ]] += n;
            accum[mesh.indices[i + 1]] += n;
            accum[mesh.indices[i + 2]] += n;
        }
        for (int i = 0; i < (int)mesh.vertices.size(); ++i)
            mesh.vertices[i].normal = accum[i].normalized();
    }

    // UV 그라디언트를 이용해 탄젠트 벡터를 계산 (Normal Map의 TBN 행렬 구성에 필요)
    {
        std::vector<Vec3> accum(mesh.vertices.size(), Vec3{});
        for (int i = 0; i + 2 < (int)mesh.indices.size(); i += 3) {
            auto& v0 = mesh.vertices[mesh.indices[i    ]];
            auto& v1 = mesh.vertices[mesh.indices[i + 1]];
            auto& v2 = mesh.vertices[mesh.indices[i + 2]];

            Vec3  dP1 = v1.pos - v0.pos, dP2 = v2.pos - v0.pos;
            float du1 = v1.uv.x - v0.uv.x, dv1 = v1.uv.y - v0.uv.y;
            float du2 = v2.uv.x - v0.uv.x, dv2 = v2.uv.y - v0.uv.y;
            float det = du1 * dv2 - du2 * dv1;
            if (std::abs(det) < 1e-6f) continue;
            Vec3 T = (dP1 * dv2 - dP2 * dv1) * (1.0f / det);
            accum[mesh.indices[i    ]] += T;
            accum[mesh.indices[i + 1]] += T;
            accum[mesh.indices[i + 2]] += T;
        }
        for (int i = 0; i < (int)mesh.vertices.size(); ++i)
            mesh.vertices[i].tangent = accum[i].normalized();
    }

    return mesh;
}

// 모델의 AABB를 구해 원점 중심, [-1, 1] 범위로 스케일 조정
// 어떤 크기의 OBJ든 카메라 뷰에 맞게 자동으로 들어오게 하기 위해 사용
void ObjLoader::Normalize(Mesh& mesh) {
    if (mesh.vertices.empty()) return;

    Vec3 mn = { FLT_MAX,  FLT_MAX,  FLT_MAX };
    Vec3 mx = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    for (auto& v : mesh.vertices) {
        mn.x = std::min(mn.x, v.pos.x); mx.x = std::max(mx.x, v.pos.x);
        mn.y = std::min(mn.y, v.pos.y); mx.y = std::max(mx.y, v.pos.y);
        mn.z = std::min(mn.z, v.pos.z); mx.z = std::max(mx.z, v.pos.z);
    }

    Vec3  center = (mn + mx) * 0.5f;
    float extent = std::max({ mx.x - mn.x, mx.y - mn.y, mx.z - mn.z });
    float scale  = (extent > 1e-6f) ? 2.0f / extent : 1.0f;

    for (auto& v : mesh.vertices)
        v.pos = (v.pos - center) * scale;
}
