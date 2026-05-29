cmake_minimum_required(VERSION 3.20)
project(SoftRenderer CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_SOURCES
    src/main.cpp
    src/renderer/Framebuffer.cpp
    src/renderer/Rasterizer.cpp
    src/renderer/Pipeline.cpp
    src/resource/ObjLoader.cpp
)

if(WIN32)
    list(APPEND COMMON_SOURCES src/window/Win32Window.cpp)
    add_executable(SoftRenderer WIN32 ${COMMON_SOURCES})
    target_link_libraries(SoftRenderer PRIVATE gdi32 user32)

elseif(APPLE)
    list(APPEND COMMON_SOURCES src/window/MacWindow.mm)
    add_executable(SoftRenderer ${COMMON_SOURCES})
    target_link_libraries(SoftRenderer PRIVATE "-framework Cocoa")

else()
    message(FATAL_ERROR "Unsupported platform. Only Win32 and macOS are supported.")
endif()

target_include_directories(SoftRenderer PRIVATE src)

# 빌드 후 assets 폴더를 실행 파일 옆으로 복사
add_custom_command(TARGET SoftRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/src/assets $<TARGET_FILE_DIR:SoftRenderer>/assets
)

target_compile_options(SoftRenderer PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -O2>
)
