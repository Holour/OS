#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <string>

// Forward declarations of our test suites
void run_process_tests();
void run_scheduler_tests();
void run_memory_tests();
void run_filesystem_tests();
void run_device_tests();
void run_interrupt_tests();
void run_clock_tests();

int main() {
    // A generous wait for the server to start, to be run once.
    std::cout << "Waiting for the server to initialize..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::vector<std::pair<std::string, void(*)()>> tests = {
        {"Process Management", run_process_tests},
        {"Scheduler", run_scheduler_tests},
        {"Memory Management", run_memory_tests},
        {"Filesystem", run_filesystem_tests},
        {"Device Management", run_device_tests},
        {"Interrupt Handling", run_interrupt_tests},
        {"Clock Management", run_clock_tests}
    };

    bool all_passed = true;
    for (const auto& test : tests) {
        try {
            std::cout << "\n--- Running " << test.first << " tests ---" << std::endl;
            test.second();
            std::cout << "--- " << test.first << " tests PASSED ---" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "--- " << test.first << " tests FAILED with an exception: " << e.what() << std::endl;
            all_passed = false;
        }
    }

    if (all_passed) {
        std::cout << "\n\n--- All API test suites passed! ---" << std::endl;
        return 0;
    } else {
        std::cerr << "\n\n--- Some API test suites FAILED! ---" << std::endl;
        return 1;
    }
} 