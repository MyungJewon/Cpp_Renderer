# Mini Software Rasterizer

외부 라이브러리 없이 C++17과 Win32 API만으로 구현한 소프트웨어 래스터라이저입니다.

---

## 구현 기능

- [x] Win32 DIB Section 기반 픽셀 직접 출력
- [x] 색상 버퍼 + 깊이 버퍼 (Z-buffer)
- [x] Barycentric 좌표 기반 삼각형 래스터화
- [x] 퍼스펙티브 보정 보간 (UV, 법선, 색상)
- [x] Back-face culling
- [x] Near-plane 클리핑
- [x] MVP 행렬 파이프라인 (Model / View / Projection)
- [x] Vertex / Fragment 셰이더 인터페이스
- [x] OBJ 메시 로더
- [x] Phong 라이팅
- [x] TGA 텍스처 매핑
- [x] Shadow Map
- [x] Normal Map

---

## 렌더링 파이프라인

```
[CPU 메시 데이터]
       │
       ▼
  Vertex Shader          ← Model / View / Projection 행렬 적용
  (Local → Clip Space)
       │
       ▼
  Perspective Divide     ← Clip Space → NDC [-1, 1]
       │
       ▼
  Viewport Transform     ← NDC → Screen Space
       │
       ▼
  Back-face Culling      ← 서명된 넓이(signed area)로 판별
       │
       ▼
  Triangle Rasterization ← Barycentric 좌표 + Bounding Box 순회
       │
       ▼
  Z-buffer Test          ← 깊이 테스트 통과 픽셀만 진행
       │
       ▼
  Fragment Shader        ← 보간된 varying으로 최종 색상 결정
       │
       ▼
  Framebuffer → BitBlt   ← DIB Section → Win32 화면 출력
```

---

## 프로젝트 구조

```
SoftRenderer/
├── src/
│   ├── main.cpp
│   ├── math/
│   │   ├── Vec2.h
│   │   ├── Vec3.h
│   │   ├── Vec4.h
│   │   ├── Mat4.h
│   │   └── MathUtils.h
│   ├── renderer/
│   │   ├── Framebuffer.h/.cpp
│   │   ├── Rasterizer.h/.cpp
│   │   ├── Shader.h
│   │   └── Pipeline.h/.cpp
│   ├── window/
│   │   └── Win32Window.h/.cpp
│   ├── scene/
│   └── resource/
├── CMakeLists.txt
└── README.md
```

---

## 빌드 방법

**요구 사항:** CMake 3.20+, MSVC 또는 MinGW-w64, Windows

```bat
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
build\Release\SoftRenderer.exe
```

ESC 키로 종료합니다.

---

## 기술 선택

| 항목 | 선택 | 이유 |
|------|------|------|
| 화면 출력 | Win32 DIB Section + BitBlt | 외부 의존성 없이 픽셀 직접 제어 |
| 빌드 시스템 | CMake | 크로스 컴파일러 지원 |
| 외부 라이브러리 | 없음 | 파이프라인 직접 구현이 핵심 목적 |
| 모델 포맷 | .obj | 파싱 단순, 레퍼런스 풍부 |
| 텍스처 포맷 | .tga | 헤더 구조가 단순해 직접 파싱 가능 |
