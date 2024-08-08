cmake_minimum_required(VERSION 3.0)
project (camera_rtsp)

find_package(OpenCV REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)
# 检查目标系统架构
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    message(STATUS "Target architecture is x86_64")
    # 针对 x86_64 的特定操作
    # 依赖系统的ffmepg相关库
    include_directories(/usr/local/include ${OpenCV_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIRS})
    link_directories(/usr/local/lib /usr/lib ${FFMPEG_LIBRARY_DIRS} /usr/lib/x86_64-linux-gnu)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    message(STATUS "Target architecture is ARM64")
    # 针对 ARM64 的特定操作
    # 使用BSP中的库
    include_directories(/usr/local/include ./ ./rtp ./3rdparty /j6/platform_sdk/out/release-gcc_11.4-64/target/deploy/rootfs-dev/include)
    link_directories(/usr/local/lib /usr/lib /j6/platform_sdk/out/release-gcc_11.4-64/target/deploy/rootfs-dev/usr/lib/aarch64-linux-gnu)
else()
    message(FATAL_ERROR "Unknown target architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(EXECUTABLE_OUTPUT_PATH ./)
# add_compile_options(-g)
# set(CMAKE_BUILD_TYPE Release)
message(FFMPEG librarys : ${FFMPEG_LIBRARY_DIRS} - ${FFMPEG_LIBRARIES})
add_executable(camera_rtsp main.cc)
target_link_libraries(camera_rtsp pthread avutil avformat avcodec swscale swresample ${OpenCV_LIBS})

# add_executable(camera_h264 h264test.cc)
# target_link_libraries(camera_h264 pthread avutil avformat avcodec swscale swresample ${OpenCV_LIBS})

add_executable(ffh264 ffh264.cc)
target_link_libraries(ffh264 pthread ${OpenCV_LIBS} ${FFMPEG_LIBRARIES} avutil avformat avcodec swscale swresample)
add_executable(ffh2642 ffh2642.cc)
target_link_libraries(ffh2642 pthread ${OpenCV_LIBS} ${FFMPEG_LIBRARIES} avutil avformat avcodec swscale swresample)


