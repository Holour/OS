#include "../libs/httplib.h"
#include "../libs/json.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "../include/common.h" // For ProcessID

#ifndef PROCESS_API_TEST_H
#define PROCESS_API_TEST_H

using json = nlohmann::json;

void test_get_processes(httplib::Client& cli, int expected_count) {
    auto res = cli.Get("/api/v1/processes");
    assert(res && res->status == 200);
    json body = json::parse(res->body);
    assert(body["status"] == "success");
    assert(body["data"].is_array());
    assert(body["data"].size() == expected_count);
    std::cout << "Test GET /api/v1/processes (count: " << expected_count << "): PASSED" << std::endl;
}

void test_create_process(httplib::Client& cli, uint64_t size, bool should_succeed, ProcessID* out_pid = nullptr) {
    json req_body = {{"memory_size", size}};
    auto res = cli.Post("/api/v1/processes", req_body.dump(), "application/json");
    assert(res);
    int expected_status = should_succeed ? 201 : 400;
    assert(res->status == expected_status);

    if (should_succeed) {
        json body = json::parse(res->body);
        assert(body["status"] == "success");
        assert(body["data"]["pid"].is_number());
        if (out_pid) {
            *out_pid = body["data"]["pid"];
        }
    }
    std::cout << "Test POST /api/v1/processes (size: " << size << ", success: " << should_succeed << "): PASSED" << std::endl;
}

void test_terminate_process(httplib::Client& cli, ProcessID pid, bool should_succeed) {
    std::string url = "/api/v1/processes/" + std::to_string(pid);
    auto res = cli.Delete(url);
    assert(res);
    int expected_status = should_succeed ? 200 : 404;
    assert(res->status == expected_status);
    std::cout << "Test DELETE /api/v1/processes/" << pid << " (success: " << should_succeed << "): PASSED" << std::endl;
}

void run_process_management_tests() {
    httplib::Client cli("localhost", 8080);
    std::cout << "\n--- Running Process Management API Tests ---" << std::endl;

    // 1. Get initial state
    auto res = cli.Get("/api/v1/processes");
    assert(res && res->status == 200);
    json initial_body = json::parse(res->body);
    int initial_process_count = initial_body["data"].size();
    std::cout << "Initial process count: " << initial_process_count << std::endl;
    test_get_processes(cli, initial_process_count);

    // 2. Create a process successfully
    ProcessID pid1 = -1;
    test_create_process(cli, 1024, true, &pid1);
    assert(pid1 != -1);
    test_get_processes(cli, initial_process_count + 1);

    // 3. Create another process
    ProcessID pid2 = -1;
    test_create_process(cli, 2048, true, &pid2);
    assert(pid2 != -1);
    test_get_processes(cli, initial_process_count + 2);

    // 4. Terminate the first process
    test_terminate_process(cli, pid1, true);
    test_get_processes(cli, initial_process_count + 1);

    // 5. Terminate the second process
    test_terminate_process(cli, pid2, true);
    test_get_processes(cli, initial_process_count);

    // 6. Error handling: terminate non-existent process
    test_terminate_process(cli, 999, false);

    // 7. Error handling: create process with too much memory
    test_create_process(cli, MEMORY_SIZE * 2, false);
    test_get_processes(cli, initial_process_count);

    std::cout << "--- All Process Management API tests passed! ---" << std::endl;
}

#endif // PROCESS_API_TEST_H 