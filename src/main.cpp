#ifdef _WIN32
#include "window/Win32Window.h"
using AppWindow = Win32Window;
#else
#include "window/MacWindow.h"
using AppWindow = MacWindow;
#endif

#include "renderer/Framebuffer.h"
#include "renderer/Pipeline.h"
#include "renderer/Shader.h"
#include "resource/ObjLoader.h"
#include "scene/Camera.h"
#include "scene/Light.h"
#include "math/MathUtils.h"
#include <vector>
#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <mach-o/dyld.h>
#include <limits.h>
#include <libgen.h>
#endif

// 실행 파일이 있는 디렉토리 반환
static std::string GetExeDir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string p(buf);
    return p.substr(0, p.find_last_of("\\/"));
#else
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    _NSGetExecutablePath(buf, &size);
    return std::string(dirname(buf));
#endif
}

static const int W = 800, H = 600;

// Phong 라이팅 셰이더 (Ambient + Diffuse + Specular)
struct PhongShader : IShader {
    const Mesh* mesh     = nullptr;
    Mat4        mvp;
    Mat4        modelMat;
    Vec3        cameraPos;
    Light       light;

    VertexOut Vertex(int idx) override {
        const MeshVertex& v = mesh->vertices[idx];
        VertexOut out;
        out.clipPos          = mvp * Vec4(v.pos, 1.0f);
        out.varying.worldPos = (modelMat * Vec4(v.pos,    1.0f)).xyz();
        out.varying.normal   = (modelMat * Vec4(v.normal, 0.0f)).xyz().normalized();
        out.varying.uv       = v.uv;
        return out;
    }

    Color Fragment(const Varying& v) override {
        Vec3 N = v.normal.normalized();
        Vec3 L = (light.position - v.worldPos).normalized();  // 표면 → 광원
        Vec3 V = (cameraPos      - v.worldPos).normalized();  // 표면 → 카메라
        Vec3 R = (N * (N.dot(L) * 2.0f) - L).normalized();   // 반사 벡터

        float diff = std::max(0.0f, N.dot(L));
        float spec = std::pow(std::max(0.0f, R.dot(V)), light.shininess);

        Vec3 col = light.color * (light.ambient
                 + light.diffuse  * diff
                 + light.specular * spec);

        return Color::FromFloat(col.x, col.y, col.z);
    }
};

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#else
int main(int, char**) {
#endif
    AppWindow   window(W, H, "SoftRenderer — Week 3: Phong Lighting");
    Framebuffer fb(W, H);
    Pipeline    pipeline(fb);

    std::string objPath = GetExeDir() + "/assets/models/model.obj";
    printf("Loading: %s\n", objPath.c_str());
    Mesh mesh = ObjLoader::Load(objPath);
    if (!mesh.vertices.empty()) {
        ObjLoader::Normalize(mesh);  // 모델 크기/위치 정규화
        printf("Model loaded: %zu vertices, %zu triangles\n",
               mesh.vertices.size(), mesh.indices.size() / 3);
    } else {
        printf("model.obj not found — using fallback cube\n");
        // OBJ 없으면 하드코딩 큐브로 대체
        mesh.vertices = {
            // front
            MeshVertex{{ -1,-1, 1 }, {}, { 0, 0, 1}},
            MeshVertex{{  1,-1, 1 }, {}, { 0, 0, 1}},
            MeshVertex{{  1, 1, 1 }, {}, { 0, 0, 1}},
            MeshVertex{{ -1, 1, 1 }, {}, { 0, 0, 1}},
            // back
            MeshVertex{{  1,-1,-1 }, {}, { 0, 0,-1}},
            MeshVertex{{ -1,-1,-1 }, {}, { 0, 0,-1}},
            MeshVertex{{ -1, 1,-1 }, {}, { 0, 0,-1}},
            MeshVertex{{  1, 1,-1 }, {}, { 0, 0,-1}},
            // left
            MeshVertex{{ -1,-1,-1 }, {}, {-1, 0, 0}},
            MeshVertex{{ -1,-1, 1 }, {}, {-1, 0, 0}},
            MeshVertex{{ -1, 1, 1 }, {}, {-1, 0, 0}},
            MeshVertex{{ -1, 1,-1 }, {}, {-1, 0, 0}},
            // right
            MeshVertex{{  1,-1, 1 }, {}, { 1, 0, 0}},
            MeshVertex{{  1,-1,-1 }, {}, { 1, 0, 0}},
            MeshVertex{{  1, 1,-1 }, {}, { 1, 0, 0}},
            MeshVertex{{  1, 1, 1 }, {}, { 1, 0, 0}},
            // top
            MeshVertex{{ -1, 1, 1 }, {}, { 0, 1, 0}},
            MeshVertex{{  1, 1, 1 }, {}, { 0, 1, 0}},
            MeshVertex{{  1, 1,-1 }, {}, { 0, 1, 0}},
            MeshVertex{{ -1, 1,-1 }, {}, { 0, 1, 0}},
            // bottom
            MeshVertex{{ -1,-1,-1 }, {}, { 0,-1, 0}},
            MeshVertex{{  1,-1,-1 }, {}, { 0,-1, 0}},
            MeshVertex{{  1,-1, 1 }, {}, { 0,-1, 0}},
            MeshVertex{{ -1,-1, 1 }, {}, { 0,-1, 0}},
        };
        for (int i = 0; i < 6; ++i) {
            int b = i * 4;
            mesh.indices.insert(mesh.indices.end(),
                { b, b+1, b+2, b, b+2, b+3 });
        }
    }

    Camera camera;
    camera.eye    = Vec3(0, 1, 4);
    camera.target = Vec3(0, 0, 0);
    camera.aspect = (float)W / H;

    PhongShader shader;
    shader.mesh = &mesh;

    float angle = 0.0f;

    while (window.IsOpen()) {
        window.PollEvents();
        fb.Clear(Color(20, 20, 20));

        angle += DegToRad(40.0f) * window.DeltaTime();

        Mat4 model       = Mat4::Rotate(angle, Vec3(0, 1, 0));
        Mat4 vp          = camera.GetProjection() * camera.GetView();
        shader.mvp       = vp * model;
        shader.modelMat  = model;
        shader.cameraPos = camera.eye;

        pipeline.DrawIndexed(shader, mesh.indices);
        window.Present(fb);
    }
    return 0;
}
