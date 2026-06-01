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
#include "renderer/ShadowMap.h"
#include "renderer/ShadowPass.h"
#include "renderer/OITBuffer.h"
#include "resource/ObjLoader.h"
#include "resource/Texture.h"
#include "scene/Camera.h"
#include "scene/Light.h"
#include "math/MathUtils.h"
#include <vector>
#include <cstdio>
#include <string>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <mach-o/dyld.h>
#include <limits.h>
#include <libgen.h>
#endif

static const int W = 800, H = 600;

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

// Phong + Shadow Map + Normal Map + 텍스처 통합 셰이더
struct PhongShader : IShader {
    const Mesh*      mesh      = nullptr;
    const Texture*   albedo    = nullptr;  // diffuse 텍스처
    const Texture*   normalMap = nullptr;  // tangent-space 노말맵
    const ShadowMap* shadowMap = nullptr;
    Mat4             mvp;
    Mat4             modelMat;
    Vec3             cameraPos;
    Light            light;

    VertexOut Vertex(int idx) override { // MVP 변환 + 월드 공간 속성 전달
        const MeshVertex& v = mesh->vertices[idx];
        Mat4 normalMat = modelMat.NormalMatrix();
        VertexOut out;
        out.clipPos          = mvp * Vec4(v.pos,    1.0f);
        out.varying.worldPos = (modelMat * Vec4(v.pos,    1.0f)).xyz();
        out.varying.normal   = (normalMat * Vec4(v.normal, 0.0f)).xyz().normalized();
        out.varying.tangent  = (normalMat * Vec4(v.tangent,0.0f)).xyz().normalized();
        out.varying.uv       = v.uv;
        return out;
    }

    Color Fragment(const Varying& v) override { // Phong 라이팅 계산 (Normal Map, Shadow, Texture 적용)
        // Normal Map: TBN 행렬로 탄젠트 공간 법선을 월드 공간으로 변환
        Vec3 N = v.normal.normalized();
        if (normalMap && normalMap->IsValid()) {
            Color ns   = normalMap->Sample(v.uv.x, v.uv.y);
            Vec3  tsN  = Vec3(ns.r/127.5f-1.0f, ns.g/127.5f-1.0f, ns.b/127.5f-1.0f).normalized();
            Vec3  T    = v.tangent;
            Vec3  B    = N.cross(T);
            N = (T * tsN.x + B * tsN.y + N * tsN.z).normalized();
        }

        Vec3 L = (light.position - v.worldPos).normalized();
        Vec3 V = (cameraPos      - v.worldPos).normalized();
        Vec3 R = (N * (N.dot(L) * 2.0f) - L).normalized();

        float diff = std::max(0.0f, N.dot(L));
        float spec = std::pow(std::max(0.0f, R.dot(V)), light.shininess);

        // Shadow Map: 광원 NDC 깊이 비교로 그림자 판별
        float shadow = 0.0f;
        if (shadowMap) {
            Vec3 lNDC = shadowMap->WorldToLightNDC(v.worldPos);
            int  sx   = (int)((lNDC.x + 1.0f) * 0.5f * (shadowMap->Width()  - 1));
            int  sy   = (int)((1.0f - lNDC.y)  * 0.5f * (shadowMap->Height() - 1));
            float bias = std::max(0.005f * (1.0f - N.dot(L)), 0.001f);
            if (lNDC.z > shadowMap->Sample(sx, sy) + bias) shadow = 1.0f;
        }

        float intensity = light.ambient
                        + (1.0f - shadow) * (light.diffuse * diff + light.specular * spec);

        Color base = (albedo && albedo->IsValid())
                   ? albedo->Sample(v.uv.x, v.uv.y)
                   : Color(200, 200, 200);

        Vec3 litColor = light.color * intensity;

        return Color(
            (uint8_t)std::min(255.0f, base.r * litColor.x),
            (uint8_t)std::min(255.0f, base.g * litColor.y),
            (uint8_t)std::min(255.0f, base.b * litColor.z)
        );
    }
};

// OIT용 반투명 셰이더 (단색 + alpha)
struct TransparentShader : IShader {
    const Mesh* mesh  = nullptr;
    Mat4        mvp;
    Color       color = Color(100, 180, 255);
    float       alpha = 0.4f;

    VertexOut Vertex(int idx) override { // 위치만 변환
        VertexOut out;
        out.clipPos = mvp * Vec4(mesh->vertices[idx].pos, 1.0f);
        return out;
    }

