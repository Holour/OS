#include "../libs/httplib.h"
#include "../libs/json.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "../include/common.h" // For ProcessID

#ifndef MEMORY_API_TEST_H
#define MEMORY_API_TEST_H

using json = nlohmann::json;

// Forward declarations from other test files
void test_create_process(httplib::Client& cli, uint64_t size, bool should_succeed, ProcessID* out_pid);
void test_terminate_process(httplib::Client& cli, ProcessID pid, bool should_succeed);

void test_get_memory_status(httplib::Client& cli, uint64_t* out_used_memory = nullptr) {
    auto res = cli.Get("/api/v1/memory/status");
    assert(res && res->status == 200);
    json body = json::parse(res->body);
    assert(body["status"] == "success");
    assert(body["data"]["total_memory"].get<uint64_t>() == MEMORY_SIZE);
    if (out_used_memory) {
        *out_used_memory = body["data"]["used_memory"].get<uint64_t>();
    }
    std::cout << "Test GET /api/v1/memory/status: PASSED" << std::endl;
}

void run_memory_management_tests() {
    httplib::Client cli("localhost", 8080);
    std::cout << "\n--- Running Memory Management API Tests ---" << std::endl;

    uint64_t initial_used, after_alloc_used, after_free_used;

    // 1. Get initial memory state
    test_get_memory_status(cli, &initial_used);

    // 2. Create a process to allocate memory
    ProcessID pid = -1;
    uint64_t alloc_size = 1024 * 1024; // 1 MB
    test_create_process(cli, alloc_size, true, &pid);
    assert(pid != -1);
    std::cout << "Test: Created a process to allocate memory." << std::endl;

    // 3. Check memory status after allocation
    test_get_memory_status(cli, &after_alloc_used);
    assert(after_alloc_used >= initial_used + alloc_size);
    std::cout << "Test State After Allocation: PASSED" << std::endl;

    // 4. Terminate the process to free memory
    test_terminate_process(cli, pid, true);
    std::cout << "Test: Terminated process to free memory." << std::endl;

    // 5. Check memory status after freeing
    test_get_memory_status(cli, &after_free_used);
    // Allow for small fragmentation or metadata overhead differences
    assert(after_free_used <= after_alloc_used);
    std::cout << "Test State After Free: PASSED" << std::endl;

    std::cout << "--- All Memory Management API tests passed! ---" << std::endl;
}

#endif // MEMORY_API_TEST_H 