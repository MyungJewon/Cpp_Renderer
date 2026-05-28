#include "window/Win32Window.h"
#include "renderer/Framebuffer.h"
#include "renderer/Pipeline.h"
#include "renderer/Shader.h"
#include "math/MathUtils.h"
#include <vector>

static const int W = 800, H = 600;

// --- Minimal test shader: vertex-colored triangle ---
struct TestVertex {
    Vec3  pos;
    Color color;
};

struct ColorShader : IShader {
    std::vector<TestVertex> vertices;
    Mat4 mvp;

    VertexOut Vertex(int idx) override {
        const TestVertex& v = vertices[idx];
        VertexOut out;
        out.clipPos        = mvp * Vec4(v.pos, 1.0f);
        out.varying.color  = v.color;
        out.varying.worldPos = v.pos;
        return out;
    }

    Color Fragment(const Varying& v) override {
        return v.color;
    }
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Win32Window window(W, H, "SoftRenderer — Week 1");
    Framebuffer fb(W, H);
    Pipeline    pipeline(fb);

    ColorShader shader;
    shader.vertices = {
        { Vec3( 0.0f,  0.5f, 0.0f), Color(255, 50,  50 ) },
        { Vec3(-0.5f, -0.5f, 0.0f), Color( 50, 255, 50 ) },
        { Vec3( 0.5f, -0.5f, 0.0f), Color( 50,  50, 255) },
    };
    std::vector<int> indices = { 0, 1, 2 };

    float angle = 0.0f;

    while (window.IsOpen()) {
        window.PollEvents();
        fb.Clear(Color(30, 30, 30));

        angle += DegToRad(60.0f) * window.DeltaTime();

        // Rotate the triangle over time
        Mat4 model = Mat4::Rotate(angle, Vec3(0, 0, 1));
        Mat4 view  = Mat4::LookAt(Vec3(0, 0, 2), Vec3(0, 0, 0), Vec3(0, 1, 0));
        Mat4 proj  = Mat4::Perspective(DegToRad(60.0f), (float)W / H, 0.1f, 100.0f);
        shader.mvp = proj * view * model;

        pipeline.DrawIndexed(shader, indices);

        window.Present(fb);
    }
    return 0;
}
