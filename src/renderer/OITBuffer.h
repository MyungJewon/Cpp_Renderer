#pragma once
#include "Framebuffer.h"
#include <vector>

// 픽셀당 최대 저장 가능한 투명 프래그먼트 수
static constexpr int OIT_MAX_FRAGS = 8;

struct OITFragment {
    float  depth;
    Color  color;
    float  alpha;
};

class OITBuffer {
public:
    OITBuffer(int width, int height);

    void Clear();                                                        // 모든 픽셀의 프래그먼트 초기화
    void AddFragment(int x, int y, float depth, Color color, float alpha); // 투명 프래그먼트 추가
    void Compose(Framebuffer& fb);                                       // 깊이 정렬 후 불투명 버퍼 위에 알파 블렌딩

private:
    struct Pixel {
        OITFragment frags[OIT_MAX_FRAGS];
        int count = 0;
    };

    int              m_width, m_height;
    std::vector<Pixel> m_pixels;
};
