cmake_minimum_required(VERSION 3.20)
project(SoftRenderer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows only
if(NOT WIN32)
    message(FATAL_ERROR "This project targets Win32 only.")
endif()

set(SOURCES
    src/main.cpp
    src/window/Win32Window.cpp
    src/renderer/Framebuffer.cpp
    src/renderer/Rasterizer.cpp
    src/renderer/Pipeline.cpp
)

add_executable(SoftRenderer WIN32 ${SOURCES})

target_include_directories(SoftRenderer PRIVATE src)

target_compile_options(SoftRenderer PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -O2>
)

# Link Win32 libs
target_link_libraries(SoftRenderer PRIVATE gdi32 user32)
