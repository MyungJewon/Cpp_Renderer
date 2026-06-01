#include "MacWindow.h"
#import <Cocoa/Cocoa.h>

// ─── PixelView ────────────────────────────────────────────────────────────────
// 프레임버퍼 픽셀 데이터를 CGImage로 변환해 NSView에 직접 그리는 커스텀 뷰
@interface PixelView : NSView {
    const uint32_t* _pixels;
    int _width, _height;
}
- (void)updatePixels:(const uint32_t*)pixels width:(int)w height:(int)h;
@end

@implementation PixelView

// 새 프레임 데이터를 받아 다음 드로우 사이클에 화면을 갱신하도록 예약
- (void)updatePixels:(const uint32_t*)pixels width:(int)w height:(int)h {
    _pixels = pixels;
    _width  = w;
    _height = h;
    [self setNeedsDisplay:YES];
}

// CGImage를 생성해 NSView 컨텍스트에 픽셀 버퍼를 그림
// 버퍼 포맷(0xAARRGGBB)과 CGImage 포맷을 맞추기 위해 리틀엔디언 + ARGB 플래그 사용
- (void)drawRect:(NSRect)dirtyRect {
    if (!_pixels) return;

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef dp = CGDataProviderCreateWithData(
        nullptr, _pixels, _width * _height * 4, nullptr);

    CGImageRef img = CGImageCreate(
        _width, _height, 8, 32, _width * 4, cs,
        kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst,
        dp, nullptr, false, kCGRenderingIntentDefault);

    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    CGContextDrawImage(ctx, CGRectMake(0, 0, _width, _height), img);

    CGImageRelease(img);
    CGDataProviderRelease(dp);
    CGColorSpaceRelease(cs);
}
@end

// ─── WindowDelegate ───────────────────────────────────────────────────────────
// 창 닫기 이벤트를 C++ 측 m_open 플래그에 전달하기 위한 델리게이트
@interface WindowDelegate : NSObject<NSWindowDelegate>
@property(nonatomic, assign) bool* openPtr;
@end

@implementation WindowDelegate

// 닫기 버튼 클릭 시 메인 루프 종료 신호 전달
- (BOOL)windowShouldClose:(NSWindow*)sender {
    *_openPtr = false;
    return YES;
}

// 창이 실제로 닫힐 때 한 번 더 플래그 보장
- (void)windowWillClose:(NSNotification*)n {
    *_openPtr = false;
}
@end

// ─── MacWindow ────────────────────────────────────────────────────────────────
struct MacWindow::Impl {
    NSWindow*       window   = nil;
    PixelView*      view     = nil;
    WindowDelegate* delegate = nil;
};

// NSApplication 초기화, NSWindow + PixelView 생성, 델타타임 타이머 시작
MacWindow::MacWindow(int width, int height, const char* title)
    : m_impl(std::make_unique<Impl>())
{
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp finishLaunching];

    NSRect frame = NSMakeRect(0, 0, width, height);
    NSWindowStyleMask style = NSWindowStyleMaskTitled
                            | NSWindowStyleMaskClosable
                            | NSWindowStyleMaskMiniaturizable;

    m_impl->window = [[NSWindow alloc]
        initWithContentRect:frame
        styleMask:style
        backing:NSBackingStoreBuffered
        defer:NO];

    m_impl->view     = [[PixelView alloc] initWithFrame:frame];
    m_impl->delegate = [[WindowDelegate alloc] init];
    m_impl->delegate.openPtr = &m_open;

    [m_impl->window setContentView:m_impl->view];
    [m_impl->window setDelegate:m_impl->delegate];
    [m_impl->window setTitle:@(title)];
    [m_impl->window setReleasedWhenClosed:NO];
    [m_impl->window center];
    [m_impl->window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];

    m_lastTime = std::chrono::steady_clock::now();
}

// 창 리소스 해제
MacWindow::~MacWindow() {
    if (m_impl->window) [m_impl->window close];
}

// 블로킹 없이 이벤트 큐를 비우고 ESC 입력 처리, 델타타임 갱신
// 렌더 루프 최상단에서 매 프레임 호출
void MacWindow::PollEvents() {
    NSEvent* e;
    while ((e = [NSApp nextEventMatchingMask:NSEventMaskAny
                        untilDate:[NSDate distantPast]
                        inMode:NSDefaultRunLoopMode
                        dequeue:YES])) {
        if ([e type] == NSEventTypeKeyDown && [e keyCode] == 53)
            m_open = false;
        [NSApp sendEvent:e];
    }

    auto now    = std::chrono::steady_clock::now();
    m_deltaTime = std::chrono::duration<float>(now - m_lastTime).count();
    m_lastTime  = now;
}

// 프레임버퍼 픽셀 데이터를 NSView에 넘겨 즉시 화면에 출력
void MacWindow::Present(const Framebuffer& fb) {
    [m_impl->view updatePixels:fb.ColorData() width:fb.Width() height:fb.Height()];
    [m_impl->view display];
}
