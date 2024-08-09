cmake_minimum_required(VERSION 3.0)
project (camera_rtsps)


# 检查目标系统架构
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    message(STATUS "Target architecture is x86_64")
    # 针对 x86_64 的特定操作
    # 依赖系统的ffmepg相关库
    include_directories(/usr/local/include ./include)
    link_directories(/usr/local/lib /usr/lib)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    message(STATUS "Target architecture is ARM64")
    # 针对 ARM64 的特定操作
    # 使用BSP中的库
    include_directories(/usr/local/include ${CMAKE_CURRENT_SOURCE_DIR}/include
    /j6/platform_sdk/out/release-gcc_11.4-64/target/deploy/rootfs-dev/include
    /j6/platform_sdk/out/release-gcc_11.4-64/target/deploy/rootfs-dev/usr/include/cjson
    /j6/platform_sdk/out/release-gcc_11.4-64/build/external/liblog/liblog-dev/usr/hobot/include
    /j6/platform_sdk/hbre/camsys/libvio/src/vio_interface
    /j6/platform_sdk/hbre/camsys/libvpf/include
    /j6/platform_sdk/hbre/libmm/inc
    /j6/platform_sdk/hbre/libhbmem/inc
    /j6/platform_sdk/hbre/camsys/libvio/src/vio_interface/utils
    /j6/platform_sdk/hbre/camsys/libvio/include
    /j6/platform_sdk/hbre/camsys/include
    /j6/platform_sdk/hbre/camsys/libcam/include
    )
    
    link_directories(/usr/local/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    /usr/lib
    /j6/platform_sdk/out/release-gcc_11.4-64/target/deploy/rootfs-dev/usr/lib/aarch64-linux-gnu
    /j6/platform_sdk/out/release-gcc_11.4-64/hbrootfs/usr/lib
    /j6/platform_sdk/out/release-gcc_11.4-64/build/hbre/camsys/libvio/libvio-dev/usr/hobot/lib
    /j6/platform_sdk/out/release-gcc_11.4-64/build/hbre/libhbmem/libhbmem-dev/usr/hobot/lib
    /j6/platform_sdk/out/release-gcc_11.4-64/build/hbre/camsys/libvpf/libvpf-dev/usr/hobot/lib
    /j6/platform_sdk/out/release-gcc_11.4-64/build/external/liblog/liblog-dev/usr/hobot/lib
    /j6/platform_sdk/out/release-gcc_11.4-64/build/hbre/libmm/libmm-dev/usr/hobot/lib
    )

else()
    message(FATAL_ERROR "Unknown target architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()


aux_source_directory(./src SRC)


set(EXECUTABLE_OUTPUT_PATH ./)
add_compile_options(-g)

add_executable(demo_external ${SRC} )
target_link_libraries(demo_external pthread avutil avformat avcodec swscale swresample hbmem multimedia cam vio vpf vio rtppay rtsps)