    Color Fragment(const Varying&) override { return color; } // 단색 반환 (OIT에서 alpha 적용)
};

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#else
int main(int, char**) {
#endif
    // 4x MSAA 활성화
    AppWindow   window(W, H, "SoftRenderer — Full Pipeline");
    Framebuffer fb(W, H, 4);
    Pipeline    pipeline(fb);

    std::string base = GetExeDir();
    Mesh mesh = ObjLoader::Load(base + "/assets/models/model.obj");
    if (!mesh.vertices.empty()) {
        ObjLoader::Normalize(mesh);
        printf("Model: %zu verts, %zu tris\n", mesh.vertices.size(), mesh.indices.size()/3);
    } else {
        printf("model.obj not found — fallback cube\n");
        mesh.vertices = {
            MeshVertex{{ -1,-1, 1 }, {0,0}, { 0, 0, 1}}, MeshVertex{{  1,-1, 1 }, {1,0}, { 0, 0, 1}},
            MeshVertex{{  1, 1, 1 }, {1,1}, { 0, 0, 1}}, MeshVertex{{ -1, 1, 1 }, {0,1}, { 0, 0, 1}},
            MeshVertex{{  1,-1,-1 }, {0,0}, { 0, 0,-1}}, MeshVertex{{ -1,-1,-1 }, {1,0}, { 0, 0,-1}},
            MeshVertex{{ -1, 1,-1 }, {1,1}, { 0, 0,-1}}, MeshVertex{{  1, 1,-1 }, {0,1}, { 0, 0,-1}},
            MeshVertex{{ -1,-1,-1 }, {0,0}, {-1, 0, 0}}, MeshVertex{{ -1,-1, 1 }, {1,0}, {-1, 0, 0}},
            MeshVertex{{ -1, 1, 1 }, {1,1}, {-1, 0, 0}}, MeshVertex{{ -1, 1,-1 }, {0,1}, {-1, 0, 0}},
            MeshVertex{{  1,-1, 1 }, {0,0}, { 1, 0, 0}}, MeshVertex{{  1,-1,-1 }, {1,0}, { 1, 0, 0}},
            MeshVertex{{  1, 1,-1 }, {1,1}, { 1, 0, 0}}, MeshVertex{{  1, 1, 1 }, {0,1}, { 1, 0, 0}},
            MeshVertex{{ -1, 1, 1 }, {0,0}, { 0, 1, 0}}, MeshVertex{{  1, 1, 1 }, {1,0}, { 0, 1, 0}},
            MeshVertex{{  1, 1,-1 }, {1,1}, { 0, 1, 0}}, MeshVertex{{ -1, 1,-1 }, {0,1}, { 0, 1, 0}},
            MeshVertex{{ -1,-1,-1 }, {0,0}, { 0,-1, 0}}, MeshVertex{{  1,-1,-1 }, {1,0}, { 0,-1, 0}},
            MeshVertex{{  1,-1, 1 }, {1,1}, { 0,-1, 0}}, MeshVertex{{ -1,-1, 1 }, {0,1}, { 0,-1, 0}},
        };
        for (int i = 0; i < 6; ++i) {
            int b = i*4;
            mesh.indices.insert(mesh.indices.end(), {b,b+1,b+2, b,b+2,b+3});
        }
    }

    Texture albedoTex  = Texture::Load(base + "/assets/textures/texture.tga");
    Texture normalTex  = Texture::Load(base + "/assets/textures/normal.tga");
    if (albedoTex.IsValid()) printf("Albedo texture loaded\n");
    if (normalTex.IsValid()) printf("Normal map loaded\n");

    Camera camera;
    camera.eye    = Vec3(0, 1, 4);
    camera.target = Vec3(0, 0, 0);
    camera.aspect = (float)W / H;

    // 반투명 쿼드 (OIT 데모용)
    Mesh quad;
    quad.vertices = {
        MeshVertex{{ -0.6f,-0.6f, 0.5f }, {}, { 0,0,1 }},
        MeshVertex{{  0.6f,-0.6f, 0.5f }, {}, { 0,0,1 }},
        MeshVertex{{  0.6f, 0.6f, 0.5f }, {}, { 0,0,1 }},
        MeshVertex{{ -0.6f, 0.6f, 0.5f }, {}, { 0,0,1 }},
    };
    quad.indices = { 0,1,2, 0,2,3 };

    ShadowMap          shadowMap(1024, 1024);
    ShadowPassRenderer shadowRenderer(shadowMap);
    ShadowShader       shadowShader;
    shadowShader.mesh = &mesh;

    OITBuffer oitBuffer(W, H);

    PhongShader shader;
    shader.mesh      = &mesh;
    shader.albedo    = &albedoTex;
    shader.normalMap = &normalTex;
    shader.shadowMap = &shadowMap;

    TransparentShader transShader;
    transShader.mesh = &quad;

    float angle = 0.0f;

    while (window.IsOpen()) {
        window.PollEvents();
        fb.Clear(Color(20, 20, 20));
        oitBuffer.Clear();

        angle += DegToRad(40.0f) * window.DeltaTime();
        Mat4 model = Mat4::Rotate(angle, Vec3(0, 1, 0));
        Mat4 vp    = camera.GetProjection() * camera.GetView();

        // 1. Shadow Pass
        Mat4 lightView = Mat4::LookAt(shader.light.position, Vec3(0,0,0), Vec3(0,1,0));
        Mat4 lightProj = Mat4::Perspective(DegToRad(90.0f), 1.0f, 0.1f, 20.0f);
        shadowMap.SetLightVP(lightProj * lightView);
        shadowMap.Clear();
        shadowShader.lightMVP = lightProj * lightView * model;
        shadowRenderer.Render(shadowShader, mesh.indices);

        // 2. Opaque Pass (Phong + Shadow + Texture + Normal Map)
        shader.mvp       = vp * model;
        shader.modelMat  = model;
        shader.cameraPos = camera.eye;
        pipeline.DrawIndexed(shader, mesh.indices);

        // 3. MSAA Resolve
        fb.Resolve();

        // 4. Transparent Pass (OIT: 반투명 쿼드 수집 후 합성)
        transShader.mvp      = vp;
        // OIT 데모 비활성화 (파란 사각형 제거)

        oitBuffer.Compose(fb);

        window.Present(fb);
    }
    return 0;
}
