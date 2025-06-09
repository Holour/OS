#include "../libs/httplib.h"
#include "../libs/json.hpp"
#include "../include/common.h"
#include <iostream>
#include <cassert>
#include <string>
#include <chrono>
#include <thread>
#include <vector>

using json = nlohmann::json;

void test_create_semaphore(httplib::Client& cli, int initial_value, const std::string& name) {
    json req_body;
    req_body["initial_value"] = initial_value;
    req_body["name"] = name;
    
    auto res = cli.Post("/api/v1/sync/semaphore", req_body.dump(), "application/json");
    assert(res);
    assert(res->status == 201);
    
    json res_body = json::parse(res->body);
    assert(res_body["status"] == "success");
    assert(res_body["data"]["name"] == name);
    assert(res_body["data"]["value"] == initial_value);
    
    std::cout << "Test POST /api/v1/sync/semaphore: PASSED (name: " << name << ")" << std::endl;
    return res_body["data"]["sem_id"];
}

void test_semaphore_operation(httplib::Client& cli, int sem_id, const std::string& operation, bool should_succeed) {
    json req_body;
    req_body["operation"] = operation;
    req_body["timeout"] = 5000;
    
    auto res = cli.Post("/api/v1/sync/semaphore/" + std::to_string(sem_id) + "/operation", 
                       req_body.dump(), "application/json");
    assert(res);
    
    if (should_succeed) {
        assert(res->status == 200);
        json res_body = json::parse(res->body);
        assert(res_body["status"] == "success");
        assert(res_body["data"]["success"] == true);
        std::cout << "Test POST /api/v1/sync/semaphore/{id}/operation (" << operation << "): PASSED" << std::endl;
    } else {
        assert(res->status == 400);
        std::cout << "Test POST /api/v1/sync/semaphore/{id}/operation (" << operation << "): PASSED (expected failure)" << std::endl;
    }
}

void test_concurrent_semaphore_operations(httplib::Client& cli, int sem_id) {
    std::vector<std::thread> threads;
    std::vector<bool> results(4, false);
    
    // 创建4个线程同时进行P操作
    for (int i = 0; i < 4; i++) {
        threads.emplace_back([&cli, sem_id, &results, i]() {
            json req_body;
            req_body["operation"] = "P";
            req_body["timeout"] = 1000;
            
            auto res = cli.Post("/api/v1/sync/semaphore/" + std::to_string(sem_id) + "/operation", 
                              req_body.dump(), "application/json");
            results[i] = (res && res->status == 200);
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 验证只有一个线程成功获取信号量
    int success_count = std::count(results.begin(), results.end(), true);
    assert(success_count == 1);
    std::cout << "Test concurrent semaphore operations: PASSED" << std::endl;
}

int main() {
    httplib::Client cli("localhost", 8080);
    cli.set_connection_timeout(5, 0);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "--- Starting Process Synchronization API Test ---" << std::endl;
    
    try {
        // 1. 创建信号量
        int sem_id = test_create_semaphore(cli, 1, "test_mutex");
        
        // 2. 测试P操作
        test_semaphore_operation(cli, sem_id, "P", true);
        
        // 3. 测试V操作
        test_semaphore_operation(cli, sem_id, "V", true);
        
        // 4. 测试并发操作
        test_concurrent_semaphore_operations(cli, sem_id);
        
        // 5. 测试错误操作
        test_semaphore_operation(cli, 999, "P", false); // 不存在的信号量
        
        std::cout << "--- All Process Synchronization API tests passed! ---" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "A test failed with an exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 