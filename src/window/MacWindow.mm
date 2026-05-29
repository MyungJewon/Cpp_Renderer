#include "MacWindow.h"
#import <Cocoa/Cocoa.h>

// ── NSView: CGImage로 픽셀 버퍼를 직접 그림 ──────────────────────────────
@interface PixelView : NSView {
    const uint32_t* _pixels;
    int _width, _height;
}
- (void)updatePixels:(const uint32_t*)pixels width:(int)w height:(int)h;
@end

@implementation PixelView
- (void)updatePixels:(const uint32_t*)pixels width:(int)w height:(int)h {
    _pixels = pixels;
    _width  = w;
    _height = h;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    if (!_pixels) return;

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef dp = CGDataProviderCreateWithData(
        nullptr, _pixels, _width * _height * 4, nullptr);

    // 우리 버퍼 형식: uint32_t = 0xAARRGGBB (ToARGB)
    // 리틀엔디언 메모리 배치: [BB][GG][RR][AA]
    // kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst → ARGB로 올바르게 해석됨
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

// ── NSWindowDelegate: 창 닫기 처리 ──────────────────────────────────────
@interface WindowDelegate : NSObject<NSWindowDelegate>
@property(nonatomic, assign) bool* openPtr;
@end

@implementation WindowDelegate
- (BOOL)windowShouldClose:(NSWindow*)sender {
    *_openPtr = false;
    return YES;
}
- (void)windowWillClose:(NSNotification*)n {
    *_openPtr = false;
}
@end

// ── Impl 정의 ────────────────────────────────────────────────────────────
struct MacWindow::Impl {
    NSWindow*       window   = nil;
    PixelView*      view     = nil;
    WindowDelegate* delegate = nil;
};

// ── MacWindow 구현 ────────────────────────────────────────────────────────
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

MacWindow::~MacWindow() {
    if (m_impl->window) [m_impl->window close];
}

void MacWindow::PollEvents() {
    // 블로킹 없이 대기 중인 이벤트 모두 처리
    NSEvent* e;
    while ((e = [NSApp nextEventMatchingMask:NSEventMaskAny
                        untilDate:[NSDate distantPast]
                        inMode:NSDefaultRunLoopMode
                        dequeue:YES])) {
        if ([e type] == NSEventTypeKeyDown &&
            [e keyCode] == 53) {  // ESC
            m_open = false;
        }
        [NSApp sendEvent:e];
    }

    auto now = std::chrono::steady_clock::now();
    m_deltaTime = std::chrono::duration<float>(now - m_lastTime).count();
    m_lastTime  = now;
}

void MacWindow::Present(const Framebuffer& fb) {
    [m_impl->view updatePixels:fb.ColorData() width:fb.Width() height:fb.Height()];
    [m_impl->view display];
}
