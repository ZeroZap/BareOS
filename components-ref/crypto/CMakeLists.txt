cmake_minimum_required(VERSION 3.10)
project(xy_tiny_crypto VERSION 1.0.0 LANGUAGES C)

# 设置 C 标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 包含目录
include_directories(
    inc
    ../hal/inc
    ../kernel/osal
    ../trace/xy_log/inc
    ../clib/xy_clib
)

# 添加子模块
add_subdirectory(xy_sm3)
add_subdirectory(xy_sm4)
add_subdirectory(xy_sm2)

# 收集源文件 - 核心算法在 src/
file(GLOB CRYPTO_SOURCES "src/*.c")
file(GLOB CRYPTO_HEADERS "inc/*.h")
file(GLOB TEST_SOURCES "test/*.c")

# 创建静态库
add_library(xy_tiny_crypto STATIC ${CRYPTO_SOURCES})

# 链接子模块
target_link_libraries(xy_tiny_crypto PRIVATE
    xy_sm3
    xy_sm4
    xy_sm2
)

# 设置库属性
set_target_properties(xy_tiny_crypto PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${CRYPTO_HEADERS}"
)

# Windows 系统需要链接额外库
if(WIN32)
    target_link_libraries(xy_tiny_crypto PRIVATE advapi32)
endif()

# 安装规则
install(TARGETS xy_tiny_crypto
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# 测试暂时禁用 (需要标准库支持)
# enable_testing()
# add_test(NAME crypto_test COMMAND test_crypto)

# 打包
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
