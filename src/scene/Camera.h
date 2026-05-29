#pragma once
#include "../math/Vec3.h"
#include "../math/Mat4.h"
#include "../math/MathUtils.h"

class Camera {
public:
    Vec3  eye    = { 0, 0, 3 };
    Vec3  target = { 0, 0, 0 };
    Vec3  up     = { 0, 1, 0 };
    float fovY   = DegToRad(60.0f);
    float aspect = 800.0f / 600.0f;
    float zNear  = 0.1f;
    float zFar   = 100.0f;

    Mat4 GetView()       const { return Mat4::LookAt(eye, target, up); }
    Mat4 GetProjection() const { return Mat4::Perspective(fovY, aspect, zNear, zFar); }
};
