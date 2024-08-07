cmake_minimum_required(VERSION 3.0)
project (RTSPSERVER)


# 检查目标系统架构
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    message(STATUS "Target architecture is x86_64")
    # 针对 x86_64 的特定操作
    # 依赖系统的ffmepg相关库
    include_directories(/usr/local/include ./ ./rtp ./3rdparty)
    link_directories(/usr/local/lib /usr/lib)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    message(STATUS "Target architecture is ARM64")
    # 针对 ARM64 的特定操作
    # 使用BSP中的库
    include_directories(/usr/local/include ./ ./rtp ./3rdparty /j6/platform_sdk/out/release-gcc_11.4-64/target/deploy/rootfs-dev/include)
    link_directories(/usr/local/lib /usr/lib /j6/platform_sdk/out/release-gcc_11.4-64/target/deploy/rootfs-dev/usr/lib/aarch64-linux-gnu)

else()
    message(FATAL_ERROR "Unknown target architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()


aux_source_directory(./ SRC)
aux_source_directory(./rtp RTP)
aux_source_directory(./3rdparty 3RD)

set(EXECUTABLE_OUTPUT_PATH ./)
add_compile_options(-g)
set(CMAKE_BUILD_TYPE Release)

add_executable(rtsp_server ${SRC} ${RTP} ${3RD})
target_link_libraries(rtsp_server pthread avutil avformat avcodec swscale swresample)

add_subdirectory(camera_rtsp)