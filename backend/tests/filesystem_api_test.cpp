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
    cli.Delete(path.c_str());
    // No assertion here, the resource might not exist, which is fine.
}

void test_filesystem_status(httplib::Client& cli) {
    auto res = cli.Get("/api/v1/filesystem/status");
    assert(res && res->status == 200);
    json body = json::parse(res->body);
    assert(body["status"] == "success");
    assert(body["data"]["total_space"].is_number());
    assert(body["data"]["used_space"].is_number());
    std::cout << "Test GET /api/v1/filesystem/status: PASSED" << std::endl;
}

void test_create_directory(httplib::Client& cli, const std::string& path, int expected_status) {
    json req_body = {{"path", path}};
    auto res = cli.Post("/api/v1/filesystem/directory", req_body.dump(), "application/json");
    assert(res && res->status == expected_status);
    std::cout << "Test POST /api/v1/filesystem/directory (path: " << path << ", expected: " << expected_status << "): PASSED" << std::endl;
}

void test_create_file(httplib::Client& cli, const std::string& path, const std::string& content, int expected_status) {
    json req_body = {{"path", path}, {"content", content}};
    auto res = cli.Post("/api/v1/filesystem/file", req_body.dump(), "application/json");
    assert(res && res->status == expected_status);
    std::cout << "Test POST /api/v1/filesystem/file (path: " << path << ", expected: " << expected_status << "): PASSED" << std::endl;
}

void test_read_file(httplib::Client& cli, const std::string& path, int expected_status, const std::string& expected_content = "") {
    auto url = build_url({"/api/v1/filesystem/file", path});
    auto res = cli.Get(url);
    assert(res && res->status == expected_status);
    if (res->status == 200) {
        json body = json::parse(res->body);
        assert(body["status"] == "success");
        assert(body["data"]["content"] == expected_content);
    }
    std::cout << "Test GET " << url << " (expected: " << expected_status << "): PASSED" << std::endl;
}

void test_update_file(httplib::Client& cli, const std::string& path, const std::string& new_content, int expected_status) {
    auto url = build_url({"/api/v1/filesystem/file", path});
    json req_body = {{"content", new_content}};
    auto res = cli.Put(url, req_body.dump(), "application/json");
    assert(res && res->status == expected_status);
    std::cout << "Test PUT " << url << " (expected: " << expected_status << "): PASSED" << std::endl;
}

void test_list_directory(httplib::Client& cli, const std::string& path, int expected_status, int expected_files = 0, int expected_dirs = 0) {
    auto url = build_url({"/api/v1/filesystem/directory", path});
    auto res = cli.Get(url);
    assert(res && res->status == expected_status);
    if (res->status == 200) {
        json body = json::parse(res->body);
        assert(body["status"] == "success");
        assert(body["data"]["files"].size() == expected_files);
        assert(body["data"]["directories"].size() == expected_dirs);
    }
    std::cout << "Test GET " << url << " (expected: " << expected_status << "): PASSED" << std::endl;
}

void test_delete(httplib::Client& cli, const std::string& path, int expected_status) {
    auto url = build_url({"/api/v1/filesystem", path});
    auto res = cli.Delete(url);
    assert(res && res->status == expected_status);
    std::cout << "Test DELETE " << url << " (expected: " << expected_status << "): PASSED" << std::endl;
}

void run_filesystem_tests() {
    httplib::Client cli("localhost", 8080);
    cli.set_connection_timeout(5, 0);

    std::cout << "--- Starting Filesystem API Test ---" << std::endl;

    // Use a unique directory for this test run to avoid conflicts with initial state
    const std::string TEST_DIR = "/test_run_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    const std::string DIR_HOME = TEST_DIR;
    const std::string DIR_USER = DIR_HOME + "/user";
    const std::string FILE_1 = DIR_USER + "/file1.txt";
    const std::string FILE_2 = DIR_HOME + "/file2.txt";
    const std::string NON_EXISTENT_FILE = "/non/existent/path/file.txt";

    // 1. Initial State & Setup
    std::cout << "\n--- Step 1: Initial State and Test Directory Setup ---" << std::endl;
    test_filesystem_status(cli);
    
    // Get initial directory count in root
    auto res = cli.Get("/api/v1/filesystem/directory/");
    assert(res && res->status == 200);
    json initial_root_body = json::parse(res->body);
    int initial_root_dirs = initial_root_body["data"]["directories"].size();
    
    // Create the main directory for this test run
    test_create_directory(cli, TEST_DIR, 201);
    test_list_directory(cli, "/", 200, 0, initial_root_dirs + 1);

    // 2. Directory Creation within Test Dir
    std::cout << "\n--- Step 2: Directory Creation ---" << std::endl;
    test_create_directory(cli, DIR_HOME, 400); // Already exists
    test_list_directory(cli, "/", 200, 0, initial_root_dirs + 1);
    test_create_directory(cli, DIR_USER, 201);
    test_list_directory(cli, DIR_HOME, 200, 0, 1);
    
    // 3. File Creation and Reading within Test Dir
    std::cout << "\n--- Step 3: File Creation and Reading ---" << std::endl;
    test_create_file(cli, FILE_1, "content1", 201);
    test_list_directory(cli, DIR_USER, 200, 1, 0);
    test_read_file(cli, FILE_1, 200, "content1");
    test_create_file(cli, FILE_2, "content2", 201);
    test_list_directory(cli, DIR_HOME, 200, 1, 1);
    test_read_file(cli, NON_EXISTENT_FILE, 404);

    // 4. File Updates within Test Dir
    std::cout << "\n--- Step 4: File Updates ---" << std::endl;
    test_update_file(cli, FILE_1, "new_content", 200);
    test_read_file(cli, FILE_1, 200, "new_content");
    test_update_file(cli, NON_EXISTENT_FILE, "content", 404);

    // 5. Deletion Logic within Test Dir
    std::cout << "\n--- Step 5: Deletion ---" << std::endl;
    test_delete(cli, DIR_HOME, 400); // Directory not empty
    test_delete(cli, FILE_1, 200);
    test_list_directory(cli, DIR_USER, 200, 0, 0);
    test_delete(cli, FILE_2, 200);
    test_list_directory(cli, DIR_HOME, 200, 0, 1);
    test_delete(cli, DIR_USER, 200);
    test_list_directory(cli, DIR_HOME, 200, 0, 0);
    test_delete(cli, DIR_HOME, 200); // Now it's empty
    test_list_directory(cli, "/", 200, 0, initial_root_dirs);
    test_delete(cli, NON_EXISTENT_FILE, 404);

    // 6. Final State Verification
    std::cout << "\n--- Step 6: Final State Verification ---" << std::endl;
    test_filesystem_status(cli);
    test_list_directory(cli, "/", 200, 0, initial_root_dirs); // Should be back to original state

    std::cout << "\n--- All Filesystem API tests passed! ---" << std::endl;
} 