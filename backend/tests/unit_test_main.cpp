#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
#include "test_common.h"

// Forward declarations for test suites
// These functions are implemented in the test_*.cpp files
void run_clock_manager_tests();
void run_device_manager_tests();
void run_fs_manager_tests();
void run_memory_manager_tests();
void run_process_manager_tests();
void run_interrupt_manager_tests();
// Add more declarations as new test files are created

int main() {
    std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"Clock Manager", run_clock_manager_tests},
        {"Device Manager", run_device_manager_tests},
        {"FS Manager", run_fs_manager_tests},
        {"Memory Manager", run_memory_manager_tests},
        {"Process Manager", run_process_manager_tests},
        {"Interrupt Manager", run_interrupt_manager_tests}
        // Add more test suites here
    };

    int passed_count = 0;
    int failed_count = 0;

    std::cout << "--- Starting Unit Tests ---" << std::endl;

    for (const auto& test : tests) {
        try {
            std::cout << "\n--- Running " << test.first << " tests ---" << std::endl;
            test.second();
            std::cout << "--- " << test.first << " tests PASSED ---" << std::endl;
            passed_count++;
        } catch (const std::exception& e) {
            std::cerr << "--- " << test.first << " tests FAILED: " << e.what() << std::endl;
            failed_count++;
        }
    }

    std::cout << "\n--- Unit Test Summary ---" << std::endl;
    std::cout << "Total suites: " << tests.size() << std::endl;
    std::cout << "Passed: " << passed_count << std::endl;
    std::cout << "Failed: " << failed_count << std::endl;
    std::cout << "-------------------------" << std::endl;

    if (failed_count > 0) {
        std::cerr << "\n--- Some unit tests FAILED! ---" << std::endl;
        return 1;
    } else {
        std::cout << "\n--- All unit tests passed! ---" << std::endl;
        return 0;
    }
} 