#pragma once
#include "../renderer/Framebuffer.h"
#include <chrono>
#include <memory>

class MacWindow {
public:
    MacWindow(int width, int height, const char* title);
    ~MacWindow();

    bool  IsOpen()     const { return m_open; }
    void  PollEvents();                          // 이벤트 폴링 + 델타타임 갱신
    void  Present(const Framebuffer& fb);        // 프레임버퍼 → CGImage → NSView 출력
    float DeltaTime()  const { return m_deltaTime; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    bool  m_open      = true;
    float m_deltaTime = 0.016f;
    std::chrono::steady_clock::time_point m_lastTime;
};
