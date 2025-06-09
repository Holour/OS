#include <iostream>
#include <chrono>
#include <thread>
#include "../libs/httplib.h"

// Forward declarations for test runners from other files
void run_process_management_tests();
void run_scheduler_tests();
void run_memory_management_tests();
void run_filesystem_tests();
void run_device_api_tests();
void run_interrupt_api_tests();
void run_clock_api_tests();

void wait_for_server(httplib::Client& cli) {
    std::cout << "Waiting for the server to initialize..." << std::endl;
    for (int i = 0; i < 30; ++i) { // Wait for max 30 seconds
        auto res = cli.Get("/api/v1/memory/status");
        if (res && res->status == 200) {
            std::cout << "Server is up!" << std::endl;
            return;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cerr << "Server did not start in time. Aborting tests." << std::endl;
    exit(1);
}

int main() {
    httplib::Client cli("localhost", 8080);
    cli.set_connection_timeout(10, 0); // 10 seconds

    wait_for_server(cli);

    try {
        run_process_management_tests();
        run_scheduler_tests();
        run_memory_management_tests();
        run_filesystem_tests();
        run_device_api_tests();
        run_interrupt_api_tests();
        run_clock_api_tests();

        std::cout << "\n\n========================================" << std::endl;
        std::cout << "  ALL API TESTS PASSED SUCCESSFULLY!  " << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n\n--- A test failed with an exception: " << e.what() << " ---" << std::endl;
        return 1;
    }

    return 0;
} 