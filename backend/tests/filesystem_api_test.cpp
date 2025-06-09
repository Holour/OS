#include "../libs/httplib.h"
#include "../libs/json.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <utility>
#include <sstream>
#include <iomanip>
#include <cctype>

using json = nlohmann::json;

// Helper to build clean URLs without worrying about slashes
std::string build_url(const std::vector<std::string>& parts) {
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        std::string part = parts[i];
        if (i > 0 && !result.empty() && result.back() == '/') {
            result.pop_back();
        }
        if (!part.empty() && part.front() != '/') {
            result += '/';
        }
        result += part;
    }
    return result;
}

// Pre-test cleanup helper
void cleanup_path(httplib::Client& cli, const std::string& path) {
    auto res = cli.Delete((path + "?recursive=true").c_str());
    // No assertion here, the resource might not exist, which is fine.
}

void test_filesystem_status(httplib::Client& cli) {
    auto res = cli.Get("/api/v1/filesystem/status");
    assert(res && res->status == 200);
    json body = json::parse(res->body);
    assert(body["status"] == "success");
    assert(body["data"]["total_space"].is_number());
    assert(body["data"]["used_space"].is_number());
    assert(body["data"]["allocation_method"].is_string());
    std::cout << "Test GET /api/v1/filesystem/status: PASSED" << std::endl;
}

void test_create_directory(httplib::Client& cli, const std::string& path, int expected_status) {
    json req_body = {{"path", path}};
    auto res = cli.Post("/api/v1/filesystem/directory", req_body.dump(), "application/json");
    assert(res && res->status == expected_status);
    std::cout << "Test POST /api/v1/filesystem/directory (path: " << path << ", expected: " << expected_status << "): PASSED" << std::endl;
}

void test_create_file(httplib::Client& cli, const std::string& path, uint64_t simulated_size, int expected_status) {
    json req_body = {{"path", path}, {"simulated_size", simulated_size}};
    auto res = cli.Post("/api/v1/filesystem/file", req_body.dump(), "application/json");
    assert(res && res->status == expected_status);
    std::cout << "Test POST /api/v1/filesystem/file (path: " << path << ", size: " << simulated_size << ", expected: " << expected_status << "): PASSED" << std::endl;
}

void test_read_file(httplib::Client& cli, const std::string& path, int expected_status, uint64_t expected_size = 0) {
    auto url = build_url({"/api/v1/filesystem/file", path});
    auto res = cli.Get(url);
    assert(res && res->status == expected_status);
    if (res->status == 200) {
        json body = json::parse(res->body);
        assert(body["status"] == "success");
        assert(body["data"]["simulated_size"] == expected_size);
    }
    std::cout << "Test GET " << url << " (expected: " << expected_status << "): PASSED" << std::endl;
}

void test_set_allocation_strategy(httplib::Client& cli, const std::string& strategy) {
    json req_body = {{"allocation_method", strategy}};
    auto res = cli.Put("/api/v1/filesystem/config", req_body.dump(), "application/json");
    assert(res && res->status == 200);
    
    // Verify by checking status
    auto status_res = cli.Get("/api/v1/filesystem/status");
    assert(status_res && status_res->status == 200);
    json status_body = json::parse(status_res->body);
    assert(status_body["data"]["allocation_method"] == strategy);
    
    std::cout << "Test PUT /api/v1/filesystem/config (strategy: " << strategy << "): PASSED" << std::endl;
}

void test_get_file_address(httplib::Client& cli, const std::string& path, const std::string& strategy) {
    auto res = cli.Get(("/api/v1/filesystem/file-address?path=" + path).c_str());
    assert(res && res->status == 200);
    json body = json::parse(res->body);
    assert(body["status"] == "success");
    assert(body["data"]["path"] == path);
    if (strategy == "CONTIGUOUS") {
        assert(body["data"]["addresses"]["contiguous"].is_number());
    } else if (strategy == "LINKED") {
        assert(body["data"]["addresses"]["linked"].is_number());
    } else if (strategy == "INDEXED") {
        assert(body["data"]["addresses"]["indexed"].is_number());
    }
    std::cout << "Test GET /file-address (path: " << path << ", strategy: " << strategy << "): PASSED" << std::endl;
}

