#include "../libs/httplib.h"
#include "../libs/json.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "../include/common.h" // For ProcessID
#include <cstdlib>

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

void test_memory_status_api() {
    httplib::Client cli("localhost", 8080);
    
    std::cout << "Testing Memory Status API..." << std::endl;
    
    // 测试获取内存状态
    auto res = cli.Get("/api/v1/memory/status");
    
    if (res && res->status == 200) {
        auto data = json::parse(res->body);
        
        if (data["status"] == "success" && data.contains("data")) {
            auto memory_data = data["data"];
            
            // 基本字段验证
            if (memory_data.contains("total_memory") && 
                memory_data.contains("used_memory") &&
                memory_data.contains("allocation_strategy")) {
                
                uint64_t total = memory_data["total_memory"];
                uint64_t used = memory_data["used_memory"];
                int strategy = memory_data["allocation_strategy"];
                
                std::cout << "✓ Memory Status API: PASSED" << std::endl;
                std::cout << "  Total Memory: " << total / (1024*1024) << " MB" << std::endl;
                std::cout << "  Used Memory: " << used / (1024*1024) << " MB" << std::endl;
                std::cout << "  Allocation Strategy: " << strategy << std::endl;
                
                // 根据策略验证特定字段
                if (strategy == 0 || strategy == 2) { // 连续分配或分页
                    if (memory_data.contains("free_blocks")) {
                        std::cout << "  Free blocks count: " << memory_data["free_blocks"].size() << std::endl;
                    }
                } else if (strategy == 1) { // 分区分配
                    if (memory_data.contains("partitions")) {
                        std::cout << "  Partitions count: " << memory_data["partitions"].size() << std::endl;
                    }
                }
            } else {
                std::cout << "✗ Memory Status API: Missing required fields" << std::endl;
            }
        } else {
            std::cout << "✗ Memory Status API: Invalid response format" << std::endl;
        }
    } else {
        std::cout << "✗ Memory Status API: Request failed" << std::endl;
    }
}

void test_memory_strategy_api() {
    httplib::Client cli("localhost", 8080);
    
    std::cout << "\nTesting Memory Strategy API..." << std::endl;
    
    // 测试设置分区分配策略
    json request_body = {{"strategy", 1}};
    auto res = cli.Put("/api/v1/memory/strategy", request_body.dump(), "application/json");
    
    if (res && res->status == 200) {
        auto data = json::parse(res->body);
        
        if (data["status"] == "success" && data.contains("data")) {
            std::cout << "✓ Memory Strategy Change API: PASSED" << std::endl;
            std::cout << "  Response: " << data["message"] << std::endl;
        } else {
            std::cout << "✗ Memory Strategy Change API: Invalid response format" << std::endl;
        }
    } else {
        std::cout << "✗ Memory Strategy Change API: Request failed" << std::endl;
    }
    
    // 验证策略是否真的改变了
    auto status_res = cli.Get("/api/v1/memory/status");
    if (status_res && status_res->status == 200) {
        auto data = json::parse(status_res->body);
        if (data["data"]["allocation_strategy"] == 1) {
            std::cout << "✓ Strategy verification: PASSED (now using PARTITIONED)" << std::endl;
        } else {
            std::cout << "✗ Strategy verification: FAILED" << std::endl;
        }
    }
    
    // 测试设置分页分配策略
    request_body = {{"strategy", 2}};
    res = cli.Put("/api/v1/memory/strategy", request_body.dump(), "application/json");
    
    if (res && res->status == 200) {
        std::cout << "✓ Memory Strategy Change to PAGED: PASSED" << std::endl;
    } else {
        std::cout << "✗ Memory Strategy Change to PAGED: FAILED" << std::endl;
    }
    
    // 测试无效策略值
    request_body = {{"strategy", 5}};
    res = cli.Put("/api/v1/memory/strategy", request_body.dump(), "application/json");
    
    if (res && res->status == 400) {
        std::cout << "✓ Invalid strategy test: PASSED (correctly rejected)" << std::endl;
    } else {
        std::cout << "✗ Invalid strategy test: FAILED (should reject invalid values)" << std::endl;
    }
    
    // 恢复到连续分配策略
    request_body = {{"strategy", 0}};
    cli.Put("/api/v1/memory/strategy", request_body.dump(), "application/json");
}

void run_memory_management_tests() {
    std::cout << "=== Memory Management API Tests ===" << std::endl;
    
    test_memory_status_api();
    test_memory_strategy_api();
    
    std::cout << "\nMemory API tests completed." << std::endl;
}

#endif // MEMORY_API_TEST_H 