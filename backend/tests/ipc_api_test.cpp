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

int test_create_queue(httplib::Client& cli, const std::string& name, int max_size, int message_size) {
    json req_body;
    req_body["name"] = name;
    req_body["max_size"] = max_size;
    req_body["message_size"] = message_size;
    
    auto res = cli.Post("/api/v1/ipc/queue", req_body.dump(), "application/json");
    assert(res);
    assert(res->status == 201);
    
    json res_body = json::parse(res->body);
    assert(res_body["status"] == "success");
    assert(res_body["data"]["name"] == name);
    
    std::cout << "Test POST /api/v1/ipc/queue: PASSED (name: " << name << ")" << std::endl;
    return res_body["data"]["queue_id"];
}

void test_send_message(httplib::Client& cli, int queue_id, const std::string& message, int type, int priority) {
    json req_body;
    req_body["message"] = message;
    req_body["type"] = type;
    req_body["priority"] = priority;
    
    auto res = cli.Post("/api/v1/ipc/queue/" + std::to_string(queue_id) + "/send", 
                       req_body.dump(), "application/json");
    assert(res);
    assert(res->status == 200);
    
    json res_body = json::parse(res->body);
    assert(res_body["status"] == "success");
    assert(res_body["data"]["success"] == true);
    
    std::cout << "Test POST /api/v1/ipc/queue/{id}/send: PASSED" << std::endl;
}

void test_receive_message(httplib::Client& cli, int queue_id, const std::string& expected_message, int expected_type) {
    json req_body;
    req_body["timeout"] = 5000;
    req_body["type"] = expected_type;
    
    auto res = cli.Post("/api/v1/ipc/queue/" + std::to_string(queue_id) + "/receive", 
                       req_body.dump(), "application/json");
    assert(res);
    assert(res->status == 200);
    
    json res_body = json::parse(res->body);
    assert(res_body["status"] == "success");
    assert(res_body["data"]["message"] == expected_message);
    assert(res_body["data"]["type"] == expected_type);
    
    std::cout << "Test POST /api/v1/ipc/queue/{id}/receive: PASSED" << std::endl;
}

void test_queue_overflow(httplib::Client& cli, int queue_id) {
    // 尝试发送超过队列最大容量的消息
    for (int i = 0; i < 101; i++) { // 假设队列最大容量为100
        json req_body;
        req_body["message"] = "test message " + std::to_string(i);
        req_body["type"] = 1;
        
        auto res = cli.Post("/api/v1/ipc/queue/" + std::to_string(queue_id) + "/send", 
                          req_body.dump(), "application/json");
        
        if (i < 100) {
            assert(res && res->status == 200);
        } else {
            assert(res && res->status == 400); // 队列满
            break;
        }
    }
    
    std::cout << "Test queue overflow: PASSED" << std::endl;
}

void test_concurrent_send_receive(httplib::Client& cli, int queue_id) {
    std::vector<std::thread> senders;
    std::vector<std::thread> receivers;
    std::atomic<int> received_count(0);
    
    // 创建发送者线程
    for (int i = 0; i < 5; i++) {
        senders.emplace_back([&cli, queue_id, i]() {
            json req_body;
            req_body["message"] = "concurrent message " + std::to_string(i);
            req_body["type"] = 1;
            
            auto res = cli.Post("/api/v1/ipc/queue/" + std::to_string(queue_id) + "/send", 
                              req_body.dump(), "application/json");
            assert(res && res->status == 200);
        });
    }
    
    // 创建接收者线程
    for (int i = 0; i < 5; i++) {
        receivers.emplace_back([&cli, queue_id, &received_count]() {
            json req_body;
            req_body["timeout"] = 1000;
            req_body["type"] = 1;
            
            auto res = cli.Post("/api/v1/ipc/queue/" + std::to_string(queue_id) + "/receive", 
                              req_body.dump(), "application/json");
            if (res && res->status == 200) {
                received_count++;
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& t : senders) t.join();
    for (auto& t : receivers) t.join();
    
    // 验证所有消息都被接收
    assert(received_count == 5);
    std::cout << "Test concurrent send/receive: PASSED" << std::endl;
}

int main() {
    httplib::Client cli("localhost", 8080);
    cli.set_connection_timeout(5, 0);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "--- Starting IPC API Test ---" << std::endl;
    
    try {
        // 1. 创建消息队列
        int queue_id = test_create_queue(cli, "test_queue", 100, 1024);
        
        // 2. 测试发送消息
        test_send_message(cli, queue_id, "Hello, World!", 1, 0);
        
        // 3. 测试接收消息
        test_receive_message(cli, queue_id, "Hello, World!", 1);
        
        // 4. 测试队列溢出
        test_queue_overflow(cli, queue_id);
        
        // 5. 测试并发发送和接收
        test_concurrent_send_receive(cli, queue_id);
        
        std::cout << "--- All IPC API tests passed! ---" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "A test failed with an exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 