void test_list_directory(httplib::Client& cli, const std::string& path, int expected_status, int expected_total_items = 0) {
    auto url = build_url({"/api/v1/filesystem/directory", path});
    auto res = cli.Get(url);
    assert(res && res->status == expected_status);
    if (res->status == 200) {
        json body = json::parse(res->body);
        assert(body["status"] == "success");
        assert(body["data"].is_array());
        assert(body["data"].size() == expected_total_items);
    }
    std::cout << "Test GET " << url << " (expected items: " << expected_total_items << "): PASSED" << std::endl;
}

void test_delete(httplib::Client& cli, const std::string& path, int expected_status, bool recursive = false) {
    auto url = build_url({"/api/v1/filesystem", path});
    if (recursive) {
        url += "?recursive=true";
    }
    auto res = cli.Delete(url);
    assert(res && res->status == expected_status);
    std::cout << "Test DELETE " << url << " (expected: " << expected_status << "): PASSED" << std::endl;
}

void run_strategy_test(httplib::Client& cli, const std::string& strategy_name) {
    std::cout << "\n\n--- Testing with Allocation Strategy: " << strategy_name << " ---" << std::endl;
    test_set_allocation_strategy(cli, strategy_name);

    const std::string TEST_DIR = "/test_run_" + strategy_name;
    const std::string SUB_DIR = TEST_DIR + "/subdir";
    const std::string FILE_1 = SUB_DIR + "/file1.txt";
    const std::string FILE_2 = TEST_DIR + "/file2.log";

    // Cleanup before starting
    cleanup_path(cli, TEST_DIR);

    // 1. Directory and File Creation
    std::cout << "\n--- Step 1: Creation ---" << std::endl;
    test_create_directory(cli, TEST_DIR, 201);
    test_create_directory(cli, TEST_DIR, 409); // Test creating an existing directory
    test_list_directory(cli, "/", 200, 5);
    test_create_directory(cli, SUB_DIR, 201);
    test_list_directory(cli, TEST_DIR, 200, 1);
    
    // 2. File Creation, Reading, and Address Checking
    std::cout << "\n--- Step 2: File Operations ---" << std::endl;
    test_create_file(cli, FILE_1, 5000, 201); // ~5KB file
    test_list_directory(cli, SUB_DIR, 200, 1);
    test_read_file(cli, FILE_1, 200, 5000);
    test_get_file_address(cli, FILE_1, strategy_name);
    
    test_create_file(cli, FILE_2, 30 * 1024 * 1024, 201); // 30MB file
    test_list_directory(cli, TEST_DIR, 200, 2);
    test_read_file(cli, FILE_2, 200, 30 * 1024 * 1024);
    test_get_file_address(cli, FILE_2, strategy_name);

    // 3. Deletion
    std::cout << "\n--- Step 3: Deletion ---" << std::endl;
    test_delete(cli, TEST_DIR, 400, false); // Directory not empty, non-recursive
    test_delete(cli, FILE_1, 200);
    test_list_directory(cli, SUB_DIR, 200, 0);
    test_delete(cli, FILE_2, 200);
    test_list_directory(cli, TEST_DIR, 200, 1);
    test_delete(cli, SUB_DIR, 200);
    test_list_directory(cli, TEST_DIR, 200, 0);
    test_delete(cli, TEST_DIR, 200, true); // Now it's empty, but test with recursive flag just in case
    test_list_directory(cli, "/", 200, 4); // Back to initial 4 root dirs
}

void run_filesystem_tests() {
    httplib::Client cli("localhost", 8080);
    cli.set_connection_timeout(5, 0);

    std::cout << "--- Starting Filesystem API Test ---" << std::endl;
    
    test_filesystem_status(cli);

    run_strategy_test(cli, "INDEXED");
    run_strategy_test(cli, "LINKED");
    run_strategy_test(cli, "CONTIGUOUS");
    
    // Test some failure cases
    std::cout << "\n\n--- Testing Failure Cases ---" << std::endl;
    const std::string NON_EXISTENT_FILE = "/non/existent/path/file.txt";
    test_read_file(cli, NON_EXISTENT_FILE, 404);
    test_delete(cli, NON_EXISTENT_FILE, 404);
    test_create_directory(cli, "/non/existent/dir", 409); // Parent does not exist
    test_create_file(cli, "/non/existent/file.txt", 100, 409); // Parent does not exist
    
    // Set back to default for any subsequent manual tests
    test_set_allocation_strategy(cli, "INDEXED");

    std::cout << "\n--- All Filesystem API tests passed! ---" << std::endl;
} 