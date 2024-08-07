# 设置目标系统名称
set(CMAKE_SYSTEM_NAME Linux)

# 设置目标系统处理器架构
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 设置交叉编译工具链
set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

# 设置交叉编译的根路径
#set(CMAKE_FIND_ROOT_PATH /path/to/your/aarch64/sysroot)

# 设置查找模式
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)