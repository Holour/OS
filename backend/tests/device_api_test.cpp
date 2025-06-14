#include "../libs/httplib.h"
#include "../libs/json.hpp"
#include "../include/common.h"
#include <iostream>
#include <cassert>
#include <string>
#include <chrono>
#include <thread>
#include <vector>

#ifndef DEVICE_API_TEST_H
#define DEVICE_API_TEST_H

using json = nlohmann::json;

void test_get_devices(httplib::Client& cli) {
    auto res = cli.Get("/api/v1/devices");
    assert(res && res->status == 200);
    json body = json::parse(res->body);
    assert(body["status"] == "success");
    assert(body["data"].is_array());
    std::cout << "Test GET /api/v1/devices: PASSED" << std::endl;
}

void test_request_device(httplib::Client& cli, int device_id, int process_id, int expected_status) {
    json req_body = {{"device_id", device_id}, {"process_id", process_id}};
    auto res = cli.Post("/api/v1/devices/request", req_body.dump(), "application/json");
    assert(res && res->status == expected_status);
    if (res->status == 200) {
        json body = json::parse(res->body);
        assert(body["status"] == "success");
        assert(body["data"]["device_id"].get<int>() == device_id);
        assert(body["data"]["current_user"].get<int>() == process_id);
    }
    std::cout << "Test POST /api/v1/devices/request (dev_id: " << device_id << ", pid: " << process_id << ") expected " << expected_status << ": PASSED" << std::endl;
}

void test_release_device(httplib::Client& cli, int device_id, int process_id, int expected_status) {
    std::string url = "/api/v1/devices/" + std::to_string(device_id) + "/release";
    json body = {{"process_id", process_id}};
    auto res = cli.Post(url, body.dump(), "application/json");
    assert(res && res->status == expected_status);
    std::cout << "Test POST " << url << " release by pid " << process_id << " expected " << expected_status << ": PASSED" << std::endl;
}

void test_delete_device(httplib::Client& cli, int device_id, int expected_status) {
    std::string url = "/api/v1/devices/" + std::to_string(device_id);
    auto res = cli.Delete(url);
    assert(res && res->status == expected_status);
    std::cout << "Test DELETE " << url << " expected " << expected_status << ": PASSED" << std::endl;
}

void test_device_operation(httplib::Client& cli, int device_id, const std::string& op, int expected_status) {
    std::string url = "/api/v1/devices/" + std::to_string(device_id) + "/operation";
    json req_body = {{"operation", op}};
    auto res = cli.Post(url, req_body.dump(), "application/json");
    assert(res && res->status == expected_status);
    std::cout << "Test POST " << url << " op:" << op << " expected " << expected_status << ": PASSED" << std::endl;
}

void test_device_error_handling(httplib::Client& cli, int allocated_device_id) {
    // 1. Test requesting a non-existent device type
    json req_body;
    req_body["device_type"] = "NONEXISTENT_DEVICE";
    
    auto res = cli.Post("/api/v1/devices/request", req_body.dump(), "application/json");
    assert(res && res->status == 400);
    std::cout << "Test Error Handling (request non-existent device): PASSED" << std::endl;
    
    // 2. Test operating on a non-existent device
    json op_req;
    op_req["operation"] = "PRINT";
    op_req["params"] = {{"file_path", "/test.txt"}};
    
    res = cli.Post("/api/v1/devices/999/operation", op_req.dump(), "application/json");
    assert(res && res->status == 404); // Expect 404 for not found
    std::cout << "Test Error Handling (operate on non-existent device): PASSED" << std::endl;

    // 3. Test releasing an already released/idle device
    // First, release the one we allocated in the main test flow.
    test_release_device(cli, allocated_device_id, 0, 200);
    // Now, try to release it again. This should fail.
    test_release_device(cli, allocated_device_id, 0, 400);
    std::cout << "Test Error Handling (release idle device): PASSED" << std::endl;
}

void run_device_api_tests() {
    httplib::Client cli("localhost", 8080);
    std::cout << "\n--- Running Device Management API Tests (V2) ---" << std::endl;

    // 0. 测试获取设备列表接口
    test_get_devices(cli);

    // 1. 获取设备列表
    auto res = cli.Get("/api/v1/devices");
    assert(res && res->status == 200);
    json all_devices_json = json::parse(res->body);
    auto devices_arr = all_devices_json["data"];
    assert(devices_arr.is_array() && !devices_arr.empty());

    int first_device_id = devices_arr[0]["device_id"].get<int>();
    std::string first_device_status = devices_arr[0]["status"];

    // 如果首个设备忙碌，则寻找任意空闲设备
    if (first_device_status != "IDLE") {
        bool found_idle = false;
        for (const auto& dev : devices_arr) {
            if (dev["status"].get<std::string>() == "IDLE") {
                first_device_id = dev["device_id"].get<int>();
                found_idle = true;
                break;
            }
        }
        if (!found_idle) {
            std::cerr << "No idle device available for tests." << std::endl;
            return; // 退出测试
        }
    }

    int test_pid = 100; // 任意测试进程ID

    // 2. 申请设备
    test_request_device(cli, first_device_id, test_pid, 200);

    // 3. 再次申请同一设备（应失败）
    test_request_device(cli, first_device_id, test_pid + 1, 400);

    // 4. 进行一次占位操作
    test_device_operation(cli, first_device_id, "PRINT", 200);

    // 5. 释放设备
    test_release_device(cli, first_device_id, test_pid, 200);

    // 6. 释放已空闲设备（应失败）
    test_release_device(cli, first_device_id, test_pid, 400);

    // 7. 删除空闲设备（成功）
    test_delete_device(cli, first_device_id, 200);

    // 8. 删除不存在的设备（失败）
    test_delete_device(cli, first_device_id, 400);

    std::cout << "--- All Device Management API tests (V2) passed! ---" << std::endl;
}

#endif // DEVICE_API_TEST_H 