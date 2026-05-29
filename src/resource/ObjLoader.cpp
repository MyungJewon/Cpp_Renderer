#include "ObjLoader.h"
#include <fstream>
#include <sstream>
#include <map>
#include <tuple>
#include <algorithm>
#include <cfloat>

Mesh ObjLoader::Load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};

    // OBJ는 위치/UV/법선 인덱스가 각각 따로 존재
    // → 조합(pos_idx, uv_idx, normal_idx)을 키로 정점을 통합
    std::vector<Vec3> positions;
    std::vector<Vec2> uvs;
    std::vector<Vec3> normals;

    Mesh mesh;
    std::map<std::tuple<int,int,int>, int> indexCache;

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
            // f 토큰은 "v", "v/vt", "v//vn", "v/vt/vn" 형식 모두 처리
            std::vector<int> faceIndices;
            std::string part;
            while (ss >> part) {
                std::replace(part.begin(), part.end(), '/', ' ');
                std::istringstream ps(part);
                int pi = -1, ui = -1, ni = -1;
                ps >> pi; pi--;          // OBJ 인덱스는 1-based
                if (ps >> ui) { ui--; } else { ui = -1; }
                if (ps >> ni) { ni--; } else { ni = -1; }

                faceIndices.push_back(getOrAdd(pi, ui, ni));
            }

            // 폴리곤을 삼각형으로 분할 (fan triangulation)
            for (int i = 1; i + 1 < (int)faceIndices.size(); ++i) {
                mesh.indices.push_back(faceIndices[0]);
                mesh.indices.push_back(faceIndices[i]);
                mesh.indices.push_back(faceIndices[i + 1]);
            }
        }
    }

    // 법선이 없는 메시는 면 법선을 직접 계산
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

    return mesh;
}

void ObjLoader::Normalize(Mesh& mesh) {
    if (mesh.vertices.empty()) return;

    // AABB 계산
    Vec3 mn = { FLT_MAX,  FLT_MAX,  FLT_MAX };
    Vec3 mx = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    for (auto& v : mesh.vertices) {
        mn.x = std::min(mn.x, v.pos.x); mx.x = std::max(mx.x, v.pos.x);
        mn.y = std::min(mn.y, v.pos.y); mx.y = std::max(mx.y, v.pos.y);
        mn.z = std::min(mn.z, v.pos.z); mx.z = std::max(mx.z, v.pos.z);
    }

    Vec3  center = (mn + mx) * 0.5f;
    float extent = std::max({ mx.x - mn.x, mx.y - mn.y, mx.z - mn.z });
    float scale  = (extent > 1e-6f) ? 2.0f / extent : 1.0f;  // [-1, 1] 범위

    for (auto& v : mesh.vertices) {
        v.pos = (v.pos - center) * scale;
    }
}
