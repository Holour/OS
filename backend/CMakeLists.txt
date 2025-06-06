cmake_minimum_required(VERSION 3.10)
project(os_simulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_NAME os_simulator)

include_directories(include)

# --- Source File Definitions ---

# Define the main entry point for the application
set(MAIN_SOURCES src/core/main.cpp)

# Collect library source files that will be used by the main executable and tests
set(MEMORY_SOURCES
    src/memory/memory_manager.cpp
)
set(PROCESS_SOURCES
    ${MEMORY_SOURCES}
    src/process/process_manager.cpp
    src/process/pcb.cpp
)
set(FS_SOURCES
    ${MEMORY_SOURCES}
    src/fs/fs_manager.cpp
)

# The main executable links against all library sources
add_executable(${EXECUTABLE_NAME} ${MAIN_SOURCES} ${PROCESS_SOURCES} ${FS_SOURCES})

# --- Unit Tests ---
enable_testing()
set(CTEST_OUTPUT_ON_FAILURE TRUE)

# --- Process Manager Test ---
add_executable(process_test
    tests/test_process_manager.cpp
    ${PROCESS_SOURCES}
)
add_test(NAME ProcessManagerTest COMMAND process_test)

# --- Memory Manager Test ---
add_executable(memory_test
    tests/test_memory_manager.cpp
    ${MEMORY_SOURCES}
)
add_test(NAME MemoryManagerTest COMMAND memory_test)

# --- File System Test ---
add_executable(fs_test
    tests/test_fs_manager.cpp
    ${FS_SOURCES}
)
add_test(NAME FileSystemManagerTest COMMAND fs_test)