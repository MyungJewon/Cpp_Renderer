#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../renderer/Framebuffer.h"

class Win32Window {
public:
    Win32Window(int width, int height, const char* title);
    ~Win32Window();

    bool IsOpen() const { return m_open; }      // 창이 열려 있으면 true
    void PollEvents();                          // 메시지 펌프 + 델타타임 갱신 (매 프레임 호출)
    void Present(const Framebuffer& fb);        // 프레임버퍼 → DIB → BitBlt 화면 출력
    float DeltaTime() const { return m_deltaTime; } // 이전 프레임과의 경과 시간 (초)

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp); // Win32 메시지 핸들러

    HWND    m_hwnd      = nullptr;
    HDC     m_hdc       = nullptr;
    HBITMAP m_bitmap    = nullptr;
    void*   m_dibBits   = nullptr;   // pixel pointer into the DIB section
    int     m_width, m_height;
    bool    m_open      = true;

    LARGE_INTEGER m_freq, m_lastTime;
    float m_deltaTime = 0.016f;
};
