#include "../libs/httplib.h"
#include "../libs/json.hpp"
#include "../include/common.h"
#include <iostream>
#include <cassert>
#include <string>
#include <chrono>
#include <thread>
#include <vector>

#ifndef INTERRUPT_API_TEST_H
#define INTERRUPT_API_TEST_H

using json = nlohmann::json;

void test_get_vector_table(httplib::Client& cli) {
    auto res = cli.Get("/api/v1/interrupts/vector_table");
    assert(res && res->status == 200);
    json body = json::parse(res->body);
    assert(body["status"] == "success");
    assert(body["data"]["vectors"].is_array());
    std::cout << "Test GET /api/v1/interrupts/vector_table: PASSED" << std::endl;
}

void test_register_handler(httplib::Client& cli, int vector, const std::string& type, int expected_status) {
    json req_body = {
        {"vector", vector},
        {"handler_type", type}
    };
    auto res = cli.Post("/api/v1/interrupts/handler", req_body.dump(), "application/json");
    assert(res && res->status == expected_status);
    std::cout << "Test POST /api/v1/interrupts/handler (vector: " << vector << ", expected: " << expected_status << "): PASSED" << std::endl;
}

void test_trigger_interrupt(httplib::Client& cli, int vector, int expected_status) {
    json req_body = {{"vector", vector}};
    auto res = cli.Post("/api/v1/interrupts/trigger", req_body.dump(), "application/json");
    assert(res && res->status == expected_status);
    std::cout << "Test POST /api/v1/interrupts/trigger (vector: " << vector << ", expected: " << expected_status << "): PASSED" << std::endl;
}

void run_interrupt_api_tests() {
    httplib::Client cli("localhost", 8080);
    std::cout << "\n--- Running Interrupt Handling API Tests ---" << std::endl;

    const int TIMER_VECTOR = 32;
    const int IO_VECTOR = 33;
    const int UNUSED_VECTOR = 40;

    // 1. Get initial vector table
    test_get_vector_table(cli);

    // 2. Register handlers
    test_register_handler(cli, TIMER_VECTOR, "TIMER", 201);
    test_register_handler(cli, IO_VECTOR, "IO", 201);
    test_register_handler(cli, TIMER_VECTOR, "TIMER", 400); // Re-register, expect failure

    // 3. Trigger interrupts
    test_trigger_interrupt(cli, TIMER_VECTOR, 200);
    test_trigger_interrupt(cli, IO_VECTOR, 200);
    test_trigger_interrupt(cli, UNUSED_VECTOR, 404); // Trigger unregistered vector

    // 4. Final state of vector table
    test_get_vector_table(cli);

    std::cout << "--- All Interrupt Handling API tests passed! ---" << std::endl;
}

#endif // INTERRUPT_API_TEST_H 