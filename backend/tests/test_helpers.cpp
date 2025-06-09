#include "test_helpers.h"
#include <cassert>

pid_t create_process(httplib::Client& cli, uint64_t memory_size) {
    json req_body;
    req_body["memory_size"] = memory_size;
    auto res = cli.Post("/api/v1/processes", req_body.dump(), "application/json");
    if (res && res->status == 201) {
        json res_body = json::parse(res->body);
        return res_body["data"]["pid"];
    }
    return -1; // Indicate failure
}

void kill_process(httplib::Client& cli, pid_t pid) {
    auto res = cli.Delete("/api/v1/processes/" + std::to_string(pid));
    // We don't assert here because in cleanup scenarios, the process might already be gone.
    // In test logic, the assertion should be in the test itself.
} 