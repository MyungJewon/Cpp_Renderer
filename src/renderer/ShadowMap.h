#pragma once
#include "../math/Mat4.h"
#include "../math/Vec3.h"
#include "../math/MathUtils.h"
#include <vector>

class ShadowMap {
public:
    ShadowMap(int width, int height);

    int   Width()  const { return m_width;  }
    int   Height() const { return m_height; }

    void  Clear();
    bool  TestAndSet(int x, int y, float depth); // shadow pass 기록
    float Sample(int x, int y) const;            // main pass 조회

    // 광원 시점 VP 행렬 설정
    void  SetLightVP(const Mat4& vp) { m_lightVP = vp; }
    Mat4  GetLightVP() const         { return m_lightVP; }

    // 월드 좌표 → 광원 공간 NDC (그림자 판별용)
    Vec3  WorldToLightNDC(const Vec3& worldPos) const;

private:
    int   m_width, m_height;
    std::vector<float> m_depth;
    Mat4  m_lightVP;
};
