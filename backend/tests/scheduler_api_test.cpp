#include "../libs/httplib.h"
#include "../libs/json.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include "../include/common.h" // For ProcessID

#ifndef SCHEDULER_API_TEST_H
#define SCHEDULER_API_TEST_H

using json = nlohmann::json;

// Since process creation/termination is a prerequisite for scheduler tests,
// we must use its test functions. To avoid linking errors, we declare them here
// and the definitions will be found by the linker from process_api_test.cpp.
void test_create_process(httplib::Client& cli, uint64_t size, bool should_succeed, ProcessID* out_pid);
void test_terminate_process(httplib::Client& cli, ProcessID pid, bool should_succeed);


void test_get_ready_queue(httplib::Client& cli, size_t expected_size) {
    auto res = cli.Get("/api/v1/scheduler/ready_queue");
    assert(res && res->status == 200);
    json body = json::parse(res->body);
    assert(body["status"] == "success");
    assert(body["data"].is_array() && body["data"].size() == expected_size);
    std::cout << "Test GET /api/v1/scheduler/ready_queue (expected size: " << expected_size << "): PASSED" << std::endl;
}

void test_scheduler_tick(httplib::Client& cli, bool should_find_process) {
    auto res = cli.Post("/api/v1/scheduler/tick");
    assert(res && res->status == 200);
    json body = json::parse(res->body);
    assert(body["status"] == "success");
    if (should_find_process) {
        assert(body["data"].is_object() && !body["data"].is_null());
        std::cout << "Test POST /api/v1/scheduler/tick: PASSED (Scheduled pid " << body["data"]["pid"] << ")" << std::endl;
    } else {
        assert(body["data"].is_null());
        std::cout << "Test POST /api/v1/scheduler/tick: PASSED (Ready queue was empty)" << std::endl;
    }
}

void run_scheduler_tests() {
    httplib::Client cli("localhost", 8080);
    std::cout << "\n--- Running Scheduler API Tests ---" << std::endl;

    // 1. Get initial state of the ready queue
    auto res = cli.Get("/api/v1/scheduler/ready_queue");
    assert(res && res->status == 200);
    int initial_ready_count = json::parse(res->body)["data"].size();
    std::cout << "Initial ready queue size: " << initial_ready_count << std::endl;
    test_get_ready_queue(cli, initial_ready_count);

    int current_ready_count = initial_ready_count;

    // If there are initial processes, test one tick
    if (current_ready_count > 0) {
        test_scheduler_tick(cli, true);
        // After a tick, one process is moved from READY to RUNNING state.
        current_ready_count--;
        test_get_ready_queue(cli, current_ready_count);
    } else {
        test_scheduler_tick(cli, false);
    }
    
    // 2. Create some more processes to populate the ready queue
    ProcessID p1, p2, p3;
    test_create_process(cli, 100, true, &p1);
    current_ready_count++;
    test_create_process(cli, 100, true, &p2);
    current_ready_count++;
    test_create_process(cli, 100, true, &p3);
    current_ready_count++;
    std::cout << "Created temp processes with PIDs: " << p1 << ", " << p2 << ", " << p3 << std::endl;

    // After creation, check the new size
    test_get_ready_queue(cli, current_ready_count);

    // 3. Tick the scheduler to run processes
    // Each tick moves one from READY to RUNNING, and one from RUNNING to READY.
    // So the total number of READY processes should not change on subsequent ticks,
    // unless the queue is empty.
    test_scheduler_tick(cli, true); 
    test_get_ready_queue(cli, current_ready_count);   

    test_scheduler_tick(cli, true); 
    test_get_ready_queue(cli, current_ready_count);   

    // 4. Clean up: terminate all created processes.
    // Terminating will remove them from ready/running states.
    test_terminate_process(cli, p1, true);
    current_ready_count--;
    test_terminate_process(cli, p2, true);
    current_ready_count--;
    test_terminate_process(cli, p3, true);
    current_ready_count--;
    std::cout << "Killed temp processes" << std::endl;
    
    // Brief pause to allow server to process terminations and update queues
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 5. 更改调度算法并验证
    auto putBody = json{{"algorithm","SJF"}}.dump();
    auto putRes = cli.Put("/api/v1/scheduler/config", putBody, "application/json");
    assert(putRes && putRes->status == 200);

    // 验证当前算法
    auto cfgRes = cli.Get("/api/v1/scheduler/config");
    assert(cfgRes && cfgRes->status == 200);
    assert(json::parse(cfgRes->body)["data"]["algorithm"] == "SJF");

    // 6. 获取甘特图数据
    auto ganttRes = cli.Get("/api/v1/scheduler/gantt_chart");
    assert(ganttRes && ganttRes->status == 200);
    json ganttBody = json::parse(ganttRes->body);
    assert(ganttBody["status"] == "success");
    assert(ganttBody["data"].is_array());
    std::cout << "Gantt entries count (SJF): " << ganttBody["data"].size() << std::endl;

    // 6b. 更改至 RR 并验证甘特图非空
    auto putBodyRR = json{{"algorithm","RR"}, {"time_slice", 3}}.dump();
    auto putResRR = cli.Put("/api/v1/scheduler/config", putBodyRR, "application/json");
    assert(putResRR && putResRR->status == 200);

    auto rrRes = cli.Get("/api/v1/scheduler/config");
    assert(rrRes && rrRes->status == 200);
    assert(json::parse(rrRes->body)["data"]["algorithm"] == "RR");

    auto rrGanttRes = cli.Get("/api/v1/scheduler/gantt_chart");
    assert(rrGanttRes && rrGanttRes->status == 200);
    json rrBody = json::parse(rrGanttRes->body);
    assert(rrBody["status"] == "success");
    assert(rrBody["data"].is_array() && rrBody["data"].size() > 0);
    std::cout << "Gantt entries count (RR): " << rrBody["data"].size() << std::endl;

    // 7. Final state
    test_get_ready_queue(cli, current_ready_count);
    assert(current_ready_count == initial_ready_count -1);

    std::cout << "--- All Scheduler API tests passed! ---" << std::endl;
}

#endif // SCHEDULER_API_TEST_H 