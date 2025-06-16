# 设置CMake最低版本要求和项目名称
cmake_minimum_required(VERSION 3.10)
project(os_simulator)

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 包含目录 ---
# 添加项目和第三方库的头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
)

# --- OS模拟器可执行文件 ---
# 自动搜集src目录下所有的.cpp源文件
file(GLOB_RECURSE SIMULATOR_SOURCES 
    "src/*.cpp" 
    "src/*/*.cpp"
)

# 创建模拟器可执行文件
add_executable(os_simulator ${SIMULATOR_SOURCES})
# 链接Threads库
target_link_libraries(os_simulator PRIVATE ${CMAKE_THREAD_LIBS_INIT})

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


# --- CTest配置 ---
# 启用测试
enable_testing()
# 添加一个测试，该测试运行api_tests可执行文件
add_test(NAME FullApiIntegrationTest COMMAND api_tests)