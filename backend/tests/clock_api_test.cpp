#include "../libs/httplib.h"
#include "../libs/json.hpp"
#include "../include/common.h"
#include <iostream>
#include <cassert>
#include <string>
#include <chrono>
#include <thread>
#include <vector>

#ifndef CLOCK_API_TEST_H
#define CLOCK_API_TEST_H

using json = nlohmann::json;

void test_get_time(httplib::Client& cli) {
    auto res = cli.Get("/api/v1/clock/time");
    assert(res && res->status == 200);
    json body = json::parse(res->body);
    assert(body["status"] == "success");
    assert(body["data"]["ticks"].is_number());
    std::cout << "Test GET /api/v1/clock/time: PASSED" << std::endl;
}

void test_set_interval(httplib::Client& cli, int interval, int expected_status) {
    json req_body = {{"interval", interval}};
    auto res = cli.Post("/api/v1/clock/interval", req_body.dump(), "application/json");
    assert(res && res->status == expected_status);
    std::cout << "Test POST /api/v1/clock/interval (interval: " << interval << ", expected: " << expected_status << "): PASSED" << std::endl;
}

void test_create_timer(httplib::Client& cli, const json& req_body, int expected_status) {
    auto res = cli.Post("/api/v1/clock/timer", req_body.dump(), "application/json");
    assert(res && res->status == expected_status);
    std::cout << "Test POST /api/v1/clock/timer (body: " << req_body.dump() << ", expected: " << expected_status << "): PASSED" << std::endl;
}

void test_clock_error_handling(httplib::Client& cli) {
    // 1. Test setting an invalid clock interval
    test_set_interval(cli, 0, 400);
    test_set_interval(cli, -1, 400);
    std::cout << "Test Error Handling (set invalid interval): PASSED" << std::endl;
    
    // 2. Test setting an invalid timer (e.g., negative delay)
    test_create_timer(cli, {{"delay", 0}}, 400);
    test_create_timer(cli, {{"delay", -100}}, 400);
    std::cout << "Test Error Handling (set invalid timer delay): PASSED" << std::endl;

    // 3. Test setting a repeating timer with invalid interval
    test_create_timer(cli, {{"delay", 1000}, {"repeat", true}, {"interval", 0}}, 400);
    test_create_timer(cli, {{"delay", 1000}, {"repeat", true}, {"interval", -10}}, 400);
    std::cout << "Test Error Handling (set invalid repeating timer interval): PASSED" << std::endl;
}

void run_clock_api_tests() {
    httplib::Client cli("localhost", 8080);
    std::cout << "\n--- Running Clock Management API Tests ---" << std::endl;

    // 1. Get initial time
    test_get_time(cli);

    // 2. Set a valid clock interval
    test_set_interval(cli, 100, 200);

    // 3. Create valid timers
    test_create_timer(cli, {{"delay", 1000}}, 201); // Simple non-repeating timer
    test_create_timer(cli, {{"delay", 500}, {"repeat", true}, {"interval", 200}}, 201); // Repeating timer

    // 4. Get time again to see if it progressed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    test_get_time(cli);

    // 5. Test various error handling cases
    test_clock_error_handling(cli);

    std::cout << "--- All Clock Management API tests passed! ---" << std::endl;
}

#endif // CLOCK_API_TEST_H 