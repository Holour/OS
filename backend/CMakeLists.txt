# 设置CMake最低版本要求和项目名称
cmake_minimum_required(VERSION 3.10)
project(os_simulator)

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC 编码/宏设置 & 线程库
if (MSVC)
    add_compile_options(/utf-8)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

find_package(Threads REQUIRED)

# --- 包含目录 ---
# 添加项目和第三方库的头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
)

# --- OS模拟器库 ---
# 自动搜集src目录下除了main.cpp之外的所有.cpp源文件
file(GLOB_RECURSE SIMULATOR_LIB_SOURCES 
    "src/process/*.cpp"
    "src/memory/*.cpp"
    "src/fs/*.cpp"
    "src/interrupt/*.cpp"
    "src/device/*.cpp"
    "src/clock/*.cpp"
    "src/core/api_server.cpp" # 假设API服务器逻辑与main分离
)

# 创建模拟器库
add_library(os_simulator_lib ${SIMULATOR_LIB_SOURCES})
target_link_libraries(os_simulator_lib PRIVATE ${CMAKE_THREAD_LIBS_INIT})

# --- OS模拟器可执行文件 ---
# 创建模拟器可执行文件
add_executable(os_simulator src/core/main.cpp)
# 链接模拟器库
target_link_libraries(os_simulator PRIVATE os_simulator_lib)

# --- 为Windows平台链接网络库 ---
if(WIN32)
    target_link_libraries(os_simulator PRIVATE ws2_32 wsock32)
endif()

# 安装可执行文件到bin目录
install(TARGETS os_simulator DESTINATION bin)


# --- API测试可执行文件 ---
# 列出所有测试相关的源文件
set(API_TEST_SOURCES
    tests/api_tests.cpp
    tests/process_api_test.cpp
    tests/scheduler_api_test.cpp
    tests/memory_api_test.cpp
    tests/filesystem_api_test.cpp
    tests/device_api_test.cpp
    tests/interrupt_api_test.cpp
    tests/clock_api_test.cpp
    tests/relationship_api_test.cpp
)

# 创建测试可执行文件
add_executable(api_tests ${API_TEST_SOURCES})
# 链接Threads库 (httplib需要)
target_link_libraries(api_tests PRIVATE ${CMAKE_THREAD_LIBS_INIT})

# --- 为Windows平台链接网络库 ---
if(WIN32)
    target_link_libraries(api_tests PRIVATE ws2_32 wsock32)
endif()

# 安装可执行文件到bin目录
install(TARGETS api_tests DESTINATION bin)


# --- 单元测试可执行文件 ---
# 明确列出所有单元测试源文件
set(UNIT_TEST_SOURCES 
    tests/unit_test_main.cpp
    tests/test_clock_manager.cpp
    tests/test_device_manager.cpp
    tests/test_interrupt_manager.cpp
    tests/test_fs_manager.cpp
    tests/test_memory_manager.cpp
    tests/test_process_manager.cpp
)

# 创建单元测试可执行文件
add_executable(unit_tests ${UNIT_TEST_SOURCES})
# 链接模拟器库
target_link_libraries(unit_tests PRIVATE os_simulator_lib)


# --- CTest配置 ---
# 启用测试
enable_testing()
# 添加一个测试，该测试运行api_tests可执行文件
add_test(NAME FullApiIntegrationTest COMMAND api_tests)
# 添加一个测试，该测试运行unit_tests可执行文件
add_test(NAME UnitTests COMMAND unit_tests)